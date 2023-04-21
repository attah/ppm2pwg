#include <iostream>
#include <fstream>
#include <cstring>

#include "pdf2printable.h"
#include "argget.h"

#define HELPTEXT "Options from 'resolution' and onwards only affect raster output formats."

inline void print_error(std::string hint, std::string argHelp)
{
  std::cerr << hint << std::endl << std::endl << argHelp << std::endl << HELPTEXT << std::endl;
}

inline bool ends_with(std::string s, std::string ending)
{
  if(ending.length() <= s.length())
  {
    return s.substr(s.length()-ending.length(), ending.length()) == ending;
  }
  return false;
}

int main(int argc, char** argv)
{
  PrintParameters params;
  bool help = false;
  bool verbose = false;
  std::string format;
  std::string pages;
  std::string paperSize = params.paperSizeName;
  std::string colorMode;
  int hwRes = 0;
  int hwResX = 0;
  int hwResY = 0;
  std::string infile;
  std::string outfile;

  SwitchArg<bool> helpOpt(help, {"-h", "--help"}, "Print this help text");
  SwitchArg<bool> verboseOpt(verbose, {"-v", "--verbose"}, "Be verbose, print headers and progress");
  SwitchArg<std::string> formatOpt(format, {"-f", "--format"}, "Format to output (pdf/postscript/pwg/urf)");
  SwitchArg<std::string> pagesOpt(pages, {"--pages"}, "What pages to process, e.g.: 1,17-42");
  SwitchArg<size_t> copiesOpt(params.documentCopies, {"--copies"}, "Number of copies to output");
  SwitchArg<size_t> pageCopiesOpt(params.pageCopies, {"--page-copies"}, "Number of copies to output for each page");
  SwitchArg<std::string> paperSizeOpt(paperSize, {"--paper-size"}, "Paper size to output, e.g.: iso_a4_210x297mm");
  SwitchArg<int> resolutionOpt(hwRes, {"-r", "--resolution"}, "Resolution (in DPI) for rasterization");
  SwitchArg<int> resolutionXOpt(hwResX, {"-rx", "--resolution-x"}, "Resolution (in DPI) for rasterization, x-axis");
  SwitchArg<int> resolutionYOpt(hwResY, {"-ry", "--resolution-y"}, "Resolution (in DPI) for rasterization, y-axis");
  SwitchArg<bool> duplexOpt(params.duplex, {"-d", "--duplex"}, "Process for duplex printing");
  SwitchArg<bool> tumbleOpt(params.duplex, {"-t", "--tumble"}, "Set tumble indicator in raster header");
  SwitchArg<bool> hFlipOpt(params.backHFlip, {"-hf", "--hflip"}, "Flip backsides horizontally for duplex");
  SwitchArg<bool> vFlipOpt(params.backVFlip, {"-vf", "--vflip"}, "Flip backsides vertically for duplex");
  SwitchArg<std::string> colorModeOpt(colorMode, {"-c", "--color-mode"}, "Color mode (srgb24/cmyk32/gray8/black8/gray1/black1)");
  SwitchArg<size_t> qualityOpt(params.quality, {"-q", "--quality"}, "Quality setting in raster header (3,4,5)");
  SwitchArg<bool> antiAliasOpt(params.antiAlias, {"-aa", "--antaialias"}, "Enable antialiasing in rasterization");

  PosArg pdfArg(infile, "PDF-file");
  PosArg outArg(outfile, "out-file");

  ArgGet args({&helpOpt, &verboseOpt, &formatOpt, &pagesOpt,
               &copiesOpt, &pageCopiesOpt, &paperSizeOpt, &resolutionOpt,
               &resolutionXOpt, &resolutionYOpt, &duplexOpt, &tumbleOpt,
               &hFlipOpt, &vFlipOpt, &colorModeOpt, &qualityOpt, &antiAliasOpt},
              {&pdfArg, &outArg});

  bool correctArgs = args.get_args(argc, argv);
  if(help)
  {
    std::cout << args.argHelp() << std::endl << HELPTEXT << std::endl;
    return 0;
  }
  else if(!correctArgs)
  {
    print_error(args.errmsg(), args.argHelp());
    return 1;
  }

  if(format == "ps" || format == "postscript" || (format == "" && ends_with(outfile, ".ps")))
  {
    params.format = PrintParameters::Postscript;
  }
  else if(format == "pwg" || (format == "" && ends_with(outfile, ".pwg")))
  {
    params.format = PrintParameters::PWG;
  }
  else if(format == "urf" || (format == "" && ends_with(outfile, ".urf")))
  {
    params.format = PrintParameters::URF;
  }
  else if(format == "pdf" || (format == "" && ends_with(outfile, ".pdf")))
  {
    params.format = PrintParameters::PDF;
  }
  else if(format != "")
  {
    print_error("Unrecognized target format", args.argHelp());
    return 1;
  }

  if(!pages.empty())
  {
    if(!params.setPageRange(pages))
    {
      print_error("Malformed page selection", args.argHelp());
      return 1;
    }
  }

  if(!params.setPaperSize(paperSize))
  {
    print_error("Malformed paper size", args.argHelp());
    return 1;
  }

  if(hwResX != 0)
  {
    params.hwResW = hwResX;
  }
  else if(hwRes != 0)
  {
    params.hwResW = hwRes;
  }

  if(hwResY != 0)
  {
    params.hwResH = hwResY;
  }
  else if(hwRes != 0)
  {
    params.hwResH = hwRes;
  }

  if(colorMode != "" && !params.setColorMode(colorMode))
  {
    print_error("Unrecognized color mode", args.argHelp());
  }

  if(params.format == PrintParameters::URF && (params.bitsPerColor == 1 || params.black))
  {
    print_error("URF does not support black or 1-bit color modes", args.argHelp());
    return 1;
  }

  std::ofstream of = std::ofstream(outfile, std::ofstream::out | std::ios::binary);
  WriteFun writeFun([&of](unsigned char const* buf, unsigned int len) -> bool
           {
             of.write((const char*)buf, len);
             return of.exceptions() == std::ostream::goodbit;
           });

  if(verbose)
  {
    ProgressFun progressFun([](size_t page, size_t total) -> void
                {
                  std::cerr << "Progress: " << page << "/" << total << "\n\n";
                });
    return pdf_to_printable(infile, writeFun, params, progressFun, true);
  }
  else
  {
    return pdf_to_printable(infile, writeFun, params);
  }

}
