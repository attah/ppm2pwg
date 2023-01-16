#include "printparameters.h"
#include <regex>

#define MM_PER_IN 25.4
#define PTS_PER_IN 72.0

size_t PrintParameters::getPaperSizeWInPixels() const
{
  switch(paperSizeUnits)
  {
    case Pixels:
      return paperSizeW;
    case Millimeters:
      return round(paperSizeW * hwResW / MM_PER_IN);
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
      return round(paperSizeH * hwResH / MM_PER_IN);
    case Inches:
      return paperSizeH * hwResH;
  }
  return 0;
}

size_t PrintParameters::getPaperSizeInPixels() const
{
  return getPaperSizeWInPixels() * getPaperSizeHInPixels();
}

float PrintParameters::getPaperSizeWInMillimeters() const
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

float PrintParameters::getPaperSizeHInMillimeters() const
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

float PrintParameters::getPaperSizeWInPoints() const
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

float PrintParameters::getPaperSizeHInPoints() const
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
  if(bitsPerColor == 1)
  {
    // Round up to whole bytes
    return colors * ((getPaperSizeWInPixels() + 7) / 8);
  }
  else
  {
    return colors * (getPaperSizeWInPixels() / (8 / bitsPerColor));
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

  for(const std::pair<size_t, size_t>& r : tmp)
  {
    for(size_t p = r.first; p <= std::min(r.second, pages); p++)
    {
      seq.push_back(p);
    }
  }
  if(duplex && (documentCopies > 1 || pageCopies > 1) && ((seq.size() % 2) == 1))
  {
    seq.push_back(INVALID_PAGE);
  }

  if(pageCopies > 1)
  {
    PageSequence copy;
    for(PageSequence::iterator it = seq.begin(); it != seq.end(); it++)
    {
      if(duplex)
      {
        for(size_t pc = pageCopies; pc > 0; pc--)
        {
          copy.push_back(*it);
          copy.push_back(*(it+1));
        }
        it++;
      }
      else
      {
        for(size_t pc = pageCopies; pc > 0; pc--)
        {
          copy.push_back(*it);
        }
      }
    }
    seq = copy;
  }

  if(documentCopies > 1)
  {
    PageSequence copy = seq;
    for(size_t dc = documentCopies; dc > 1; dc--)
    {
      seq.insert(seq.end(), copy.begin(), copy.end());
    }
  }
  return seq;
}

bool PrintParameters::setPageRange(const std::string& rangeStr)
{
  PageRangeList rangeList;
  const std::regex single("^([0-9]+)$");
  const std::regex range("^([0-9]+)-([0-9]+)$");
  std::smatch match;

  size_t pos = 0;
  while(pos <= rangeStr.length())
  {
    size_t found = std::min(rangeStr.length(), rangeStr.find(",", pos));
    std::string tmp(rangeStr, pos, (found-pos));

    if(std::regex_match(tmp, match, single))
    {
      size_t tmp = stol(match[1]);
      rangeList.push_back({tmp, tmp});
    }
    else if(std::regex_match(tmp, match, range))
    {
      size_t tmp_from = stol(match[1]);
      size_t tmp_to = stol(match[2]);
      if(tmp_to < tmp_from)
      {
        return false;
      }
      rangeList.push_back({tmp_from, tmp_to});
    }
    else
    {
      return false;
    }
    pos = found+1;
  }

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
  locale_t c_locale = newlocale(LC_ALL_MASK, "C", NULL);

  if(std::regex_match(sizeStr.c_str(), match, nameRegex))
  {
    paperSizeName = sizeStr;
    paperSizeW = strtof_l(match[1].first, nullptr, c_locale);
    paperSizeH = strtof_l(match[3].first, nullptr, c_locale);
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

bool PrintParameters::setColorMode(std::string colorMode)
{
  if(colorMode == "srgb24")
  {
    colors = 3;
    bitsPerColor = 8;
    black = false;
    return true;
  }
  else if(colorMode == "cmyk32")
  {
    colors = 4;
    bitsPerColor = 8;
    black = false;
    return true;
  }
  else if(colorMode == "gray8")
  {
    colors = 1;
    bitsPerColor = 8;
    black = false;
    return true;
  }
  else if(colorMode == "black8")
  {
    colors = 1;
    bitsPerColor = 8;
    black = true;
    return true;
  }
  else if(colorMode == "gray1")
  {
    colors = 1;
    bitsPerColor = 1;
    black = false;
    return true;
  }
  else if(colorMode == "black1")
  {
    colors = 1;
    bitsPerColor = 1;
    black = true;
    return true;
  }
  return false;
}
