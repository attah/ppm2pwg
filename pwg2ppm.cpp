#include "pwg2ppm.h"
#include <fstream>
#include <iostream>

void raster_to_bmp(Bytestream& OutBts, Bytestream& file,
                   size_t width, size_t height, size_t colors, bool urf)
{
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

    OutBts << line;
    height--;
    for(size_t i=0; i < line_repeat; i++)
    {
      OutBts << line;
    }
    height-=line_repeat;
  }
}

void write_ppm(Bytestream& OutBts,size_t width, size_t height, size_t colors,
               std::string outfile_prefix, int page)
{
  std::string outfile_name = outfile_prefix+std::to_string(page)
                                           +(colors==3 ? ".ppm" : ".pgm");
  std::ofstream outfile(outfile_name, std::ofstream::out);
  outfile << (colors==3 ? "P6" : "P5")
          << '\n' << width << ' ' << height << '\n' << 255 << '\n';

  outfile << OutBts;
}
