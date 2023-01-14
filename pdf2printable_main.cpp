#include <iostream>
#include <fstream>
#include <cstring>

#include "pdf2printable.h"
#include "argget.h"

#define HELPTEXT "Options from 'resolution' and onwards only affect raster output formats."

void print_error(std::string hint, std::string arghelp)
{
  std::cerr << hint << std::endl << std::endl << arghelp << std::endl << HELPTEXT << std::endl;
}

bool ends_with(std::string s, std::string ending)
{
  if(ending.length() <= s.length())
  {
    return s.substr(s.length()-ending.length(), ending.length()) == ending;
  }
  return false;
}

int main(int argc, char** argv)
{
  PrintParameters Params;
  bool help = false;
  bool verbose = false;
  std::string format;
  std::string pages;
  std::string paperSize = Params.paperSizeName;
  std::string colorMode;
  int hwRes = 0;
  int hwResX = 0;
  int hwResY = 0;
  std::string Infile;
  std::string Outfile;

  SwitchArg<bool> helpOpt(help, {"-h", "--help"}, "Print this help text");
  SwitchArg<bool> verboseOpt(verbose, {"-v", "--verbose"}, "Be verbose, print headers and progress");
  SwitchArg<std::string> formatOpt(format, {"-f", "--format"}, "Format to output (pdf/postscript/pwg/urf)");
  SwitchArg<std::string> pagesOpt(pages, {"--pages"}, "What pages to process, e.g.: 1,17-42");
  SwitchArg<size_t> copiesOpt(Params.documentCopies, {"--copies"}, "Number of copies to output");
  SwitchArg<size_t> pageCopiesOpt(Params.pageCopies, {"--page-copies"}, "Number of copies to output for each page");
  SwitchArg<std::string> paperSizeOpt(paperSize, {"--paper-size"}, "Paper size to output, e.g.: iso_a4_210x297mm");
  SwitchArg<int> resolutionOpt(hwRes, {"-r", "--resolution"}, "Resolution (in DPI) for rasterization");
  SwitchArg<int> resolutionXOpt(hwResX, {"-rx", "--resolution-x"}, "Resolution (in DPI) for rasterization, x-axis");
  SwitchArg<int> resolutionYOpt(hwResY, {"-ry", "--resolution-y"}, "Resolution (in DPI) for rasterization, y-axis");
  SwitchArg<bool> duplexOpt(Params.duplex, {"-d", "--duplex"}, "Process for duplex printing");
  SwitchArg<bool> tumbleOpt(Params.duplex, {"-t", "--tumble"}, "Set tumble indicator in raster header");
  SwitchArg<bool> hFlipOpt(Params.backHFlip, {"-hf", "--hflip"}, "Flip backsides horizontally for duplex");
  SwitchArg<bool> vFlipOpt(Params.backVFlip, {"-vf", "--vflip"}, "Flip backsides vertically for duplex");
  SwitchArg<std::string> colorModeOpt(colorMode, {"-c", "--color-mode"}, "Color mode (srgb24/cmyk32/gray8/black8/gray1/black1)");
  SwitchArg<size_t> qualityOpt(Params.quality, {"-q", "--quality"}, "Quality setting in raster header (3,4,5)");
  SwitchArg<bool> antiAliasOpt(Params.antiAlias, {"-aa", "--antaialias"}, "Enable antialiasing in rasterization");

  PosArg pdfArg(Infile, "PDF-file");
  PosArg outArg(Outfile, "out-file");

  ArgGet args({&helpOpt, &verboseOpt, &formatOpt, &pagesOpt,
               &copiesOpt, &pageCopiesOpt, &paperSizeOpt, &resolutionOpt,
               &resolutionXOpt, &resolutionYOpt, &duplexOpt, &tumbleOpt,
               &hFlipOpt, &vFlipOpt, &colorModeOpt, &qualityOpt, &antiAliasOpt},
              {&pdfArg, &outArg});

  bool correctArgs = args.get_args(argc, argv);
  if(help)
  {
    std::cout << args.arghelp() << std::endl << HELPTEXT << std::endl;
    return 0;
  }
  else if(!correctArgs)
  {
    print_error(args.errmsg(), args.arghelp());
    return 1;
  }

  if(format == "ps" || format == "postscript" || (format == "" && ends_with(Outfile, ".ps")))
  {
    Params.format = PrintParameters::Postscript;
  }
  else if(format == "pwg" || (format == "" && ends_with(Outfile, ".pwg")))
  {
    Params.format = PrintParameters::PWG;
  }
  else if(format == "urf" || (format == "" && ends_with(Outfile, ".urf")))
  {
    Params.format = PrintParameters::URF;
  }
  else if(format == "pdf" || (format == "" && ends_with(Outfile, ".pdf")))
  {
    Params.format = PrintParameters::PDF;
  }
  else if(format != "")
  {
    print_error("Unrecognized target format", args.arghelp());
    return 1;
  }

  if(!pages.empty())
  {
    if(!Params.setPageRange(pages))
    {
      print_error("Malformed page selection", args.arghelp());
      return 1;
    }
  }

  if(!Params.setPaperSize(paperSize))
  {
    print_error("Malformed paper size", args.arghelp());
    return 1;
  }

  if(hwResX != 0)
  {
    Params.hwResW = hwResX;
  }
  else if(hwRes != 0)
  {
    Params.hwResW = hwRes;
  }

  if(hwResY != 0)
  {
    Params.hwResH = hwResY;
  }
  else if(hwRes != 0)
  {
    Params.hwResH = hwRes;
  }

  if(colorMode != "" && !Params.setColorMode(colorMode))
  {
    print_error("Unrecognized color mode", args.arghelp());
  }

  if(Params.format == PrintParameters::URF && (Params.bitsPerColor == 1 || Params.black))
  {
    print_error("URF does not support black or 1-bit color modes", args.arghelp());
    return 1;
  }

  std::ofstream of = std::ofstream(Outfile, std::ofstream::out | std::ios::binary);
  write_fun WriteFun([&of](unsigned char const* buf, unsigned int len) -> bool
            {
              of.write((char*)buf, len);
              return of.exceptions() == std::ostream::goodbit;
            });

  if(verbose)
  {
    progress_fun ProgressFun([](size_t page, size_t total) -> void
                 {
                   std::cerr << "Progress: " << page << "/" << total << "\n\n";
                 });
    return pdf_to_printable(Infile, WriteFun, Params, ProgressFun, true);
  }
  else
  {
    return pdf_to_printable(Infile, WriteFun, Params);
  }

}
