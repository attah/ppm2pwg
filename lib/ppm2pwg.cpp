#include "ppm2pwg.h"

#include "array.h"
#include "log.h"

#include "pwgpghdr.h"
#include "urfpghdr.h"

#include <cstring>
#include <iostream>
#include <map>

void make_pwg_hdr(Bytestream& outBts, const PrintParameters& params, bool backside);
void make_urf_hdr(Bytestream& outBts, const PrintParameters& params);

void compress_line(uint8_t* raw, size_t len, Bytestream& outBts, size_t oneChunk);

Bytestream make_pwg_file_hdr()
{
  Bytestream pwgFileHdr;
  pwgFileHdr << "RaS2";
  return pwgFileHdr;
}

Bytestream make_urf_file_hdr(uint32_t pages)
{
  Bytestream urfFileHdr;
  urfFileHdr << "UNIRAST" << (uint8_t)0 << pages;
  return urfFileHdr;
}

inline uint8_t reverse_byte(uint8_t b)
{
  // https://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64Bits
  return ((b * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32;
}

void bmp_to_pwg(Bytestream& bmpBts, Bytestream& outBts, size_t page, const PrintParameters& params)
{
  bool backside = params.isTwoSided() && ((page % 2) == 0);

  DBG(<< "Page " << page);

  if(!(params.format == PrintParameters::URF))
  {
    make_pwg_hdr(outBts, params, backside);
  }
  else
  {
    make_urf_hdr(outBts, params);
  }

  size_t yRes = params.getPaperSizeHInPixels();
  uint8_t* raw = bmpBts.raw();
  size_t bytesPerLine = params.getPaperSizeWInBytes();
  int oneLine = backside && params.getBackVFlip() ? -bytesPerLine : bytesPerLine;
  uint8_t* row0 = backside && params.getBackVFlip() ? raw + ((yRes - 1) * bytesPerLine) : raw;
  Array<uint8_t> tmpLine(bytesPerLine);

  size_t colors = params.getNumberOfColors();
  size_t bpc = params.getBitsPerColor();
  // A chunk is the unit used for compression.
  // Usually this is the number of bytes per color times the number of colors,
  // but for 1-bit, compression is applied in whole bytes.
  size_t oneChunk = bpc == 1 ? colors : colors * bpc / 8;

  for(size_t y = 0; y < yRes; y++)
  {
    uint8_t* thisLine = row0 + (y * oneLine);
    uint8_t lineRepeat = 0;

    uint8_t* next_line = thisLine + oneLine;
    while((y+1)<yRes && memcmp(thisLine, next_line, bytesPerLine) == 0)
    {
      y++;
      next_line += oneLine;
      lineRepeat++;
      if(lineRepeat == 255)
      {
        break;
      }
    }

    outBts << lineRepeat;
    if(backside && params.getBackHFlip())
    {
      // Flip line into tmp buffer
      if(bpc == 1)
      {
        for(size_t i = 0; i < bytesPerLine; i++)
        {
          tmpLine[i] = reverse_byte(thisLine[bytesPerLine-1-i]);
        }
      }
      else
      {
        uint8_t* lastChunk = thisLine + bytesPerLine - oneChunk;
        for(size_t i = 0; i < bytesPerLine; i += oneChunk)
        {
          memcpy(tmpLine+i, lastChunk-i, oneChunk);
        }
      }
      compress_line(tmpLine, bytesPerLine, outBts, oneChunk);
    }
    else
    {
      compress_line(thisLine, bytesPerLine, outBts, oneChunk);
    }
  }
}

void compress_line(uint8_t* raw, size_t len, Bytestream& outBts, size_t oneChunk)
{
  uint8_t* current;
  uint8_t* pos = raw;
  uint8_t* epos = raw + len;

  while(pos != epos)
  {
    uint8_t* currentStart = pos;
    current = pos;
    pos += oneChunk;

    if(pos == epos || memcmp(pos, current, oneChunk) == 0)
    {
      int8_t repeat = 0;
      // Find number of repititions
      while(pos != epos && memcmp(pos, current, oneChunk) == 0)
      {
        pos += oneChunk;
        repeat++;
        if(repeat == 127)
        {
          break;
        }
      }
      outBts << repeat;
      outBts.putBytes(current, oneChunk);
    }
    else
    {
      size_t verbatim = 1;
      // Find the number of byte sequences that are different
      // (we know the first one is)
      do
      {
        current = pos;
        pos += oneChunk;
        verbatim++;
        if(verbatim == 127)
        {
          break;
        }
      }
      while(pos != epos && memcmp(pos, current, oneChunk) != 0);

      // This and the next sequence are equal,
      // assume it starts a repeating sequence.
      // (unless we are at the end)
      if(pos != epos)
      {
        verbatim--;
      }

      // Yes, these are similar and technically only the second case is needed.
      // But in order to not lean on that (uint8_t)(256)==0, we have this.
      if(verbatim == 1)
      { // We ended up with one sequence, encode it as such
        pos = currentStart + oneChunk;
        outBts << (uint8_t)0;
        outBts.putBytes(currentStart, oneChunk);
      }
      else
      { // 2 or more non-repeating sequnces
        pos = currentStart + verbatim * oneChunk;
        outBts << (uint8_t)(257 - verbatim);
        outBts.putBytes(currentStart, verbatim * oneChunk);
      }
    }
  }
}

static const std::map<std::string, UrfPgHdr::MediaType_enum>
  UrfMediaTypeMappings {{"auto", UrfPgHdr::AutomaticMediaType},
                        {"stationery", UrfPgHdr::Stationery},
                        {"transparency", UrfPgHdr::Transparency},
                        {"envelope", UrfPgHdr::Envelope},
                        {"cardstock", UrfPgHdr::Cardstock},
                        {"labels", UrfPgHdr::Labels},
                        {"stationery-letterhead", UrfPgHdr::StationeryLetterhead},
                        {"disc", UrfPgHdr::Disc},
                        {"photographic-matte", UrfPgHdr::PhotographicMatte},
                        {"photographic-satin", UrfPgHdr::PhotographicSatin},
                        {"photographic-semi-gloss", UrfPgHdr::PhotographicSemiGloss},
                        {"photographic-glossy", UrfPgHdr::PhotographicGlossy},
                        {"photographic-high-gloss", UrfPgHdr::PhotographicHighGloss},
                        {"other", UrfPgHdr::OtherMediaType}};

bool isUrfMediaType(const std::string& mediaType)
{
  return UrfMediaTypeMappings.find(mediaType) != UrfMediaTypeMappings.cend();
}

void make_pwg_hdr(Bytestream& outBts, const PrintParameters& params, bool backside)
{
  PwgPgHdr outHdr;

  static const std::map<PrintParameters::ColorMode, PwgPgHdr::ColorSpace_enum>
    pwgColorSpaceMappings {{PrintParameters::sRGB24, PwgPgHdr::sRGB},
                           {PrintParameters::CMYK32, PwgPgHdr::CMYK},
                           {PrintParameters::Gray8, PwgPgHdr::sGray},
                           {PrintParameters::Black8, PwgPgHdr::Black},
                           {PrintParameters::Gray1, PwgPgHdr::sGray},
                           {PrintParameters::Black1, PwgPgHdr::Black},
                           {PrintParameters::sRGB48, PwgPgHdr::sRGB},
                           {PrintParameters::Gray16, PwgPgHdr::sGray}};

  outHdr.MediaType = params.mediaType;
  outHdr.Duplex = params.isTwoSided();
  outHdr.HWResolutionX = params.hwResW;
  outHdr.HWResolutionY = params.hwResH;
  outHdr.setMediaPosition(params.mediaPosition);
  outHdr.NumCopies = 1;
  outHdr.PageSizeX = round(params.getPaperSizeWInPoints());
  outHdr.PageSizeY = round(params.getPaperSizeHInPoints());
  outHdr.Tumble = params.duplexMode == PrintParameters::TwoSidedShortEdge;
  outHdr.Width = params.getPaperSizeWInPixels();
  outHdr.Height = params.getPaperSizeHInPixels();
  outHdr.BitsPerColor = params.getBitsPerColor();
  outHdr.BitsPerPixel = params.getNumberOfColors() * outHdr.BitsPerColor;
  outHdr.BytesPerLine = params.getPaperSizeWInBytes();
  outHdr.ColorSpace = pwgColorSpaceMappings.at(params.colorMode);
  outHdr.NumColors = params.getNumberOfColors();
  outHdr.TotalPageCount = 0;
  outHdr.CrossFeedTransform = backside && params.getBackHFlip() ? -1 : 1;
  outHdr.FeedTransform = backside && params.getBackVFlip() ? -1 : 1;
  outHdr.AlternatePrimary = 0x00ffffff;
  outHdr.setPrintQuality(params.quality);
  outHdr.PageSizeName = params.paperSizeName;

  DBG(<< outHdr.describe());

  outHdr.encodeInto(outBts);
}

void make_urf_hdr(Bytestream& outBts, const PrintParameters& params)
{
  if(params.hwResW != params.hwResH)
  {
    throw std::logic_error("Asymmetric URF resolution");
  }

  UrfPgHdr outHdr;

  static const std::map<PrintParameters::ColorMode, UrfPgHdr::ColorSpace_enum>
    urfColorSpaceMappings {{PrintParameters::sRGB24, UrfPgHdr::sRGB},
                           {PrintParameters::CMYK32, UrfPgHdr::CMYK},
                           {PrintParameters::Gray8, UrfPgHdr::sGray},
                           {PrintParameters::sRGB48, UrfPgHdr::sRGB},
                           {PrintParameters::Gray16, UrfPgHdr::sGray}};

  static const std::map<PrintParameters::DuplexMode, UrfPgHdr::Duplex_enum>
    urfDuplexMappings {{PrintParameters::OneSided, UrfPgHdr::OneSided},
                       {PrintParameters::TwoSidedLongEdge, UrfPgHdr::TwoSidedLongEdge},
                       {PrintParameters::TwoSidedShortEdge, UrfPgHdr::TwoSidedShortEdge}};

  outHdr.BitsPerPixel = params.getNumberOfColors() * params.getBitsPerColor();
  outHdr.ColorSpace = urfColorSpaceMappings.at(params.colorMode);
  outHdr.Duplex = urfDuplexMappings.at(params.duplexMode);
  outHdr.setQuality(params.quality);
  outHdr.MediaType = params.mediaType == "" ? UrfPgHdr::AutomaticMediaType
                                            : UrfMediaTypeMappings.at(params.mediaType);
  outHdr.setMediaPosition(params.mediaPosition);
  outHdr.Width = params.getPaperSizeWInPixels();
  outHdr.Height = params.getPaperSizeHInPixels();
  outHdr.HWRes = params.hwResW;

  DBG(<< outHdr.describe());

  outHdr.encodeInto(outBts);
}
