#include <iostream>
#include <fstream>

#include "argget.h"
#include "binfile.h"
#include "log.h"
#include "pwg2ppm.h"
#include "pwgpghdr.h"
#include "urfpghdr.h"

#define HELPTEXT "Use \"-\" as filename for stdin."

inline void print_error(const std::string& hint, const std::string& argHelp)
{
  std::cerr << hint << std::endl << std::endl << argHelp << std::endl << HELPTEXT << std::endl;
}

int main(int argc, char** argv)
{
  bool help = false;
  bool verbose = false;

  std::string inFileName;
  std::string outFilePrefix;

  SwitchArg<bool> helpOpt(help, {"-h", "--help"}, "Print this help text");
  SwitchArg<bool> verboseOpt(verbose, {"-v", "--verbose"}, "Be verbose, print headers");

  PosArg inArg(inFileName, "in-file");
  PosArg outArg(outFilePrefix, "out-file prefix");

  ArgGet args({&helpOpt, &verboseOpt},
              {&inArg, &outArg});

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

  if(verbose)
  {
    LogController::instance().enable(LogController::Debug);
  }

  InBinFile inFile(inFileName);
  if(!inFile)
  {
    std::cerr << "Failed to open input" << std::endl;
    return 1;
  }
  Bytestream file(inFile);
  DBG(<< "File is " << file.size() << " long");

  size_t pages = 0;
  Bytestream outBts;

  if(file >>= "RaS2")
  {

    DBG(<< "Smells like PWG Raster");

    while(file.remaining())
    {
      DBG(<< "Page " << ++pages);
      PwgPgHdr pwgHdr;
      pwgHdr.decodeFrom(file);
      DBG(<< pwgHdr.describe());
      raster_to_bmp(outBts, file, pwgHdr.BytesPerLine, pwgHdr.Height, pwgHdr.NumColors, false);
      write_ppm(outBts, pwgHdr.Width, pwgHdr.Height, pwgHdr.NumColors, pwgHdr.BitsPerColor,
                pwgHdr.ColorSpace == PwgPgHdr::Black, outFilePrefix, pages);
      outBts.reset();
    }
  }
  else if(file >>= "UNIRAST")
  {
    uint32_t pageCount;
    file >> (uint8_t)0 >> pageCount;
    DBG(<< "Smells like URF Raster, with " << pageCount << " pages");

    while(file.remaining())
    {
      DBG(<< "Page " << ++pages);
      UrfPgHdr urfHdr;
      urfHdr.decodeFrom(file);
      DBG(<< urfHdr.describe());
      uint32_t byteWidth = urfHdr.Width * (urfHdr.BitsPerPixel/8);
      raster_to_bmp(outBts, file, byteWidth, urfHdr.Height, urfHdr.BitsPerPixel/8, true);
      write_ppm(outBts, urfHdr.Width, urfHdr.Height, urfHdr.BitsPerPixel/8, 8,
                false, outFilePrefix, pages);
      outBts.reset();
    }
  }
  else
  {
    std::cerr << "Unknown file format" << std::endl;
    return 1;
  }
  DBG(<< "Total pages: " << pages);
  return 0;
}
