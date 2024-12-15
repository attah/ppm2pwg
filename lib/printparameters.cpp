#include "printparameters.h"

#include "stringutils.h"

#include <charconv>
#include <regex>

#define MM_PER_IN 25.4
#define PTS_PER_IN 72.0

bool PrintParameters::isTwoSided() const
{
  return duplexMode != OneSided;
}

size_t PrintParameters::getPaperSizeWInPixels() const
{
  switch(paperSizeUnits)
  {
    case Pixels:
      return paperSizeW;
    case Millimeters:
      return floor(paperSizeW * hwResW / MM_PER_IN);
    case Inches:
      return paperSizeW * hwResW;
    default:
      throw(std::logic_error("Unknown paper size units"));
  }
}

size_t PrintParameters::getPaperSizeHInPixels() const
{
  switch(paperSizeUnits)
  {
    case Pixels:
      return paperSizeH;
    case Millimeters:
      return floor(paperSizeH * hwResH / MM_PER_IN);
    case Inches:
      return paperSizeH * hwResH;
    default:
      throw(std::logic_error("Unknown paper size units"));
  }
}

size_t PrintParameters::getPaperSizeInPixels() const
{
  return getPaperSizeWInPixels() * getPaperSizeHInPixels();
}

double PrintParameters::getPaperSizeWInMillimeters() const
{
  switch(paperSizeUnits)
  {
    case Pixels:
      return round2((paperSizeW / hwResW) * MM_PER_IN);
    case Millimeters:
      return paperSizeW;
    case Inches:
      return round2(paperSizeW * MM_PER_IN);
    default:
      throw(std::logic_error("Unknown paper size units"));
  }
}

double PrintParameters::getPaperSizeHInMillimeters() const
{
  switch(paperSizeUnits)
  {
    case Pixels:
      return round2((paperSizeH / hwResH) * MM_PER_IN);
    case Millimeters:
      return paperSizeH;
    case Inches:
      return round2(paperSizeH * MM_PER_IN);
    default:
      throw(std::logic_error("Unknown paper size units"));
  }
}

double PrintParameters::getPaperSizeWInPoints() const
{
  switch(paperSizeUnits)
  {
    case Pixels:
      return round2((paperSizeW / hwResW) * PTS_PER_IN);
    case Millimeters:
      return round2((paperSizeW / MM_PER_IN) * PTS_PER_IN);
    case Inches:
      return paperSizeW * PTS_PER_IN;
    default:
      throw(std::logic_error("Unknown paper size units"));
  }
}

double PrintParameters::getPaperSizeHInPoints() const
{
  switch(paperSizeUnits)
  {
    case Pixels:
      return round2((paperSizeH / hwResH) * PTS_PER_IN);
    case Millimeters:
      return round2((paperSizeH / MM_PER_IN) * PTS_PER_IN);
    case Inches:
      return paperSizeH * PTS_PER_IN;
    default:
      throw(std::logic_error("Unknown paper size units"));
  }
}

size_t PrintParameters::getPaperSizeWInBytes() const
{
  if(getBitsPerColor() == 1 && getNumberOfColors() == 1)
  {
    // Round up to whole bytes
    return (getPaperSizeWInPixels() + 7) / 8;
  }
  else if(getBitsPerColor() % 8 == 0)
  {
    return getPaperSizeWInPixels() * getNumberOfColors() * (getBitsPerColor() / 8);
  }
  else
  {
    throw(std::logic_error("Unhandled color and bit depth combination"));
  }
}

size_t PrintParameters::getPaperSizeInBytes() const
{
  return getPaperSizeWInBytes() * getPaperSizeHInPixels();
}

PageSequence PrintParameters::getPageSequence(size_t pages) const
{
  PageRangeList tmp = pageSelection.empty() ? PageRangeList {{1, pages}} : pageSelection;
  PageSequence seq;

  for(const auto& [first, last] : tmp)
  {
    for(size_t p = first; p <= std::min(last, pages); p++)
    {
      seq.push_back(p);
    }
  }
  if(copies > 1)
  {
    if(isTwoSided() && (seq.size() % 2) == 1)
    {
      seq.push_back(INVALID_PAGE);
    }

    if(collatedCopies)
    {
      PageSequence copy = seq;
      for(size_t dc = copies; dc > 1; dc--)
      {
        seq += copy;
      }
    }
    else
    {
      PageSequence copy;
      for(PageSequence::const_iterator it = seq.cbegin(); it != seq.cend(); it++)
      {
        if(isTwoSided())
        {
          size_t front = *it;
          size_t back = *(++it);
          for(size_t pc = copies; pc > 0; pc--)
          {
            copy.push_back(front);
            copy.push_back(back);
          }
        }
        else
        {
          size_t page = *it;
          for(size_t pc = copies; pc > 0; pc--)
          {
            copy.push_back(page);
          }
        }
      }
      seq = copy;
    }
  }
  return seq;
}

PageRangeList PrintParameters::parsePageSelection(const std::string& pageSelectionStr)
{
  PageRangeList rangeList;
  const std::regex single("^([0-9]+)$");
  const std::regex range("^([0-9]+)-([0-9]*)$");
  std::smatch match;
  size_t prevMax = 0;

  List<std::string> rangeStrList = split_string(pageSelectionStr, ",");
  for(const std::string& rangeStr : rangeStrList)
  {
    if(std::regex_match(rangeStr, match, single))
    {
      size_t singleValue = stoul(match[1]);
      if(singleValue <= prevMax)
      {
        return {};
      }
      prevMax = singleValue;
      rangeList.push_back({singleValue, singleValue});
    }
    else if(std::regex_match(rangeStr, match, range))
    {
      size_t from = stoul(match[1]);
      size_t to = match[2] != "" ? stoul(match[2]) : std::numeric_limits<size_t>::max();
      if(to < from || from <= prevMax)
      {
        return {};
      }
      prevMax = to;
      rangeList.push_back({from, to});
    }
    else
    {
      return {};
    }
  }
  return rangeList;
}

bool PrintParameters::setPageSelection(const std::string& pageSelectionStr)
{
  pageSelection = parsePageSelection(pageSelectionStr);
  return !pageSelection.empty();
}

double from_string(const std::string& str)
{
  size_t dotAt = str.find('.');
  double res = std::stoul(str.substr(0, dotAt));
  if(dotAt != std::string::npos)
  {
    std::string decimalString = str.substr(dotAt+1);
    res += (std::stoul(decimalString) / (pow(10, decimalString.length())));
  }
  return res;
}

bool PrintParameters::setPaperSize(const std::string& sizeStr)
{
  const std::regex nameRegex("^[0-9a-z_-]+_([0-9]+([.][0-9]+)?)x([0-9]+([.][0-9]+)?)(mm|in)$");
  std::smatch match;

  if(std::regex_match(sizeStr, match, nameRegex))
  {
    paperSizeName = sizeStr;
    paperSizeW = from_string(match[1]);
    paperSizeH = from_string(match[3]);
    if(match[5] == "in")
    {
      paperSizeUnits = Inches;
    }
    else
    {
      paperSizeUnits = Millimeters;
    }
    return true;
  }
  return false;
}

bool PrintParameters::isBlack() const
{
  switch(colorMode)
  {
    case sRGB24:
    case CMYK32:
    case Gray8:
    case Gray1:
    case sRGB48:
    case Gray16:
      return false;
    case Black8:
    case Black1:
      return true;
    default:
      throw(std::logic_error("Unknown color mode"));
  }
}

size_t PrintParameters::getNumberOfColors() const
{
  switch(colorMode)
  {
    case sRGB24:
    case sRGB48:
      return 3;
    case CMYK32:
      return 4;
    case Gray8:
    case Black8:
    case Gray1:
    case Black1:
    case Gray16:
      return 1;
    default:
      throw(std::logic_error("Unknown color mode"));
  }
}

size_t PrintParameters::getBitsPerColor() const
{
  switch(colorMode)
  {
    case sRGB24:
    case CMYK32:
    case Gray8:
    case Black8:
      return 8;
    case Gray1:
    case Black1:
      return 1;
    case sRGB48:
    case Gray16:
      return 16;
    default:
      throw(std::logic_error("Unknown color mode"));
  }
}

bool PrintParameters::getBackHFlip() const
{
  if(duplexMode == TwoSidedLongEdge)
  {
    return backXformMode == Rotated;
  }
  else if(duplexMode == TwoSidedShortEdge)
  {
    return backXformMode == Flipped || backXformMode == ManualTumble;
  }
  return false;
}

bool PrintParameters::getBackVFlip() const
{
  if(duplexMode == TwoSidedLongEdge)
  {
    return backXformMode == Flipped || backXformMode == Rotated;
  }
  else if(duplexMode == TwoSidedShortEdge)
  {
    return backXformMode == ManualTumble;
  }
  return false;
}
