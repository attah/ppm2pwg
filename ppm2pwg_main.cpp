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
  bool ForcePortrait = getenv_bool("FORCE_PORTRAIT");

  Params.hwResW = getenv_int("HWRES_X", Params.hwResW);
  Params.hwResH = getenv_int("HWRES_Y", Params.hwResH);
  Params.quality = getenv_int("QUALITY", Params.quality);

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
      Params.colors = 3;
    }
    else if(p == "P5")
    {
      Params.colors = 1;
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
    if(bmp_bts.size() != Params.colors*ResX*ResY)
    {
      bmp_bts = Bytestream(Params.colors*ResX*ResY);
    }

    if(ForcePortrait && (ResY < ResX))
    {
      rotate_tmp.initFrom(std::cin, Params.colors*ResX*ResY);

      for(size_t y=1; y<(ResY+1); y++)
      {
        for(size_t x=1; x<(ResX+1); x++)
        {
          rotate_tmp.getBytes(bmp_bts.raw()+((x*ResY)-y)*Params.colors, Params.colors);
        }
      }
      std::swap(ResX, ResY);
      bmp_bts.setPos(0);
    }
    else
    {
      bmp_bts.initFrom(std::cin, Params.colors*ResX*ResY);
    }

    Params.paperSizeW = ResX;
    Params.paperSizeH = ResY;
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
