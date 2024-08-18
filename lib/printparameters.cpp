#include "printparameters.h"
#include "stringutils.h"
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
  }
  return 0;
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
  }
  return 0;
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
  }
  return 0;
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
  }
  return 0;
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
  }
  return 0;
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
  }
  return 0;
}

size_t PrintParameters::getPaperSizeWInBytes() const
{
  if(getBitsPerColor() == 1)
  {
    // Round up to whole bytes
    return getNumberOfColors() * ((getPaperSizeWInPixels() + 7) / 8);
  }
  else
  {
    return getNumberOfColors() * (getPaperSizeWInPixels() / (8 / getBitsPerColor()));
  }
}

size_t PrintParameters::getPaperSizeInBytes() const
{
  return getPaperSizeWInBytes() * getPaperSizeHInPixels();
}

PageSequence PrintParameters::getPageSequence(size_t pages) const
{
  PageRangeList tmp = pageRangeList.empty() ? PageRangeList {{0, 0}}
                                            : pageRangeList;
  PageSequence seq;

  if(tmp.size() == 1)
  {
    std::pair<size_t, size_t> pair = tmp[0];
    if(pair.first == 0)
    {
      pair.first = 1;
    }
    if(pair.second == 0)
    {
      pair.second = pages;
    }
    tmp = {pair};
  }

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
        seq.insert(seq.cend(), copy.cbegin(), copy.cend());
      }
    }
    else
    {
      PageSequence copy;
      for(PageSequence::const_iterator it = seq.cbegin(); it != seq.cend(); it++)
      {
        if(isTwoSided())
        {
          for(size_t pc = copies; pc > 0; pc--)
          {
            copy.push_back(*it);
            copy.push_back(*(it+1));
          }
          it++;
        }
        else
        {
          for(size_t pc = copies; pc > 0; pc--)
          {
            copy.push_back(*it);
          }
        }
      }
      seq = copy;
    }
  }
  return seq;
}

PageRangeList PrintParameters::parsePageRange(const std::string& rangesStr)
{
  PageRangeList rangeList;
  const std::regex single("^([0-9]+)$");
  const std::regex range("^([0-9]+)-([0-9]*)$");
  std::smatch match;

  List<std::string> rangeStrList = split_string(rangesStr, ",");
  for(const std::string& rangeStr : rangeStrList)
  {
    if(std::regex_match(rangeStr, match, single))
    {
      size_t single_value = stol(match[1]);
      rangeList.push_back({single_value, single_value});
    }
    else if(std::regex_match(rangeStr, match, range))
    {
      size_t from = stol(match[1]);
      size_t to = match[2] != "" ? stol(match[2]) : std::numeric_limits<size_t>::max();
      if(to < from)
      {
        return {};
      }
      rangeList.push_back({from, to});
    }
    else
    {
      return {};
    }
  }
  return rangeList;
}

bool PrintParameters::setPageRange(const std::string& rangeStr)
{
  PageRangeList rangeList = parsePageRange(rangeStr);

  if(!rangeList.empty())
  {
    pageRangeList = rangeList;
    return true;
  }
  return false;
}

bool PrintParameters::setPaperSize(const std::string& sizeStr)
{
  const std::regex nameRegex("^[0-9a-z_-]+_(([0-9]+[.])?[0-9]+)x(([0-9]+[.])?[0-9]+)(mm|in)$");
  std::cmatch match;
  locale_t c_locale = newlocale(LC_ALL_MASK, "C", nullptr);

  if(std::regex_match(sizeStr.c_str(), match, nameRegex))
  {
    paperSizeName = sizeStr;
    paperSizeW = strtod_l(match[1].first, nullptr, c_locale);
    paperSizeH = strtod_l(match[3].first, nullptr, c_locale);
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
  switch (colorMode)
  {
    case Black8:
    case Black1:
      return true;
    case sRGB24:
    case CMYK32:
    case Gray8:
    case Gray1:
      return false;
    default:
      throw(std::logic_error("Unknown color mode"));
  }
}

size_t PrintParameters::getNumberOfColors() const
{
  switch (colorMode)
  {
    case sRGB24:
      return 3;
    case CMYK32:
      return 4;
    case Gray8:
    case Black8:
    case Gray1:
    case Black1:
      return 1;
    default:
      throw(std::logic_error("Unknown color mode"));
  }
}

size_t PrintParameters::getBitsPerColor() const
{
  switch (colorMode)
  {
    case sRGB24:
    case CMYK32:
    case Gray8:
    case Black8:
      return 8;
    case Gray1:
    case Black1:
      return 1;
    default:
      throw(std::logic_error("Unknown color mode"));
  }
}

bool PrintParameters::getBackHFlip() const
{
  if(duplexMode == TwoSidedLongEdge)
  {
    switch (backXformMode)
    {
      case Flipped:
      case ManualTumble:
      case Normal:
        return false;
      case Rotated:
        return true;
      default:
        throw(std::logic_error("Unknown back flip mode"));
    }
  }
  else if(duplexMode == TwoSidedShortEdge)
  {
    switch (backXformMode)
    {
      case Flipped:
      case ManualTumble:
        return true;
      case Normal:
      case Rotated:
        return false;
      default:
        throw(std::logic_error("Unknown back flip mode"));
    }
  }
  return false;
}

bool PrintParameters::getBackVFlip() const
{
  if(duplexMode == TwoSidedLongEdge)
  {
    switch (backXformMode)
    {
      case Flipped:
        return true;
      case ManualTumble:
      case Normal:
        return false;
      case Rotated:
        return true;
      default:
        throw(std::logic_error("Unknown back flip mode"));
    }
  }
  else if(duplexMode == TwoSidedShortEdge)
  {
    switch (backXformMode)
    {
      case Flipped:
        return false;
      case ManualTumble:
        return true;
      case Normal:
      case Rotated:
        return false;
      default:
        throw(std::logic_error("Unknown back flip mode"));
    }
  }
  return false;
}
