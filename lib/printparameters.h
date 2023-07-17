#ifndef PRINTPARAMETERS_H
#define PRINTPARAMETERS_H

#include <cmath>
#include <vector>
#include <string>

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

  enum ColorMode
  {
    sRGB24,
    CMYK32,
    Gray8,
    Black8,
    Gray1,
    Black1
  };

  enum Quality
  {
    DefaultQuality = 0,
    DraftQuality = 3,
    NormalQuality = 4,
    HighQuality = 5
  };

  enum BackXformMode
  {
    Normal,
    Rotated,
    Flipped,
    ManualTumble
  };

  Format format = PDF;

  ColorMode colorMode = sRGB24;
  Quality quality = DefaultQuality;
  bool antiAlias = false;
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

  enum DuplexMode
  {
    OneSided = 0,
    TwoSidedLongEdge,
    TwoSidedShortEdge
  };

  std::string mediaType;
  std::string mediaPosition;

  DuplexMode duplexMode = OneSided;

  bool isTwoSided() const;

  BackXformMode backXformMode = Normal;

  size_t copies = 1;
  bool collatedCopies = true;

  PageRangeList pageRangeList;

  size_t getPaperSizeWInPixels() const;
  size_t getPaperSizeHInPixels() const;

  size_t getPaperSizeInPixels() const;

  float getPaperSizeWInMillimeters() const;
  float getPaperSizeHInMillimeters() const;

  float getPaperSizeWInPoints() const;
  float getPaperSizeHInPoints() const;

  size_t getPaperSizeWInBytes() const;
  size_t getPaperSizeInBytes() const;

  PageSequence getPageSequence(size_t pages) const;

  bool setPageRange(const std::string& rangeStr);

  bool setPaperSize(const std::string& sizeStr);

  bool isBlack() const;
  size_t getNumberOfColors() const;
  size_t getBitsPerColor() const;

  bool getBackHFlip() const;
  bool getBackVFlip() const;

private:

  double round2(double d) const
  {
    return round(d*100)/100;
  }

};

#endif //PRINTPARAMETERS_H
