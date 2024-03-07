#include <iostream>
#include <map>
#include <string.h>

#include <array.h>
#include "ppm2pwg.h"
#include "PwgPgHdr.h"
#include "UrfPgHdr.h"
#include "log.h"

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
  int step = backside&&params.getBackVFlip() ? -bytesPerLine : bytesPerLine;
  uint8_t* row0 = backside&&params.getBackVFlip() ? raw+(yRes-1)*bytesPerLine : raw;
  Array<uint8_t> tmpLine(bytesPerLine);

  for(size_t y=0; y<yRes; y++)
  {
    uint8_t* thisLine = row0+y*step;
    uint8_t lineRepeat = 0;
    size_t colors = params.getNumberOfColors();

    uint8_t* next_line = thisLine + step;
    while((y+1)<yRes && memcmp(thisLine, next_line, bytesPerLine) == 0)
    {
      y++;
      next_line += step;
      lineRepeat++;
      if(lineRepeat == 255)
      {
        break;
      }
    }

    outBts << lineRepeat;
    if(backside&&params.getBackHFlip())
    {
      // Flip line into tmp buffer
      if(params.getBitsPerColor() == 1)
      {
        for(size_t i = 0; i < bytesPerLine; i++)
        {
          // https://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64Bits
          tmpLine[i] = ((thisLine[bytesPerLine-1-i] * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32;
        }
      }
      else
      {
        for(size_t i = 0; i < bytesPerLine; i += colors)
        {
          memcpy(tmpLine+i, thisLine+bytesPerLine-colors-i, colors);
        }
      }
      compress_line(tmpLine, bytesPerLine, outBts, colors);
    }
    else
    {
      compress_line(thisLine, bytesPerLine, outBts, colors);
    }
  }
}

void compress_line(uint8_t* raw, size_t len, Bytestream& outBts, int colors)
{
  uint8_t* current;
  uint8_t* pos = raw;
  uint8_t* epos = raw+len;
  while(pos != epos)
  {
    uint8_t* currentStart = pos;
    current = pos;
    pos += colors;

    if(pos == epos || memcmp(pos, current, colors) == 0)
    {
      int8_t repeat = 0;
      // Find number of repititions
      while(pos != epos && memcmp(pos, current, colors) == 0)
      {
        pos += colors;
        repeat++;
        if(repeat == 127)
        {
          break;
        }
      }
      outBts << repeat;
      outBts.putBytes(current, colors);
    }
    else
    {
      size_t verbatim = 1;
      // Find the number of byte sequences that are different
      // (we know the first one is)
      do
      {
        current = pos;
        pos += colors;
        verbatim++;
        if(verbatim == 127)
        {
          break;
        }
      }
      while(pos != epos && memcmp(pos, current, colors) != 0);

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
        pos = currentStart + colors;
        outBts << (uint8_t)0;
        outBts.putBytes(currentStart, colors);
      }
      else
      { // 2 or more non-repeating sequnces
        pos = currentStart + verbatim*colors;
        outBts << (uint8_t)(257-verbatim);
        outBts.putBytes(currentStart, verbatim*colors);
      }
    }
  }
}

static std::map<std::string, UrfPgHdr::MediaType_enum>
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

bool isUrfMediaType(std::string mediaType)
{
  return UrfMediaTypeMappings.find(mediaType) != UrfMediaTypeMappings.cend();
}

void make_pwg_hdr(Bytestream& outBts, const PrintParameters& params, bool backside)
{
  PwgPgHdr outHdr;

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
  outHdr.ColorSpace = params.colorMode == PrintParameters::CMYK32 ? PwgPgHdr::CMYK
                    : params.colorMode == PrintParameters::sRGB24 ? PwgPgHdr::sRGB
                    : params.isBlack() ? PwgPgHdr::Black
                    : PwgPgHdr::sGray;
  outHdr.NumColors = params.getNumberOfColors();
  outHdr.TotalPageCount = 0;
  outHdr.CrossFeedTransform = backside&&params.getBackHFlip() ? -1 : 1;
  outHdr.FeedTransform = backside&&params.getBackVFlip() ? -1 : 1;
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
    exit(2);
  }

  UrfPgHdr outHdr;

  outHdr.BitsPerPixel = 8*params.getNumberOfColors();
  outHdr.ColorSpace = params.colorMode == PrintParameters::CMYK32 ? UrfPgHdr::CMYK
                    : params.colorMode == PrintParameters::sRGB24 ? UrfPgHdr::sRGB
                    : UrfPgHdr::sGray;
  outHdr.Duplex = params.isTwoSided() ? (params.duplexMode == PrintParameters::TwoSidedShortEdge
                                             ? UrfPgHdr::ShortSide
                                             : UrfPgHdr::LongSide)
                                      : UrfPgHdr::NoDuplex;
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
