#ifndef PRINTPARAMETERS_H
#define PRINTPARAMETERS_H

#include <cmath>
#include <string>

#define MM_PER_IN 25.4
#define PTS_PER_IN 72.0

struct PrintParameters
{

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

  size_t fromPage = 0; // Auto first
  size_t toPage = 0; // Auto last

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

  size_t getFromPage() const
  {
    return fromPage == 0 ? 1 : fromPage;
  }

  size_t getToPage(size_t max) const
  {
    return (toPage == 0 || toPage > max) ? max : toPage;
  }

private:
  double round2(double d) const
  {
    return round(d*100)/100;
  }

};

#endif //PRINTPARAMETERS_H
