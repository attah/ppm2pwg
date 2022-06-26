#include <iostream>
#include <string.h>

#include "ppm2pwg.h"
#include "PwgPgHdr.h"
#include "UrfPgHdr.h"
#include "reversebytes.h"

Bytestream make_pwg_file_hdr()
{
  Bytestream PwgFileHdr;
  PwgFileHdr << "RaS2";
  return PwgFileHdr;
}

Bytestream make_urf_file_hdr(uint32_t pages)
{
  Bytestream UrfFileHdr;
  UrfFileHdr << "UNIRAST" << (uint8_t)0 << pages;
  return UrfFileHdr;
}

void bmp_to_pwg(Bytestream& bmp_bts, Bytestream& OutBts,
                size_t page, const PrintParameters& Params, bool Verbose)
{
  bool backside = Params.duplex && ((page%2)==0);

  if(Verbose)
  {
    std::cerr << "Page " << page << std::endl;
  }

  if(!(Params.format == PrintParameters::URF))
  {
    make_pwg_hdr(OutBts, Params, backside, Verbose);
  }
  else
  {
    make_urf_hdr(OutBts, Params, Verbose);
  }

  size_t ResY = Params.getPaperSizeHInPixels();
  uint8_t* raw = bmp_bts.raw();
  size_t bytesPerLine = Params.getPaperSizeWInBytes();
  int step = backside&&Params.backVFlip ? -bytesPerLine : bytesPerLine;
  uint8_t* row0 = backside&&Params.backVFlip ? raw+(ResY-1)*bytesPerLine : raw;
  uint8_t* tmp_line = new uint8_t[bytesPerLine];

  for(size_t y=0; y<ResY; y++)
  {
    uint8_t* this_line = row0+y*step;
    uint8_t line_repeat = 0;

    uint8_t* next_line = this_line + step;
    while((y+1)<ResY && memcmp(this_line, next_line, bytesPerLine) == 0)
    {
      y++;
      next_line += step;
      line_repeat++;
      if(line_repeat == 255)
      {
        break;
      }
    }

    OutBts << line_repeat;
    if(backside&&Params.backHFlip)
    {
      // Flip line into tmp buffer
      if(Params.bitsPerColor == 1)
      {
        for(size_t i = 0; i < bytesPerLine; i++)
        {
          tmp_line[i] = reverse_bytes[this_line[bytesPerLine-1-i]];
        }
      }
      else
      {
        for(size_t i = 0; i < bytesPerLine; i += Params.colors)
        {
          memcpy(tmp_line+i, this_line+bytesPerLine-Params.colors-i, Params.colors);
        }
      }
      compress_line(tmp_line, bytesPerLine, OutBts, Params.colors);
    }
    else
    {
      compress_line(this_line, bytesPerLine, OutBts, Params.colors);
    }
  }
  delete[] tmp_line;
}

void compress_line(uint8_t* raw, size_t len, Bytestream& OutBts, int Colors)
{
  uint8_t* current;
  uint8_t* pos = raw;
  uint8_t* epos = raw+len;
  while(pos != epos)
  {
    uint8_t* current_start = pos;
    current = pos;
    pos += Colors;

    if(pos == epos || memcmp(pos, current, Colors) == 0)
    {
      int8_t repeat = 0;
      // Find number of repititions
      while(pos != epos && memcmp(pos, current, Colors) == 0)
      {
        pos += Colors;
        repeat++;
        if(repeat == 127)
        {
          break;
        }
      }
      OutBts << repeat;
      OutBts.putBytes(current, Colors);
    }
    else
    {
      size_t verbatim = 1;
      // Find the number of byte sequences that are different
      // (we know the first one is)
      do
      {
        current = pos;
        pos += Colors;
        verbatim++;
        if(verbatim == 127)
        {
          break;
        }
      }
      while(pos != epos && memcmp(pos, current, Colors) != 0);

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
        pos = current_start + Colors;
        OutBts << (uint8_t)0;
        OutBts.putBytes(current_start, Colors);
      }
      else
      { // 2 or more non-repeating sequnces
        pos = current_start + verbatim*Colors;
        OutBts << (uint8_t)(257-verbatim);
        OutBts.putBytes(current_start, verbatim*Colors);
      }
    }
  }
}

void make_pwg_hdr(Bytestream& OutBts, const PrintParameters& Params, bool backside, bool Verbose)
{
  PwgPgHdr OutHdr;

  OutHdr.Duplex = Params.duplex;
  OutHdr.HWResolutionX = Params.hwResW;
  OutHdr.HWResolutionY = Params.hwResH;
  OutHdr.NumCopies = 1;
  OutHdr.PageSizeX = round(Params.getPaperSizeWInPoints());
  OutHdr.PageSizeY = round(Params.getPaperSizeHInPoints());
  OutHdr.Tumble = Params.tumble;
  OutHdr.Width = Params.getPaperSizeWInPixels();
  OutHdr.Height = Params.getPaperSizeHInPixels();
  OutHdr.BitsPerColor = Params.bitsPerColor;
  OutHdr.BitsPerPixel = Params.colors * OutHdr.BitsPerColor;
  OutHdr.BytesPerLine = Params.getPaperSizeWInBytes();
  OutHdr.ColorSpace = Params.colors == 4 ? PwgPgHdr::CMYK
                    : Params.colors == 3 ? PwgPgHdr::sRGB
                    : Params.black ? PwgPgHdr::Black
                    : PwgPgHdr::sGray;
  OutHdr.NumColors = Params.colors;
  OutHdr.TotalPageCount = 0;
  OutHdr.CrossFeedTransform = backside&&Params.backHFlip ? -1 : 1;
  OutHdr.FeedTransform = backside&&Params.backVFlip ? -1 : 1;
  OutHdr.AlternatePrimary = 0x00ffffff;
  OutHdr.PrintQuality = (Params.quality == 3 ? PwgPgHdr::Draft
                      : (Params.quality == 4 ? PwgPgHdr::Normal
                      : (Params.quality == 5 ? PwgPgHdr::High
                      : PwgPgHdr::DefaultPrintQuality)));
  OutHdr.PageSizeName = Params.paperSizeName;

  if(Verbose)
  {
    std::cerr << OutHdr.describe() << std::endl;
  }

  OutHdr.encode_into(OutBts);
}

void make_urf_hdr(Bytestream& OutBts, const PrintParameters& Params, bool Verbose)
{
  if(Params.hwResW != Params.hwResH)
  {
    exit(2);
  }

  UrfPgHdr OutHdr;

  OutHdr.BitsPerPixel = 8*Params.colors;
  OutHdr.ColorSpace = Params.colors==4 ? UrfPgHdr::CMYK
                    : Params.colors==3 ? UrfPgHdr::sRGB
                    : UrfPgHdr::sGray;
  OutHdr.Duplex = Params.duplex ? (Params.tumble ? UrfPgHdr::ShortSide : UrfPgHdr::LongSide)
                         : UrfPgHdr::NoDuplex;
  OutHdr.Quality = (Params.quality == 3 ? UrfPgHdr::Draft
                 : (Params.quality == 4 ? UrfPgHdr::Normal
                 : (Params.quality == 5 ? UrfPgHdr::High
                 : UrfPgHdr::DefaultQuality)));
  OutHdr.Width = Params.getPaperSizeWInPixels();
  OutHdr.Height = Params.getPaperSizeHInPixels();
  OutHdr.HWRes = Params.hwResW;

  if(Verbose)
  {
    std::cerr << OutHdr.describe() << std::endl;
  }

  OutHdr.encode_into(OutBts);
}
