#include <iostream>
#include <fstream>

#include "pwg2ppm.h"
#include "PwgPgHdr.h"
#include "UrfPgHdr.h"

int main(int argc, char** argv)
{
  if(argc != 3)
  {
    std::cerr << "Usage: pwg2ppm <infile> <outfile prefix>" << std::endl;
    return 1;
  }

  std::string fileName(argv[1]);
  std::string outfilePrefix(argv[2]);

  std::ifstream ifs(fileName, std::ios::in | std::ios::binary);
  Bytestream file(ifs);
  std::cerr << "File is " << file.size() << " long" << std::endl;

  size_t pages = 0;
  Bytestream outBts;

  if(file >>= "RaS2")
  {

    std::cerr << "Smells like PWG Raster" << std::endl;

    while(file.remaining())
    {
      std::cerr << "Page " << ++pages << std::endl;
      PwgPgHdr pwgHdr;
      pwgHdr.decodeFrom(file);
      std::cerr << pwgHdr.describe() << std::endl;
      raster_to_bmp(outBts, file, pwgHdr.BytesPerLine, pwgHdr.Height, pwgHdr.NumColors, false);
      write_ppm(outBts, pwgHdr.Width, pwgHdr.Height, pwgHdr.NumColors, pwgHdr.BitsPerColor,
                pwgHdr.ColorSpace == PwgPgHdr::Black, outfilePrefix, pages);
      outBts.reset();
    }
  }
  else if(file >>= "UNIRAST")
  {
    uint32_t pageCount;
    file >> (uint8_t)0 >> pageCount;
    std::cerr << "Smells like URF Raster, with "
              << pageCount << " pages" << std::endl;

    while(file.remaining())
    {
      std::cerr << "Page " << ++pages << std::endl;
      UrfPgHdr urfHdr;
      urfHdr.decodeFrom(file);
      std::cerr << urfHdr.describe() << std::endl;
      uint32_t byteWidth = urfHdr.Width * (urfHdr.BitsPerPixel/8);
      raster_to_bmp(outBts, file, byteWidth, urfHdr.Height, urfHdr.BitsPerPixel/8, true);
      write_ppm(outBts, urfHdr.Width, urfHdr.Height, urfHdr.BitsPerPixel/8, 8,
                false, outfilePrefix, pages);
      outBts.reset();
    }
  }
  else
  {
    std::cerr << "Unknown file format" << std::endl;
    return 1;
  }
  std::cerr << "Total pages: " << pages << std::endl;
  return 0;
}
