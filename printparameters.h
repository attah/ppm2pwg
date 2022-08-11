#ifndef PRINTPARAMETERS_H
#define PRINTPARAMETERS_H

#include <cmath>
#include <string>
#include <vector>
#include <regex>

#define MM_PER_IN 25.4
#define PTS_PER_IN 72.0

#define INVALID_PAGE 0

typedef std::vector<size_t> PageSequence;
typedef std::vector<std::pair<size_t, size_t>> PageRangeList;

class PrintParameters
{
public:

  enum Format
  {
    PDF,
    Postscript,
    PWG,
    URF,
    Invalid
  };

  Format format = PDF;

  size_t colors = 3; // RGB
  size_t bitsPerColor = 8;
  // More color is black, like for black_1, black_8.
  // False for regular modes like sRGB and sGray.
  bool black = false;
  size_t quality = 0; // Default
  std::string paperSizeName = "iso_a4_210x297mm";

  size_t hwResW = 300;
  size_t hwResH = 300;

  enum PaperSizeUnits
  {
    Pixels,
    Millimeters,
    Inches
    // Feet
    // Arms
    // Legs
  };

  PaperSizeUnits paperSizeUnits = Millimeters;
  float paperSizeW = 210; // A4
  float paperSizeH = 297; // A4

  bool duplex = false;
  bool tumble = false;

  bool backHFlip = false;
  bool backVFlip = false;

  size_t documentCopies = 1;
  size_t pageCopies = 1;

  PageRangeList pageRangeList;

  size_t getPaperSizeWInPixels() const
  {
    switch(paperSizeUnits)
    {
      case Pixels:
        return paperSizeW;
      case Millimeters:
        return paperSizeW * (hwResW / MM_PER_IN);
      case Inches:
        return paperSizeW * hwResW;
    }
    return 0;
  }

  size_t getPaperSizeHInPixels() const
  {
    switch(paperSizeUnits)
    {
      case Pixels:
        return paperSizeH;
      case Millimeters:
        return round(paperSizeH * (hwResH / MM_PER_IN));
      case Inches:
        return paperSizeH * hwResH;
    }
    return 0;
  }

  size_t getPaperSizeInPixels() const
  {
    return getPaperSizeWInPixels() * getPaperSizeHInPixels();
  }

  float getPaperSizeWInPoints() const
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

  float getPaperSizeHInPoints() const
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

  size_t getPaperSizeWInBytes() const
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

  size_t getPaperSizeInBytes() const
  {
    return getPaperSizeWInBytes() * getPaperSizeHInPixels();
  }

  PageSequence getPageSequence(size_t pages) const
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
      for(size_t p = r.first; p <= r.second; p++)
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

  bool setPageRange(const std::string& rangeStr)
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

  bool setPaperSize(const std::string& sizeStr)
  {
    const std::regex nameRegex("^[0-9a-z_-]+_(([0-9]+[.])?[0-9]+)x(([0-9]+[.])?[0-9]+)(mm|in)$");
    std::smatch match;

    if(std::regex_match(sizeStr, match, nameRegex))
    {
      paperSizeW = stof(match[1]);
      paperSizeH = stof(match[3]);
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

private:

  double round2(double d) const
  {
    return round(d*100)/100;
  }

};

#endif //PRINTPARAMETERS_H
