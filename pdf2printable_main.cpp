#include <iostream>
#include <fstream>
#include <cstring>

#include "pdf2printable.h"
#include "argget.h"

int main(int argc, char** argv)
{
  PrintParameters Params;
  bool help = false;
  std::string format;
  std::string pages;
  std::string paperSize = Params.paperSizeName;
  int hwRes = 0;
  int hwResX = 0;
  int hwResY = 0;
  std::string Infile;
  std::string Outfile;

  SwitchArg<bool> helpOpt(help, {"-h", "--help"}, "Print this help text");
  SwitchArg<std::string> formatOpt(format, {"-f", "--format"}, "Format to output (pdf/postscript/pwg/urf)");
  SwitchArg<std::string> pagesOpt(pages, {"--pages"}, "What pages to process, e.g.: 1,17-42");
  SwitchArg<std::string> paperSizeOpt(paperSize, {"--paper-size"}, "Paper size to output, e.g.: iso_a4_210x297mm");
  SwitchArg<int> resolutionOpt(hwRes, {"-r", "--resolution"}, "Resolution (in DPI) for rasterization");
  SwitchArg<int> resolutionXOpt(hwResX, {"-rx", "--resolution-x"}, "Resolution (in DPI) for rasterization, x-axis");
  SwitchArg<int> resolutionYOpt(hwResY, {"-ry", "--resolution-y"}, "Resolution (in DPI) for rasterization, y-axis");
  SwitchArg<bool> duplexOpt(Params.duplex, {"-d", "--duplex"}, "Process for duplex printing");
  SwitchArg<bool> tumbleOpt(Params.duplex, {"-t", "--tumble"}, "Set tumble indicator in raster header");
  SwitchArg<bool> hFlipOpt(Params.backHFlip, {"-hf", "--hflip"}, "Flip backsides horizontally for duplex");
  SwitchArg<bool> vFlipOpt(Params.backVFlip, {"-vf", "--vflip"}, "Flip backsides vertically for duplex");
  SwitchArg<size_t> colorOpt(Params.colors, {"-c", "--colors"}, "Number of colors. 1:greyscale/mono 3:RGB 4:CMYK");
  SwitchArg<size_t> bitsPerColorOpt(Params.bitsPerColor, {"-bpc", "--bits-per-color"}, "Number of bits per color (1 or 8)");
  SwitchArg<bool> blackOpt(Params.black, {"-b", "--black"}, "Use more-color-is-black for raster format");
  SwitchArg<size_t> qualityOpt(Params.quality, {"-q", "--quality"}, "Quality setting in raster header (3,4,5)");
  SwitchArg<bool> antiAliasOpt(Params.antiAlias, {"-aa", "--antaialias"}, "Use antialiasing for raster conversion");
  SwitchArg<size_t> copiesOpt(Params.documentCopies, {"--copies"}, "Number of copies to output");
  SwitchArg<size_t> pageCopiesOpt(Params.pageCopies, {"--page-copies"}, "Number of copies to output for each page");

  PosArg pdfArg(Infile, "PDF-file");
  PosArg outArg(Outfile, "out-file");

  ArgGet args({&helpOpt, &formatOpt, &pagesOpt, &paperSizeOpt, &resolutionOpt,
               &resolutionXOpt, &resolutionYOpt, &duplexOpt, &tumbleOpt,
               &hFlipOpt, &vFlipOpt, &colorOpt, &bitsPerColorOpt, &blackOpt,
               &qualityOpt, &antiAliasOpt, &copiesOpt, &pageCopiesOpt},
              {&pdfArg, &outArg});

  bool correctArgs = args.get_args(argc, argv);
  if(help)
  {
    std::cout << args.arghelp() << std::endl;
    return 0;
  }
  else if(!correctArgs)
  {
    std::cerr << args.errmsg() << std::endl << std::endl << args.arghelp() << std::endl;
    return 1;
  }

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
  else if(format != "" && format != "pdf")
  {
    std::cerr << "Unrecognized target format" << std::endl;
    return 1;
  }

  if(!pages.empty())
  {
    if(!Params.setPageRange(pages))
    {
      std::cerr << "Malformed page selection" << std::endl;
      return 1;
    }
  }

  if(!Params.setPaperSize(paperSize))
  {
    std::cerr << "Malformed paper size" << std::endl;
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
