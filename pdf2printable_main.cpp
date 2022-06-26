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

  PrintParameters Params;

  Params.hwResW = getenv_int("HWRES_X", getenv_int("HWRES", Params.hwResW));
  Params.hwResH = getenv_int("HWRES_Y", getenv_int("HWRES", Params.hwResH));
  Params.paperSizeName = getenv_str("PAPER_SIZE", Params.paperSizeName);

  std::pair<float, float> PaperSize = PwgPaperSizes.at(Params.paperSizeName);
  Params.paperSizeW = PaperSize.first;
  Params.paperSizeH = PaperSize.second;

  Params.fromPage = getenv_int("FROM_PAGE", Params.fromPage);
  Params.toPage = getenv_int("TO_PAGE", Params.toPage);

  Params.duplex = getenv_bool("DUPLEX");
  Params.tumble = getenv_bool("TUMBLE");
  Params.backHFlip = getenv_bool("BACK_HFLIP");
  Params.backVFlip = getenv_bool("BACK_VFLIP");
  Params.colors = getenv_int("COLORS", Params.colors);
  Params.bitsPerColor = getenv_int("BPC", Params.bitsPerColor);
  Params.black = getenv_bool("BLACK");
  Params.quality = getenv_int("QUALITY", Params.quality);

  std::string format = getenv_str("FORMAT", "pdf");
  if(format == "ps" || format == "postscript")
  {
    Params.format = PrintParameters::Postscript;
  }
  else if(format == "pwg")
  {
    Params.format = PrintParameters::PWG;
  }
  else if(format == "urf")
  {
    Params.format = PrintParameters::URF;
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

  return pdf_to_printable(Infile, WriteFun, Params, ProgressFun, true);
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
