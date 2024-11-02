#ifndef PRINTPARAMETERS_H
#define PRINTPARAMETERS_H

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

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

  enum MediaPosition
  {
    AutomaticMediaPosition = 0,
    Main,
    Alternate,
    LargeCapacity,
    Manual,
    Envelope,
    Disc,
    PhotoMediaPosition,
    Hagaki,
    MainRoll,
    AlternateRoll,
    Top,
    Middle,
    Bottom,
    Side,
    Left,
    Right,
    Center,
    Rear,
    ByPassTray,
    Tray1,
    Tray2,
    Tray3,
    Tray4,
    Tray5,
    Tray6,
    Tray7,
    Tray8,
    Tray9,
    Tray10,
    Tray11,
    Tray12,
    Tray13,
    Tray14,
    Tray15,
    Tray16,
    Tray17,
    Tray18,
    Tray19,
    Tray20,
    Roll1,
    Roll2,
    Roll3,
    Roll4,
    Roll5,
    Roll6,
    Roll7,
    Roll8,
    Roll9,
    Roll10
  };

  Format format = PDF;

  ColorMode colorMode = sRGB24;
  Quality quality = DefaultQuality;
  bool antiAlias = false;
  std::string paperSizeName = "iso_a4_210x297mm";

  uint32_t hwResW = 300;
  uint32_t hwResH = 300;

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
  double paperSizeW = 210; // A4
  double paperSizeH = 297; // A4

  enum DuplexMode
  {
    OneSided = 0,
    TwoSidedLongEdge,
    TwoSidedShortEdge
  };

  DuplexMode duplexMode = OneSided;

  bool isTwoSided() const;

  BackXformMode backXformMode = Normal;

  size_t copies = 1;
  bool collatedCopies = true;

  PageRangeList pageRangeList;

  MediaPosition mediaPosition = AutomaticMediaPosition;
  std::string mediaType;

  size_t getPaperSizeWInPixels() const;
  size_t getPaperSizeHInPixels() const;

  size_t getPaperSizeInPixels() const;

  double getPaperSizeWInMillimeters() const;
  double getPaperSizeHInMillimeters() const;

  double getPaperSizeWInPoints() const;
  double getPaperSizeHInPoints() const;

  size_t getPaperSizeWInBytes() const;
  size_t getPaperSizeInBytes() const;

  PageSequence getPageSequence(size_t pages) const;

  static PageRangeList parsePageRange(const std::string& rangeStr);
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
