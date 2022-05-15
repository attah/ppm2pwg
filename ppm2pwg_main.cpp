#include <bytestream.h>
#include <fstream>
#include <iostream>
#include <string>
#include <limits>
#include <string.h>

#include "ppm2pwg.h"

bool getenv_bool(std::string VarName);
int getenv_int(std::string VarName, int Default);
std::string getenv_str(std::string VarName, std::string Default);

void ignore_comments()
{
  if(std::cin.peek() == '\n')
  {
    std::cin.ignore(1);
  }
  while(std::cin.peek() == '#')
  {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
}

#ifndef PPM2PWG_MAIN
  #define PPM2PWG_MAIN main
#endif

int PPM2PWG_MAIN(int, char**)
{
  PrintParameters Params;
  Params.paperSizeUnits = PrintParameters::Pixels;

  if(getenv_bool("URF"))
  {
    Params.format = PrintParameters::URF;
  }
  else
  {
    Params.format = PrintParameters::PWG;
  }
  Params.duplex = getenv_bool("DUPLEX");
  Params.tumble = getenv_bool("TUMBLE");

  Params.hwResW = getenv_int("HWRES_X", getenv_int("HWRES", Params.hwResW));
  Params.hwResH = getenv_int("HWRES_Y", getenv_int("HWRES", Params.hwResH));
  Params.quality = getenv_int("QUALITY", Params.quality);
  Params.black = getenv_bool("BLACK");

  Params.backVFlip = getenv_bool("BACK_VFLIP");
  Params.backHFlip = getenv_bool("BACK_HFLIP");

  Params.paperSizeName = getenv_str("PAGE_SIZE_NAME", Params.paperSizeName);

  Bytestream FileHdr;

  if(Params.format == PrintParameters::URF)
  {
    uint32_t pages = getenv_int("PAGES", 1);
    FileHdr = make_urf_file_hdr(pages);
  }
  else
  {
    FileHdr = make_pwg_file_hdr();
  }

  std::cout << FileHdr;

  size_t page = 0;

  Bytestream rotate_tmp;
  Bytestream OutBts;
  Bytestream bmp_bts;

  while(!std::cin.eof())
  {
    OutBts.reset();
    page++;

    std::string p, xs, ys, r;
    std::cin >> p;

    ignore_comments();

    std::cin >> xs >> ys;

    if(p == "P6")
    {
      std::cin >> r;
      Params.colors = 3;
      Params.bitsPerColor = 8;
    }
    else if(p == "P5")
    {
      std::cin >> r;
      Params.colors = 1;
      Params.bitsPerColor = 8;
    }
    else if(p == "P4")
    {
      r = "1";
      Params.colors = 1;
      Params.bitsPerColor = 1;
      size_t x = stoi(xs);
      if(x % 8 != 0)
      {
        std::cerr << "Only whole-byte width P4 PBMs supported, got " << x << std::endl;
        return 1;
      }
    }
    else
    {
      std::cerr << "Only P4/P5/P6 (raw) supported, got " << p << std::endl;
      return 1;
    }

    ignore_comments();

    std::cerr << p << " " << xs << "x" << ys << " " << r << std::endl;

    Params.paperSizeW = stoi(xs);
    Params.paperSizeH = stoi(ys);

    size_t size = Params.paperSizeH*Params.getPaperSizeWInBytes();
    bmp_bts.initFrom(std::cin, size);

    bmp_to_pwg(bmp_bts, OutBts, page, Params, true);

    std::cout << OutBts;
    std::cin.peek(); // maybe trigger eof
  }
  return 0;
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
