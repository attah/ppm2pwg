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

void bmp_to_pwg(Bytestream& bmp_bts, Bytestream& OutBts, bool Urf,
                size_t page, size_t Colors, size_t Quality,
                size_t HwResX, size_t HwResY, size_t ResX, size_t ResY,
                bool Duplex, bool Tumble, std::string PageSizeName,
                bool BackHFlip, bool BackVFlip)
{
  Bytestream bmp_line;
  Bytestream current;
  bool backside = (page%2)==0;

  std::cerr << "Page " << page << std::endl;

  if(!Urf)
  {
    make_pwg_hdr(OutBts, Colors, Quality, HwResX, HwResY, ResX, ResY,
                 Duplex, Tumble, PageSizeName,
                 backside&&BackHFlip, backside&&BackVFlip);
  }
  else
  {
    make_urf_hdr(OutBts, Colors, Quality, HwResX, HwResY, ResX, ResY,
                 Duplex, Tumble);
  }

  size_t bytesPerLine = Colors*ResX;
  uint8_t* raw = bmp_bts.raw();

  #define pos_fun std::function<uint8_t*(size_t)>
  pos_fun pos = backside&&BackVFlip
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

    if(backside&&BackHFlip)
    {
      bmp_line.reset();
      for(int i = bytesPerLine-Colors; i >= 0; i -= Colors)
      {
        bmp_line.putBytes(pos(y)+i, Colors);
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
      bmp_line/Colors >> current;

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
          bmp_line/Colors >> current;
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
          bmp_line/Colors >> current;
          OutBts << (uint8_t)0 << current;
        }
        else
        { // 2 or more non-repeating sequnces
          bmp_line.setPos(current_start);
          OutBts << (uint8_t)(257-verbatim);
          bmp_line.getBytes(OutBts, verbatim*Colors);
        }
      }
    }
  }
}

void make_pwg_hdr(Bytestream& OutBts, size_t Colors, size_t Quality,
                  size_t HwResX, size_t HwResY, size_t ResX, size_t ResY,
                  bool Duplex, bool Tumble, std::string PageSizeName,
                  bool HFlip, bool VFlip)
{
  PwgPgHdr OutHdr;

  OutHdr.Duplex = Duplex;
  OutHdr.HWResolutionX = HwResX;
  OutHdr.HWResolutionY = HwResY;
  OutHdr.NumCopies = 1;
  OutHdr.PageSizeX = ResX * 72.0 / OutHdr.HWResolutionX; // width in pt, WTF?!
  OutHdr.PageSizeY = ResY * 72.0 / OutHdr.HWResolutionY; // height in pt, WTF?!
  OutHdr.Tumble = Tumble;
  OutHdr.Width = ResX;
  OutHdr.Height = ResY;
  OutHdr.BitsPerColor = 8;
  OutHdr.BitsPerPixel = Colors * OutHdr.BitsPerColor;
  OutHdr.BytesPerLine = Colors * ResX;
  OutHdr.ColorSpace = Colors==3 ? PwgPgHdr::sRGB : PwgPgHdr::sGray;
  OutHdr.NumColors = Colors;
  OutHdr.TotalPageCount = 0;
  OutHdr.CrossFeedTransform = HFlip ? -1 : 1;
  OutHdr.FeedTransform = VFlip ? -1 : 1;
  OutHdr.AlternatePrimary = pow(2, OutHdr.BitsPerPixel)-1;
  OutHdr.PrintQuality = (Quality == 3 ? PwgPgHdr::Draft
                      : (Quality == 4 ? PwgPgHdr::Normal
                      : (Quality == 5 ? PwgPgHdr::High
                      : PwgPgHdr::DefaultPrintQuality)));
  OutHdr.PageSizeName = PageSizeName;

  std::cerr << OutHdr.describe() << std::endl;

  OutHdr.encode_into(OutBts);
}

void make_urf_hdr(Bytestream& OutBts, size_t Colors, size_t Quality,
                  size_t HwResX, size_t HwResY,size_t ResX, size_t ResY,
                  bool Duplex, bool Tumble)
{
  if(HwResX != HwResY)
  {
    exit(2);
  }

  UrfPgHdr OutHdr;

  OutHdr.BitsPerPixel = 8*Colors;
  OutHdr.ColorSpace = Colors==3 ? UrfPgHdr::sRGB : UrfPgHdr::sGray;
  OutHdr.Duplex = Duplex ? (Tumble ? UrfPgHdr::ShortSide : UrfPgHdr::LongSide)
                         : UrfPgHdr::NoDuplex;
  OutHdr.Quality = (Quality == 3 ? UrfPgHdr::Draft
                 : (Quality == 4 ? UrfPgHdr::Normal
                 : (Quality == 5 ? UrfPgHdr::High
                 : UrfPgHdr::DefaultQuality)));
  OutHdr.Width = ResX;
  OutHdr.Height = ResY;
  OutHdr.HWRes = HwResX;

  std::cerr << OutHdr.describe() << std::endl;

  OutHdr.encode_into(OutBts);
}
