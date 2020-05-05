#include <bytestream.h>
#include <fstream>
#include <iostream>
#include <string>
#include <cerrno>
#include <iostream>
#include <string.h>

#define CODABLE_FILE "pwg_pghdr_codable.h"
#include <codable.h>
#define CODABLE_FILE "urf_pghdr_codable.h"
#include <codable.h>

void make_pwg_hdr(Bytestream& OutBts, size_t HwResX, size_t HwResY,
                  size_t ResX, size_t ResY, size_t Colors);
void make_urf_hdr(Bytestream& OutBts, size_t HwResX, size_t HwResY,
                  size_t ResX, size_t ResY, size_t Colors);

#ifndef PPM2PWG_MAIN
  #define PPM2PWG_MAIN main
#endif

int PPM2PWG_MAIN(int, char**)
{

  bool apple = false;
  size_t HwResX = 300;
  size_t HwResY = 300;

  char* URF = getenv("URF");
  if(URF && strcmp(URF,"false")!=0)
  {
    apple = true;
  }

  char* Prepend = getenv("PREPEND_FILE");
  if(Prepend)
  {
    std::ifstream PrependFile(Prepend);
    if(!PrependFile)
      return 1;
    std::cout << PrependFile.rdbuf();
  }

  char* HWRES_X = getenv("HWRES_X");
  char* HWRES_Y = getenv("HWRES_Y");
  if(HWRES_X)
  {
    HwResX = atoi(HWRES_X);
  }
  if(HWRES_Y)
  {
    HwResY = atoi(HWRES_Y);
  }

  if(!apple)
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

    if(!apple)
    {
      make_pwg_hdr(OutBts, HwResX, HwResY, ResX, ResY, Colors);
    }
    else
    {
      make_urf_hdr(OutBts, HwResX, HwResY, ResX, ResY, Colors);
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

      // std::cerr << "line: " << y << "\n" << std::flush;

      while(bmp_line.remaining())
      {
        Bytestream current;
        size_t current_start = bmp_line.pos();
        bmp_line/Colors >> current;

        if(bmp_line/Colors >>= current)
        {
          // std::cerr << "Repeats\n";
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

          // std::cerr << y << ": "<< (int)repeat << " times\n" << std::flush;

          enc_line << repeat << current;
        }
        else
        {
          // std::cerr << "Verbatim\n" << std::flush;

          size_t verbatim = 1;
          while(bmp_line.remaining() && bmp_line.peekBytestream(Colors) != current)
          {
            bmp_line += Colors;
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


void make_pwg_hdr(Bytestream& OutBts, size_t HwResX, size_t HwResY,
                  size_t ResX, size_t ResY, size_t Colors)
{
  PwgPgHdr OutHdr;

  OutHdr.Duplex = 1;
  OutHdr.HWResolutionX = HwResX;
  OutHdr.HWResolutionY = HwResY;
  OutHdr.NumCopies = 1;
  OutHdr.PageSizeX = ResX * 72.0 / OutHdr.HWResolutionX; // width in pt, WTF?!
  OutHdr.PageSizeY = ResY * 72.0 / OutHdr.HWResolutionY; // height in pt, WTF?!

  OutHdr.Width = ResX;
  OutHdr.Height = ResY;
  OutHdr.BitsPerColor = 8;
  OutHdr.BitsPerPixel = 8*Colors;
  OutHdr.BytesPerLine = Colors * ResX;
  OutHdr.ColorOrder = 0;
  OutHdr.ColorSpace = Colors==3 ? 19 : 18; // CUPS srgb : sgray
  OutHdr.NumColors = Colors;
  OutHdr.TotalPageCount = 0;
  OutHdr.CrossFeedTransform = 1;
  OutHdr.FeedTransform = 1;
  // OutHdr.AlternatePrimary = 16777215;
  OutHdr.PageSizeName = "iso_a4_210x297mm";

  std::cerr << OutHdr.describe() << std::endl;

  OutHdr.encode_into(OutBts);
}

void make_urf_hdr(Bytestream& OutBts, size_t HwResX, size_t HwResY,
                  size_t ResX, size_t ResY, size_t Colors)
{
  if(HwResX != HwResY)
  {
    exit(2);
  }

  UrfPgHdr OutHdr;

  OutHdr.BitsPerPixel = 8*Colors;
  OutHdr.ColorSpace = Colors==3 ? 1 : 4;
  OutHdr.Duplex = 1;
  OutHdr.Quality = 4;
  OutHdr.Width = ResX;
  OutHdr.Height = ResY;
  OutHdr.HWRes = HwResX;

  std::cerr << OutHdr.describe() << std::endl;

  OutHdr.encode_into(OutBts);
}
