#include "pwg2ppm.h"
#include "PwgPgHdr.h"
#include "UrfPgHdr.h"
#include <fstream>
#include <iostream>

int main(int argc, char** argv)
{
  if(argc != 3)
  {
    std::cerr << "Usage: pwg2ppm <infile> <outfile prefix>" << std::endl;
    return 1;
  }

  std::string filename(argv[1]);
  std::string outfile_prefix(argv[2]);

  std::ifstream ifs(filename, std::ios::in | std::ios::binary);
  Bytestream file(ifs);
  std::cerr << "File is " << file.size() << " long" << std::endl;

  size_t pages = 0;
  Bytestream OutBts;

  if(file >>= "RaS2")
  {

    std::cerr << "Smells like PWG Raster" << std::endl;
    do
    {
      std::cerr << "Page " << ++pages << std::endl;
      PwgPgHdr PwgHdr;
      PwgHdr.decodeFrom(file);
      std::cerr << PwgHdr.describe() << std::endl;
      raster_to_bmp(OutBts, file, PwgHdr.BytesPerLine, PwgHdr.Height, PwgHdr.NumColors, false);
      write_ppm(OutBts, PwgHdr.Width, PwgHdr.Height, PwgHdr.NumColors, PwgHdr.BitsPerColor,
                PwgHdr.ColorSpace == PwgPgHdr::Black, outfile_prefix, pages);
      OutBts.reset();
    }
    while (file.remaining());
  }
  else if(file >>= "UNIRAST")
  {
    uint32_t PageCount;
    file >> (uint8_t)0 >> PageCount;
    std::cerr << "Smells like URF Raster, with "
              << PageCount << " pages" << std::endl;
    do
    {
      std::cerr << "Page " << ++pages << std::endl;
      UrfPgHdr UrfHdr;
      UrfHdr.decodeFrom(file);
      std::cerr << UrfHdr.describe() << std::endl;
      uint32_t ByteWidth = UrfHdr.Width * (UrfHdr.BitsPerPixel/8);
      raster_to_bmp(OutBts, file, ByteWidth, UrfHdr.Height, UrfHdr.BitsPerPixel/8, true);
      write_ppm(OutBts, UrfHdr.Width, UrfHdr.Height, UrfHdr.BitsPerPixel/8, 8,
                false, outfile_prefix, pages);
      OutBts.reset();
    }
    while (file.remaining());
  }
  else
  {
    std::cerr << "Unknown file format" << std::endl;
    return 1;
  }
  std::cerr << "Total pages: " << pages << std::endl;
  return 0;
}
