#include <iostream>
#include <fstream>
#include <cstring>

#include "pdf2printable.h"
#include "argget.h"

#define HELPTEXT "Options from 'resolution' and onwards only affect raster output formats.\n" \
                 "Use \"-\" as filename for stdin/stdout."

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
  std::string pages;
  std::string paperSize = params.paperSizeName;
  int hwRes = 0;
  int hwResX = 0;
  int hwResY = 0;
  std::string infile;
  std::string outfile;

  SwitchArg<bool> helpOpt(help, {"-h", "--help"}, "Print this help text");
  SwitchArg<bool> verboseOpt(verbose, {"-v", "--verbose"}, "Be verbose, print headers and progress");
  EnumSwitchArg<PrintParameters::Format> formatOpt(params.format,
                                                   {{"pdf", PrintParameters::PDF},
                                                    {"postscript", PrintParameters::Postscript},
                                                    {"ps", PrintParameters::Postscript},
                                                    {"pwg", PrintParameters::PWG},
                                                    {"urf", PrintParameters::URF}},
                                                   {"-f", "--format"},
                                                   "Format to output (pdf/postscript/pwg/urf)",
                                                   "Unrecognized target format");
  SwitchArg<std::string> pagesOpt(pages, {"--pages"}, "What pages to process, e.g.: 1,17-42");
  SwitchArg<size_t> copiesOpt(params.documentCopies, {"--copies"}, "Number of copies to output");
  SwitchArg<size_t> pageCopiesOpt(params.pageCopies, {"--page-copies"}, "Number of copies to output for each page");
  SwitchArg<std::string> paperSizeOpt(paperSize, {"--paper-size"}, "Paper size to output, e.g.: iso_a4_210x297mm");
  SwitchArg<int> resolutionOpt(hwRes, {"-r", "--resolution"}, "Resolution (in DPI) for rasterization");
  SwitchArg<int> resolutionXOpt(hwResX, {"-rx", "--resolution-x"}, "Resolution (in DPI) for rasterization, x-axis");
  SwitchArg<int> resolutionYOpt(hwResY, {"-ry", "--resolution-y"}, "Resolution (in DPI) for rasterization, y-axis");
  SwitchArg<bool> duplexOpt(params.duplex, {"-d", "--duplex"}, "Process for duplex printing");
  SwitchArg<bool> tumbleOpt(params.tumble, {"-t", "--tumble"}, "Set tumble indicator in raster header");
  SwitchArg<bool> hFlipOpt(params.backHFlip, {"-hf", "--hflip"}, "Flip backsides horizontally for duplex");
  SwitchArg<bool> vFlipOpt(params.backVFlip, {"-vf", "--vflip"}, "Flip backsides vertically for duplex");
  EnumSwitchArg<PrintParameters::ColorMode> colorModeOpt(params.colorMode,
                                                         {{"srgb24", PrintParameters::sRGB24},
                                                          {"cmyk32", PrintParameters::CMYK32},
                                                          {"gray8", PrintParameters::Gray8},
                                                          {"black8", PrintParameters::Black8},
                                                          {"gray1", PrintParameters::Gray1},
                                                          {"black1", PrintParameters::Black1}},
                                                         {"-c", "--color-mode"},
                                                         "Color mode (srgb24/cmyk32/gray8/black8/gray1/black1)",
                                                         "Unrecognized color mode");
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

  if(!formatOpt.isSet())
  {
    if(ends_with(outfile, ".ps"))
    {
      params.format = PrintParameters::Postscript;
    }
    else if(ends_with(outfile, ".pwg"))
    {
      params.format = PrintParameters::PWG;
    }
    else if(ends_with(outfile, ".urf"))
    {
      params.format = PrintParameters::URF;
    }
    else if(ends_with(outfile, ".pdf"))
    {
      params.format = PrintParameters::PDF;
    }
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

  if(params.format == PrintParameters::URF && (params.getBitsPerColor() == 1 || params.isBlack()))
  {
    print_error("URF does not support black or 1-bit color modes", args.argHelp());
    return 1;
  }

  std::ofstream ofs;
  std::ostream* out;

  if(outfile == "-")
  {
    out = &std::cout;
  }
  else
  {
    ofs = std::ofstream(outfile, std::ios::out | std::ios::binary);
    out = &ofs;
  }

  WriteFun writeFun([out](unsigned char const* buf, unsigned int len) -> bool
           {
             out->write((const char*)buf, len);
             return out->exceptions() == std::ostream::goodbit;
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
