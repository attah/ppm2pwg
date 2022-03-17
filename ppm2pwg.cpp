#include <bytestream.h>
#include <iostream>
#include <string>
#include <cmath>
#include <string.h>
#include <functional>

#include "ppm2pwg.h"
#include "PwgPgHdr.h"
#include "UrfPgHdr.h"

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
                size_t page, PrintParameters Params, bool Verbose)
{
  Bytestream bmp_line;
  Bytestream current;
  bool backside = (page%2)==0;

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

  size_t bytesPerLine = Params.colors*Params.getPaperSizeWInPixels();
  size_t ResY = Params.getPaperSizeHInPixels();
  uint8_t* raw = bmp_bts.raw();

  #define pos_fun std::function<uint8_t*(size_t)>
  pos_fun pos = backside&&Params.backVFlip
              ? pos_fun([raw, ResY, bytesPerLine](size_t y)
                {
                  return raw+(ResY-1-y)*bytesPerLine;
                })
              : pos_fun([raw, bytesPerLine](size_t y)
                {
                  return raw+y*bytesPerLine;
                });

  for(size_t y=0; y<ResY; y++)
  {
    uint8_t line_repeat = 0;

    if(backside&&Params.backHFlip)
    {
      bmp_line.reset();
      for(int i = bytesPerLine-Params.colors; i >= 0; i -= Params.colors)
      {
        bmp_line.putBytes(pos(y)+i, Params.colors);
      }
    }
    else
    {
      bmp_line.initFrom(pos(y), bytesPerLine);
    }

    while((y+1)<ResY && memcmp(pos(y), pos(y+1), bytesPerLine) == 0)
    {
      y++;
      line_repeat++;
      if(line_repeat == 255)
      {
        break;
      }
    }

    OutBts << line_repeat;

    while(bmp_line.remaining())
    {
      size_t current_start = bmp_line.pos();
      bmp_line/Params.colors >> current;

      if(bmp_line.atEnd() || bmp_line.peekNextBytestream(current))
      {
        int8_t repeat = 0;
        // Find number of repititions
        while(bmp_line >>= current)
        {
          repeat++;
          if(repeat == 127)
          {
            break;
          }
        }
        OutBts << repeat << current;
      }
      else
      {
        size_t verbatim = 1;
        // Find the number of byte sequences that are different
        // (we know the first one is)
        do
        {
          bmp_line/Params.colors >> current;
          verbatim++;
          if(verbatim == 127)
          {
            break;
          }
        }
        while(!bmp_line.atEnd() && !bmp_line.peekNextBytestream(current));

        // This and the next sequence are equal,
        // assume it starts a repeating sequence.
        // (unless we are at the end)
        if(!bmp_line.atEnd())
        {
          verbatim--;
        }

        if(verbatim == 1)
        { // We ended up with one sequence, encode it as such
          bmp_line.setPos(current_start);
          bmp_line/Params.colors >> current;
          OutBts << (uint8_t)0 << current;
        }
        else
        { // 2 or more non-repeating sequnces
          bmp_line.setPos(current_start);
          OutBts << (uint8_t)(257-verbatim);
          bmp_line.getBytes(OutBts, verbatim*Params.colors);
        }
      }
    }
  }
}

void make_pwg_hdr(Bytestream& OutBts, PrintParameters Params, bool backside, bool Verbose)
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
  OutHdr.BitsPerColor = 8;
  OutHdr.BitsPerPixel = Params.colors * OutHdr.BitsPerColor;
  OutHdr.BytesPerLine = Params.colors * OutHdr.Width;
  OutHdr.ColorSpace = Params.colors==3 ? PwgPgHdr::sRGB : PwgPgHdr::sGray;
  OutHdr.NumColors = Params.colors;
  OutHdr.TotalPageCount = 0;
  OutHdr.CrossFeedTransform = backside&&Params.backHFlip ? -1 : 1;
  OutHdr.FeedTransform = backside&&Params.backVFlip ? -1 : 1;
  OutHdr.AlternatePrimary = pow(2, OutHdr.BitsPerPixel)-1;
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

void make_urf_hdr(Bytestream& OutBts, PrintParameters Params, bool Verbose)
{
  if(Params.hwResW != Params.hwResH)
  {
    exit(2);
  }

  UrfPgHdr OutHdr;

  OutHdr.BitsPerPixel = 8*Params.colors;
  OutHdr.ColorSpace = Params.colors==3 ? UrfPgHdr::sRGB : UrfPgHdr::sGray;
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
