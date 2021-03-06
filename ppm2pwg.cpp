#include <bytestream.h>
#include <fstream>
#include <iostream>
#include <string>
#include <cerrno>
#include <cmath>
#include <limits>
#include <string.h>
#include <functional>

#include "PwgPgHdr.h"
#include "UrfPgHdr.h"

void make_pwg_hdr(Bytestream& OutBts, size_t Colors, size_t Quality,
                  size_t HwResX, size_t HwResY, size_t ResX, size_t ResY,
                  bool Duplex, bool Tumble, std::string PageSizeName,
                  bool HFlip, bool VFlip);
void make_urf_hdr(Bytestream& OutBts, size_t Colors, size_t Quality,
                  size_t HwResX, size_t HwResY,size_t ResX, size_t ResY,
                  bool Duplex, bool Tumble);

bool getenv_bool(std::string VarName);
int getenv_int(std::string VarName, int Default);
std::string getenv_str(std::string VarName, std::string Default);

#ifndef PPM2PWG_MAIN
  #define PPM2PWG_MAIN main
#endif

int PPM2PWG_MAIN(int, char**)
{

  bool Urf = getenv_bool("URF");
  bool Duplex = getenv_bool("DUPLEX");
  bool Tumble = getenv_bool("TUMBLE");
  bool ForcePortrait = getenv_bool("FORCE_PORTRAIT");

  size_t HwResX = getenv_int("HWRES_X", 300);
  size_t HwResY = getenv_int("HWRES_Y", 300);
  size_t Quality = getenv_int("QUALITY", 4);

  bool BackVFlip = getenv_bool("BACK_VFLIP");
  bool BackHFlip = getenv_bool("BACK_HFLIP");

  std::string PageSizeName = getenv_str("PAGE_SIZE_NAME", "iso_a4_210x297mm");

  if(!Urf)
  {
    std::cout << "RaS2";
  }
  else
  {
    Bytestream UrfFileHdr;
    uint32_t pages = getenv_int("PAGES", 1);
    UrfFileHdr << "UNIRAST" << (uint8_t)0 << pages;
    std::cout << UrfFileHdr;
  }

  size_t page = 0;

  size_t Colors = 0;

  Bytestream rotate_tmp;
  Bytestream OutBts;
  Bytestream bmp_line;
  Bytestream bmp_bts;
  Bytestream current;

  while(!std::cin.eof())
  {
    OutBts.reset();

    std::cerr << "Page " << ++page << std::endl;

    std::string p, xs, ys, r;
    std::cin >> p;

    if(std::cin.peek() == '\n')
    {
      std::cin.ignore(1);
    }
    if(std::cin.peek() == '#')
    {
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    std::cin >> xs >> ys >> r;

    if(std::cin.peek() == '\n')
    {
      std::cin.ignore(1);
    }

    if(p == "P6")
    {
      Colors = 3;
    }
    else if(p == "P5")
    {
      Colors = 1;
    }
    else
    {
      std::cerr << "Only P5/P6 ppms supported, for now " << p << std::endl;
      return 1;
    }

    std::cerr << p << " " << xs << "x" << ys << " " << r << std::endl;

    unsigned int ResX = stoi(xs);
    unsigned int ResY = stoi(ys);

    // If not the correct size (for this page), reallocate it
    if(bmp_bts.size() != Colors*ResX*ResY)
    {
      bmp_bts = Bytestream(Colors*ResX*ResY);
    }

    if(ForcePortrait && (ResY < ResX))
    {
      rotate_tmp.initFrom(std::cin, Colors*ResX*ResY);

      for(size_t y=1; y<(ResY+1); y++)
      {
        for(size_t x=1; x<(ResX+1); x++)
        {
          rotate_tmp.getBytes(bmp_bts.raw()+((x*ResY)-y)*Colors, Colors);
        }
      }
      std::swap(ResX, ResY);
      bmp_bts.setPos(0);
    }
    else
    {
      bmp_bts.initFrom(std::cin, Colors*ResX*ResY);
    }

    bool backside = (page%2)==0;

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

    std::cout << OutBts;
    std::cin.peek(); // maybe trigger eof
  }
  return 0;
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

bool getenv_bool(std::string VarName)
{
  char* tmp = getenv(VarName.c_str());
  return (tmp && strcmp(tmp,"0")!=0 && strcmp(tmp,"false")!=0);
}

int getenv_int(std::string VarName, int Default)
{
  char* tmp = getenv(VarName.c_str());
  return tmp ? atoi(tmp) : Default;
}

std::string getenv_str(std::string VarName, std::string Default)
{
  char* tmp = getenv(VarName.c_str());
  return tmp ? tmp : Default;
}
