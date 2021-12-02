#include <iostream>
#include <fstream>
#include <cstring>

#include "pwgpapersizes.h"
#include "pdf2printable.h"

bool getenv_bool(std::string VarName);
int getenv_int(std::string VarName, int Default);
std::string getenv_str(std::string VarName, std::string Default);

int main(int argc, char** argv)
{
  if(argc != 3)
  {
    std::cerr << "Usage: pdf2printable <PDF-file> <outfile>" << std::endl;
    return 1;
  }

  std::string Infile(argv[1]);
  std::string Outfile(argv[2]);

  Format TargetFormat = PDF;

  size_t HwResX = getenv_int("HWRES_X", getenv_int("HWRES", 300));
  size_t HwResY = getenv_int("HWRES_Y", getenv_int("HWRES", 300));
  std::string PaperSizeName = getenv_str("PAPER_SIZE", "iso_a4_210x297mm");

  std::pair<float, float> PaperSize = PwgPaperSizes.at(PaperSizeName);
  float PaperSizeX = PaperSize.first;
  float PaperSizeY = PaperSize.second;

  size_t FromPage = getenv_int("FROM_PAGE", 0);
  size_t ToPage = getenv_int("TO_PAGE", 0);

  bool Duplex = getenv_bool("DUPLEX");
  bool Tumble = getenv_bool("TUMBLE");
  bool BackHFlip = getenv_bool("BACK_HFLIP");
  bool BackVFlip = getenv_bool("BACK_VFLIP");
  size_t Colors = getenv_int("COLORS", 3);
  size_t Quality = getenv_int("QUALITY", 4);

  std::string format = getenv_str("FORMAT", "pdf");
  if(format == "ps" || format == "postscript")
  {
    TargetFormat = Postscript;
  }
  else if(format == "pwg")
  {
    TargetFormat = PWG;
  }
  else if(format == "urf")
  {
    TargetFormat = URF;
  }
  else if(format != "pdf")
  {
    return 1;
  }

  std::ofstream of = std::ofstream(Outfile, std::ofstream::out);
  write_fun WriteFun([&of](unsigned char const* buf, unsigned int len) -> bool
            {
              of.write((char*)buf, len);
              return of.exceptions() == std::ostream::goodbit;
            });

  progress_fun ProgressFun([](size_t page, size_t total) -> void
            {
              std::cerr << "Progress: " << page << "/" << total << "\n\n";
            });

  return pdf_to_printable(Infile, WriteFun, Colors, Quality,
                          PaperSizeName, PaperSizeX, PaperSizeY,
                          HwResX, HwResY, TargetFormat,
                          Duplex, Tumble, BackHFlip, BackVFlip,
                          FromPage, ToPage, ProgressFun, true);
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
