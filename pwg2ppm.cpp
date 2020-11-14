#include <bytestream.h>
#include <fstream>
#include <iostream>

#define CODABLE_FILE "pwg_pghdr_codable.h"
#include <codable.h>
#define CODABLE_FILE "urf_pghdr_codable.h"
#include <codable.h>

void to_image(Bytestream& file, size_t width, size_t height, size_t colors,
              bool urf, std::string outfile_prefix, int page);

int main(int argc, char** argv)
{
  if(argc != 3)
  {
    std::cerr << "Usage: pwg2ppm <infile> <outfile prefix>" << std::endl;
    return 1;
  }
  std::ifstream ifs(argv[1], std::ios::in | std::ios::binary | std::ios::ate);
  std::string outfile_prefix(argv[2]);

  std::ifstream::pos_type fileSize = ifs.tellg();
  ifs.seekg(0, std::ios::beg);

  Bytestream file(fileSize);
  ifs.read((char*)(file.raw()), fileSize);

  std::cerr << "File is " << fileSize << " long" << std::endl;

  size_t pages = 0;

  if(file >>= "RaS2")
  {

    std::cerr << "Smells like PWG Raster" << std::endl;
    do
    {
      std::cerr << "Page " << ++pages << std::endl;
      PwgPgHdr PwgHdr;
      PwgHdr.decode_from(file);
      std::cerr << PwgHdr.describe() << std::endl;
      to_image(file, PwgHdr.Width, PwgHdr.Height, PwgHdr.NumColors, false,
               outfile_prefix, pages);
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
      UrfHdr.decode_from(file);
      std::cerr << UrfHdr.describe() << std::endl;
      to_image(file, UrfHdr.Width, UrfHdr.Height, UrfHdr.BitsPerPixel/8, true,
               outfile_prefix, pages);
    }
    while (file.remaining());
  }
  else
  {
    std::cerr << "Unknown file format" << std::endl;
    return 1;
  }
  std::cerr << "Total pages: " << pages << std::endl;
}

void to_image(Bytestream& file, size_t width, size_t height, size_t colors,
              bool urf, std::string outfile_prefix, int page)
{
  std::string outfile_name = outfile_prefix+std::to_string(page)
                                           +(colors==3 ? ".ppm" : ".pgm");
  std::ofstream outfile(outfile_name, std::ofstream::out);
  outfile << (colors==3 ? "P6" : "P5")
          << '\n' << width << ' ' << height << '\n' << 255 << '\n';

  Bytestream Grey8White {(uint8_t)0};
  Bytestream RGBWhite {(uint8_t)0xff, (uint8_t)0xff, (uint8_t)0xff};
  Bytestream White = (colors == 1 ? Grey8White : RGBWhite);

  while(height)
  {
    Bytestream line;
    uint8_t line_repeat;
    file >> line_repeat;

    while(line.size() != (width*colors))
    {
      uint8_t count;
      file >> count;

      if(urf && count==128)
      { // URF special case: 128 means fill line with white
        // (code assumes gray8 or RGB)

        while(line.size() != (width*colors))
        {
          line << White;
        }
      }
      else if (count < 128)
      { // repeats
        size_t repeats = count+1;
        Bytestream tmp;
        file/colors >> tmp;
        for(size_t i=0; i<repeats; i++)
        {
          line << tmp;
        }
      }
      else
      { // verbatim
        size_t verbatim = (-1*((int)count-257));
        Bytestream tmp;
        file/(verbatim*colors) >> tmp;
        line << tmp;
      }
    }

    outfile.write((char*)line.raw(), line.size());
    height--;

    for(size_t i=0; i < line_repeat; i++)
    {
      outfile.write((char*)line.raw(), line.size());
    }
    height-=line_repeat;
  }
}
