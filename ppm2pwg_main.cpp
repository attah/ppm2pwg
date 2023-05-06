#include <bytestream.h>
#include <fstream>
#include <iostream>
#include <string>
#include <limits>
#include <string.h>

#include "ppm2pwg.h"
#include "argget.h"

inline void ignore_comments(std::istream* in)
{
  if(in->peek() == '\n')
  {
    in->ignore(1);
  }
  while(in->peek() == '#')
  {
    in->ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
}

#ifndef PPM2PWG_MAIN
  #define PPM2PWG_MAIN main
#endif

int PPM2PWG_MAIN(int argc, char** argv)
{
  PrintParameters params;
  params.paperSizeUnits = PrintParameters::Pixels;
  bool help = false;
  bool verbose = false;
  bool urf = false;
  int pages = 0;
  int hwRes = 0;
  int hwResX = 0;
  int hwResY = 0;
  std::string inFile;
  std::string outFile;

  SwitchArg<bool> helpOpt(help, {"-h", "--help"}, "Print this help text");
  SwitchArg<bool> verboseOpt(verbose, {"-v", "--verbose"}, "Be verbose, print headers");
  SwitchArg<bool> urfOpt(urf, {"-u", "--urf"}, "Output URF format (default is PWG)");
  SwitchArg<int> pagesOpt(pages, {"--num-pages"}, "Number of pages to expect (for URF header)");
  SwitchArg<std::string> paperSizeOpt(params.paperSizeName, {"--paper-size"}, "Paper size name to set in header, e.g.: iso_a4_210x297mm");
  SwitchArg<int> resolutionOpt(hwRes, {"-r", "--resolution"}, "Resolution (in DPI) to set in header");
  SwitchArg<int> resolutionXOpt(hwResX, {"-rx", "--resolution-x"}, "Resolution (in DPI) to set in header, x-axis");
  SwitchArg<int> resolutionYOpt(hwResY, {"-ry", "--resolution-y"}, "Resolution (in DPI) to set in header, y-axis");
  SwitchArg<bool> duplexOpt(params.duplex, {"-d", "--duplex"}, "Process for duplex printing");
  SwitchArg<bool> tumbleOpt(params.duplex, {"-t", "--tumble"}, "Set tumble indicator in raster header");
  SwitchArg<bool> hFlipOpt(params.backHFlip, {"-hf", "--hflip"}, "Flip backsides horizontally for duplex");
  SwitchArg<bool> vFlipOpt(params.backVFlip, {"-vf", "--vflip"}, "Flip backsides vertically for duplex");
  SwitchArg<size_t> qualityOpt(params.quality, {"-q", "--quality"}, "Quality setting in raster header (3,4,5)");

  PosArg inArg(inFile, "in-file");
  PosArg outArg(outFile, "out-file");

  ArgGet args({&helpOpt, &verboseOpt, &urfOpt, &pagesOpt, &paperSizeOpt,
               &resolutionOpt, &resolutionXOpt, &resolutionYOpt,
               &duplexOpt, &tumbleOpt, &hFlipOpt, &vFlipOpt, &qualityOpt},
              {&inArg, &outArg});

  bool correctArgs = args.get_args(argc, argv);
  if(help)
  {
    std::cout << args.argHelp() << std::endl
              << "Use \"-\" as filename for stdin/stdout" << std::endl;
    return 0;
  }
  else if(!correctArgs)
  {
    std::cerr << args.errmsg() << std::endl << std::endl << args.argHelp() << std::endl;
    return 1;
  }

  if(urf)
  {
    params.format = PrintParameters::URF;
  }
  else
  {
    params.format = PrintParameters::PWG;
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

  std::ifstream ifs;
  std::istream* in;
  std::ofstream ofs;
  std::ostream* out;

  if(inFile == "-")
  {
    in = &std::cin;
  }
  else
  {
    ifs = std::ifstream(inFile, std::ios::in | std::ios::binary);
    in = &ifs;
  }

  if(outFile == "-")
  {
    out = &std::cout;
  }
  else
  {
    ofs = std::ofstream(outFile, std::ios::out | std::ios::binary);
    out = &ofs;
  }

  *out << fileHdr;

  while(!in->eof())
  {
    outBts.reset();
    page++;

    std::string p, xs, ys, r;
    *in >> p;

    ignore_comments(in);

    *in >> xs >> ys;

    if(p == "P6")
    {
      *in >> r;
      params.colorMode = PrintParameters::sRGB24;
    }
    else if(p == "P5")
    {
      *in >> r;
      params.colorMode = PrintParameters::Gray8;
    }
    else if(p == "P4")
    {
      r = "1";
      params.colorMode = PrintParameters::Black1;
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

    ignore_comments(in);

    if(verbose)
    {
      std::cerr << "Found: " << p << " " << xs << "x" << ys << " " << r << std::endl;
    }

    params.paperSizeW = stoi(xs);
    params.paperSizeH = stoi(ys);

    size_t size = params.paperSizeH*params.getPaperSizeWInBytes();
    bmpBts.initFrom(*in, size);

    bmp_to_pwg(bmpBts, outBts, page, params, verbose);

    *out << outBts;
    in->peek(); // maybe trigger eof
  }
  return 0;
}
