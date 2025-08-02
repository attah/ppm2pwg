#include <cstring>
#include <iostream>
#include <string>
#include <limits>

#include "argget.h"
#include "binfile.h"
#include "bytestream.h"
#include "log.h"
#include "mediaposition.h"
#include "ppm2pwg.h"
#include "stringutils.h"

inline void print_error(const std::string& hint, const std::string& argHelp)
{
  std::cerr << hint << std::endl << std::endl << argHelp << std::endl;
}

inline void ignore_comments(std::istream& in)
{
  if(in.peek() == '\n')
  {
    in.ignore(1);
  }
  while(in.peek() == '#')
  {
    in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
}

int main(int argc, char** argv)
{
  PrintParameters params;
  params.format = PrintParameters::PWG;
  params.paperSizeUnits = PrintParameters::Pixels;
  bool help = false;
  bool verbose = false;
  int pages = 0;
  int hwRes = 0;
  int hwResX = 0;
  int hwResY = 0;
  bool duplex = false;
  bool tumble = false;
  std::string inFileName;
  std::string outFileName;

  SwitchArg<bool> helpOpt(help, {"-h", "--help"}, "Print this help text");
  SwitchArg<bool> verboseOpt(verbose, {"-v", "--verbose"}, "Be verbose, print headers");
  EnumSwitchArg<PrintParameters::Format> formatOpt(params.format, {{"pwg", PrintParameters::PWG},{"urf", PrintParameters::URF}},
                                                   {"-f", "--format"}, "Format to output (pwg/urf)", "Unrecognized target format");
  SwitchArg<int> pagesOpt(pages, {"--num-pages"}, "Number of pages to expect (for URF header)");
  SwitchArg<std::string> paperSizeOpt(params.paperSizeName, {"--paper-size"}, "Paper size name in header, e.g.: iso_a4_210x297mm");
  SwitchArg<int> resolutionOpt(hwRes, {"-r", "--resolution"}, "Resolution (in DPI) to set in header");
  SwitchArg<int> resolutionXOpt(hwResX, {"-rx", "--resolution-x"}, "Resolution (in DPI) to set in header, x-axis");
  SwitchArg<int> resolutionYOpt(hwResY, {"-ry", "--resolution-y"}, "Resolution (in DPI) to set in header, y-axis");
  SwitchArg<bool> duplexOpt(duplex, {"-d", "--duplex"}, "Process for duplex printing");
  SwitchArg<bool> tumbleOpt(tumble, {"-t", "--tumble"}, "Process for tumbled duplex output");
  EnumSwitchArg<PrintParameters::BackXformMode> backXformOpt(params.backXformMode,
                                                             {{"rotate", PrintParameters::Rotated},
                                                              {"flip", PrintParameters::Flipped},
                                                              {"manual-tumble", PrintParameters::ManualTumble}},
                                                             {"-b", "--back-xform"},
                                                             "Transform backsides (rotate/flip/manual-tumble)");
  EnumSwitchArg<PrintParameters::Quality> qualityOpt(params.quality,
                                                     {{"draft", PrintParameters::DraftQuality},
                                                      {"normal", PrintParameters::NormalQuality},
                                                      {"high", PrintParameters::HighQuality}},
                                                     {"-q", "--quality"},
                                                     "Quality setting in raster header (draft/normal/high)");
  EnumSwitchArg<PrintParameters::MediaPosition> mediaPositionOpt(params.mediaPosition, MEDIA_POSITION_MAP,
                                                                 {"-mp", "--media-pos"},
                                                                 "Media position, e.g.: main, top, left, roll-2 etc.");
  SwitchArg<std::string> mediaTypeOpt(params.mediaType, {"-mt", "--media-type"}, "Media type, e.g.: stationery, cardstock etc.");

  PosArg inArg(inFileName, "in-file");
  PosArg outArg(outFileName, "out-file");

  ArgGet args({&helpOpt, &verboseOpt, &formatOpt, &pagesOpt, &paperSizeOpt,
               &resolutionOpt, &resolutionXOpt, &resolutionYOpt,
               &duplexOpt, &tumbleOpt, &backXformOpt, &qualityOpt,
               &mediaPositionOpt, &mediaTypeOpt},
              {&inArg, &outArg},
              "Use \"-\" as filename for stdin/stdout.");

  bool correctArgs = args.get_args(argc, argv);
  if(help)
  {
    std::cout << args.argHelp() << std::endl;
    return 0;
  }
  else if(!correctArgs)
  {
    print_error(args.errmsg(), args.argHelp());
    return 1;
  }

  if(verbose)
  {
    LogController::instance().enable(LogController::Debug);
  }

  if(!formatOpt.isSet() && string_ends_with(outFileName, ".urf"))
  {
    params.format = PrintParameters::URF;
  }

  if((params.format == PrintParameters::URF) && mediaTypeOpt.isSet() && !isUrfMediaType(params.mediaType))
  {
      print_error("Invalid media type for URF", args.argHelp());
      return 1;
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

  Bytestream fileHdr;

  if(params.format == PrintParameters::URF)
  {
    fileHdr = make_urf_file_hdr(pages);
  }
  else
  {
    fileHdr = make_pwg_file_hdr();
  }

  size_t page = 0;

  Bytestream outBts;
  Bytestream bmpBts;

  InBinFile inFile(inFileName);
  if(!inFile)
  {
    std::cerr << "Failed to open input" << std::endl;
    return 1;
  }

  OutBinFile outFile(outFileName);

  outFile << fileHdr;

  while(!inFile->eof())
  {
    outBts.reset();
    page++;

    std::string p;
    std::string xs;
    std::string ys;
    std::string r;
    inFile >> p;

    ignore_comments(inFile);

    inFile >> xs >> ys;

    if(p == "P6")
    {
      inFile >> r;
      if(r == "255")
      {
        params.colorMode = PrintParameters::sRGB24;
      }
      else if(r == "65535")
      {
        params.colorMode = PrintParameters::sRGB48;
      }
      else
      {
        std::cerr << "Only 255 and 65535 bit-depths supported, got " << r << std::endl;
        return 1;
      }
    }
    else if(p == "P5")
    {
      inFile >> r;
      if(r == "255")
      {
        params.colorMode = PrintParameters::Gray8;
      }
      else if(r == "65535")
      {
        params.colorMode = PrintParameters::Gray16;
      }
      else
      {
        std::cerr << "Only 255 and 65535 bit-depths supported, got " << r << std::endl;
        return 1;
      }
    }
    else if(p == "P4")
    {
      r = "1";
      params.colorMode = PrintParameters::Black1;
      size_t x = stoul(xs);
      if(x % 8 != 0)
      {
        std::cerr << "Only whole-byte width P4 PBMs supported, got " << x << std::endl;
        return 1;
      }
      if(params.format == PrintParameters::URF)
      {
        std::cerr << "URF does not support 1-bit (P4/pbm) color." << std::endl;
        return 1;
      }
    }
    else
    {
      std::cerr << "Only P4/P5/P6 (raw) supported, got " << p << std::endl;
      return 1;
    }

    ignore_comments(inFile);


    DBG(<< "Found: " << p << " " << xs << "x" << ys << " " << r);

    params.paperSizeW = stoul(xs);
    params.paperSizeH = stoul(ys);

    size_t size = params.paperSizeH*params.getPaperSizeWInBytes();
    bmpBts = Bytestream(inFile, size);

    bmp_to_pwg(bmpBts, outBts, page, params);

    outFile << outBts;
    inFile->peek(); // maybe trigger eof
  }
  return 0;
}
