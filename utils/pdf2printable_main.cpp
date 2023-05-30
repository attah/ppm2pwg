#include <iostream>
#include <fstream>
#include <cstring>

#include "pdf2printable.h"
#include "ppm2pwg.h"
#include "argget.h"
#include "binfile.h"
#include "mediaposition.h"

#define HELPTEXT "Options from 'resolution' and onwards only affect raster output formats.\n" \
                 "Use \"-\" as filename for stdin/stdout."

inline void print_error(std::string hint, std::string argHelp)
{
  std::cerr << hint << std::endl << std::endl << argHelp << std::endl << HELPTEXT << std::endl;
}

inline bool endsWith(std::string s, std::string ending)
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
  std::string paperSize;
  int hwRes;
  int hwResX;
  int hwResY;
  bool duplex = false;
  bool tumble = false;
  std::string inFileName;
  std::string outFileName;

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
  SwitchArg<std::string> pagesOpt(pages, {"-p", "--pages"}, "What pages to process, e.g.: 1,17-42");
  SwitchArg<size_t> copiesOpt(params.copies, {"--copies"}, "Number of copies to output");
  // This is a pretty useless option...
  // EnumSwitchArg<bool> pageCopiesOpt(params.collatedCopies,
  //                                   {{"yes", true}, {"no", false}},
  //                                   {"--collated-copies"}, "Collate copies (yes/no)");
  SwitchArg<std::string> paperSizeOpt(paperSize, {"--paper-size"}, "Paper size to output, e.g.: iso_a4_210x297mm");
  SwitchArg<int> resolutionOpt(hwRes, {"-r", "--resolution"}, "Resolution (in DPI) for rasterization");
  SwitchArg<int> resolutionXOpt(hwResX, {"-rx", "--resolution-x"}, "Resolution (in DPI) for rasterization, x-axis");
  SwitchArg<int> resolutionYOpt(hwResY, {"-ry", "--resolution-y"}, "Resolution (in DPI) for rasterization, y-axis");
  SwitchArg<bool> duplexOpt(duplex, {"-d", "--duplex"}, "Process for duplex printing");
  SwitchArg<bool> tumbleOpt(tumble, {"-t", "--tumble"}, "Process for tumbled duplex output");
  EnumSwitchArg<PrintParameters::BackXformMode> backXformOpt(params.backXformMode,
                                                             {{"rotate", PrintParameters::Rotated},
                                                              {"flip", PrintParameters::Flipped},
                                                              {"manual-tumble", PrintParameters::ManualTumble}},
                                                             {"-b", "--back-xform"},
                                                             "Transform backsides (rotate/flip/manual-tumble)");
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
  EnumSwitchArg<PrintParameters::Quality> qualityOpt(params.quality,
                                                     {{"draft", PrintParameters::DraftQuality},
                                                      {"normal", PrintParameters::NormalQuality},
                                                      {"high", PrintParameters::HighQuality}},
                                                     {"-q", "--quality"},
                                                     "Quality setting in raster header (draft/normal/high)");
  SwitchArg<bool> antiAliasOpt(params.antiAlias, {"-aa", "--antaialias"}, "Enable antialiasing in rasterization");
  EnumSwitchArg<PrintParameters::MediaPosition> mediaPositionOpt(params.mediaPosition, MEDIA_POSITION_MAP,
                                                                 {"-mp", "--media-pos"},
                                                                 "Media position, e.g.: main, top, left, roll-2 etc.");
  SwitchArg<std::string> mediaTypeOpt(params.mediaType, {"-mt", "--media-type"}, "Media type, e.g.: stationery, cardstock etc.");

  PosArg pdfArg(inFileName, "PDF-file");
  PosArg outArg(outFileName, "out-file");

  ArgGet args({&helpOpt, &verboseOpt, &formatOpt, &pagesOpt,
               &copiesOpt, /*&pageCopiesOpt,*/ &paperSizeOpt, &resolutionOpt,
               &resolutionXOpt, &resolutionYOpt, &duplexOpt, &tumbleOpt,
               &backXformOpt, &colorModeOpt, &qualityOpt, &antiAliasOpt,
               &mediaPositionOpt, &mediaTypeOpt},
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
    if(endsWith(outFileName, ".ps"))
    {
      params.format = PrintParameters::Postscript;
    }
    else if(endsWith(outFileName, ".pwg"))
    {
      params.format = PrintParameters::PWG;
    }
    else if(endsWith(outFileName, ".urf"))
    {
      params.format = PrintParameters::URF;
    }
    else if(endsWith(outFileName, ".pdf"))
    {
      params.format = PrintParameters::PDF;
    }
  }

  if(pagesOpt.isSet())
  {
    if(!params.setPageRange(pages))
    {
      print_error("Malformed page selection", args.argHelp());
      return 1;
    }
  }

  if(paperSizeOpt.isSet())
  {
    if(!params.setPaperSize(paperSize))
    {
      print_error("Malformed paper size", args.argHelp());
      return 1;
    }
  }

  if(resolutionXOpt.isSet())
  {
    params.hwResW = hwResX;
  }
  else if(resolutionOpt.isSet())
  {
    params.hwResW = hwRes;
  }

  if(resolutionYOpt.isSet())
  {
    params.hwResH = hwResY;
  }
  else if(resolutionOpt.isSet())
  {
    params.hwResH = hwRes;
  }

  if(tumble)
  {
    params.duplexMode = PrintParameters::TwoSidedShortEdge;
  }
  else if(duplex)
  {
    params.duplexMode = PrintParameters::TwoSidedLongEdge;
  }

  if(params.format == PrintParameters::URF)
  {
    if(params.getBitsPerColor() == 1 || params.isBlack())
    {
      print_error("URF does not support black or 1-bit color modes", args.argHelp());
      return 1;
    }
    if(mediaTypeOpt.isSet() && !isUrfMediaType(params.mediaType))
    {
      print_error("Invalid media type for URF", args.argHelp());
      return 1;
    }
  }

  OutBinFile outFile(outFileName);

  WriteFun writeFun([&outFile](unsigned char const* buf, unsigned int len) -> bool
           {
             outFile->write((const char*)buf, len);
             return outFile->exceptions() == std::ostream::goodbit;
           });

  Error error;

  if(verbose)
  {
    ProgressFun progressFun([](size_t page, size_t total) -> void
                {
                  std::cerr << "Progress: " << page << "/" << total << "\n\n";
                });
    error = pdf_to_printable(inFileName, writeFun, params, progressFun, true);
  }
  else
  {
    error = pdf_to_printable(inFileName, writeFun, params);
  }
  if(error)
  {
    std::cerr << "Conversion failed: " << error.value() << std::endl;
    return 1;
  }
  return 0;
}

