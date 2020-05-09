#include <bytestream.h>
#include <fstream>
#include <iostream>
#include <string>
#include <cerrno>
#include <cmath>
#include <iostream>
#include <string.h>

#define CODABLE_FILE "pwg_pghdr_codable.h"
#include <codable.h>
#define CODABLE_FILE "urf_pghdr_codable.h"
#include <codable.h>

void make_pwg_hdr(Bytestream& Bts, size_t Colors, size_t HwResX, size_t HwResY,
                  size_t ResX, size_t ResY, bool Duplex, bool Tumble);
void make_urf_hdr(Bytestream& Bts, size_t Colors, size_t HwResX, size_t HwResY,
                  size_t ResX, size_t ResY, bool Duplex, bool Tumble);

bool getenv_bool(std::string VarName);
int getenv_int(std::string VarName, int Default);

#ifndef PPM2PWG_MAIN
  #define PPM2PWG_MAIN main
#endif

int PPM2PWG_MAIN(int, char**)
{

  bool Urf = getenv_bool("URF");
  bool Duplex = getenv_bool("DUPLEX");
  bool Tumble = getenv_bool("TUMBLE");

  size_t HwResX = getenv_int("HWRES_X", 300);
  size_t HwResY = getenv_int("HWRES_Y", 300);

  char* Prepend = getenv("PREPEND_FILE");
  if(Prepend)
  {
    std::ifstream PrependFile(Prepend);
    if(!PrependFile)
      return 1;
    std::cout << PrependFile.rdbuf();
  }

  if(!Urf)
  {
    std::cout << "RaS2";
  }
  else
  {
    Bytestream UrfFileHdr;
    UrfFileHdr << "UNIRAST" << (uint8_t)0 << (uint32_t)1;
    std::cout.write((char*)UrfFileHdr.raw(), UrfFileHdr.size());
  }

  size_t r = 0;

  size_t Colors = 0;

  while(!std::cin.eof())
  {
    Bytestream OutBts;

    std::cerr << "Round " << r++ << std::endl;

    std::string p, xs, ys, r;
    std::cin >> p >> xs >> ys >> r;

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

    std::cerr << r << " " << std::cin.peek() << std::endl;

    while(std::cin.peek() == 10)
    {
      std::cin.ignore(1);
    }

    std::cerr << p << " " << xs << "x" << ys << " " << r << std::endl;

    unsigned int ResX = stoi(xs);
    unsigned int ResY = stoi(ys);

    if(!Urf)
    {
      make_pwg_hdr(OutBts, Colors, HwResX, HwResY, ResX, ResY, Duplex, Tumble);
    }
    else
    {
      make_urf_hdr(OutBts, Colors, HwResX, HwResY, ResX, ResY, Duplex, Tumble);
    }

    std::cerr << "Exp " << Colors*ResX*ResY << std::endl;

    Bytestream bmp_bts(Colors*ResX*ResY);

    std::cin.read((char*)bmp_bts.raw(), Colors*ResX*ResY);

    for(size_t y=0; y<ResY; y++)
    {
      Bytestream bmp_line;
      bmp_bts/(Colors*ResX) >> bmp_line;
      uint8_t line_repeat = 0;
      Bytestream enc_line;

      while(bmp_line.remaining())
      {
        Bytestream current;
        size_t current_start = bmp_line.pos();
        bmp_line/Colors >> current;

        if(bmp_line/Colors >>= current)
        {
          int8_t repeat = 1;
          // Find number of repititions
          while(bmp_line >>= current)
          {
            repeat++;
            if(repeat == 127)
            {
              break;
            }
          }
          enc_line << repeat << current;
        }
        else
        {
          size_t verbatim = 1;
          // Find the number of byte sequences that are different
          while(bmp_line.nextBytestream(current, false))
          {
            verbatim++;
            if(verbatim == 127)
            {
              break;
            }
          }
          bmp_line.setPos(current_start);
          Bytestream tmp_bts;
          bmp_line/(verbatim*Colors) >> tmp_bts;
          enc_line << (uint8_t)(257-verbatim) << tmp_bts;
        }
      }

      while((y<ResY) && (bmp_bts/(Colors*ResX) >>= bmp_line))
      {
        y++;
        line_repeat++;
        if(line_repeat == 255)
        {
          break;
        }
      }
      OutBts << line_repeat << enc_line;
    }

    std::cout.write((char*)OutBts.raw(), OutBts.size());
    std::cerr << "remaining: " << bmp_bts.remaining() << "\n";
    std::cin.peek(); // maybe trigger eof
  }
  return 0;
}


void make_pwg_hdr(Bytestream& Bts, size_t Colors, size_t HwResX, size_t HwResY,
                  size_t ResX, size_t ResY, bool Duplex, bool Tumble)
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
  OutHdr.ColorOrder = 0;
  OutHdr.ColorSpace = Colors==3 ? 19 : 18; // CUPS srgb : sgray
  OutHdr.NumColors = Colors;
  OutHdr.TotalPageCount = 0;
  OutHdr.CrossFeedTransform = 1;
  OutHdr.FeedTransform = 1;
  OutHdr.AlternatePrimary = pow(2, OutHdr.BitsPerPixel)-1;
  OutHdr.PageSizeName = "iso_a4_210x297mm";

  std::cerr << OutHdr.describe() << std::endl;

  OutHdr.encode_into(Bts);
}

void make_urf_hdr(Bytestream& Bts, size_t Colors, size_t HwResX, size_t HwResY,
                  size_t ResX, size_t ResY, bool Duplex, bool Tumble)
{
  if(HwResX != HwResY)
  {
    exit(2);
  }

  UrfPgHdr OutHdr;

  OutHdr.BitsPerPixel = 8*Colors;
  OutHdr.ColorSpace = Colors==3 ? 1 : 4;
  OutHdr.Duplex = Duplex ? Tumble ? 3 : 2 : 1; // 1: no duplex, 2: long side, 3: short side
  OutHdr.Quality = 4;
  OutHdr.Width = ResX;
  OutHdr.Height = ResY;
  OutHdr.HWRes = HwResX;

  std::cerr << OutHdr.describe() << std::endl;

  OutHdr.encode_into(Bts);
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
