#include "pwg2ppm.h"
#include <fstream>
#include <iostream>

void raster_to_bmp(Bytestream& OutBts, Bytestream& file,
                   size_t byte_width, size_t height, size_t colors, bool urf)
{
  Bytestream Grey8White {(uint8_t)0};
  Bytestream RGBWhite {(uint8_t)0xff, (uint8_t)0xff, (uint8_t)0xff};
  Bytestream CMYKWhite {(uint8_t)0x00, (uint8_t)0x00, (uint8_t)0x00, (uint8_t)0x00};
  Bytestream White = (colors == 1 ? Grey8White : colors == 4 ? CMYKWhite : RGBWhite);

  while(height)
  {
    Bytestream line;
    uint8_t line_repeat;
    file >> line_repeat;

    while(line.size() != byte_width)
    {
      uint8_t count;
      file >> count;

      if(urf && count==128)
      { // URF special case: 128 means fill line with white
        while(line.size() != byte_width)
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

void write_ppm(Bytestream& OutBts, size_t width, size_t height,
               size_t colors, size_t bits, bool black,
               std::string outfile_prefix, int page)
{
  if(bits == 1 && !black)
  {
    invert(OutBts);
  }
  else if(colors == 4)
  {
    cmyk2rgb(OutBts);
  }
  std::string outfile_name = outfile_prefix+std::to_string(page)
                                           +(colors > 2 ? ".ppm"
                                                        : bits == 8 ? ".pgm"
                                                                    : ".pbm");
  std::ofstream outfile(outfile_name, std::ofstream::out);
  outfile << (colors > 2 ? "P6" : (bits == 8 ? "P5" : "P4"))
          << '\n' << width << ' ' << height << '\n' << 255 << '\n';

  outfile << OutBts;
}

void invert(Bytestream& bts)
{
  size_t size = bts.size();
  uint8_t* raw = bts.raw();
  for(size_t i=0; i < size; i++)
  {
    raw[i] = ~raw[i];
  }
}

void cmyk2rgb(Bytestream& cmyk)
{
  Bytestream rgb;
  uint8_t c, m, y, k, r, g, b, w;
  size_t size = cmyk.size()/4;
  for(size_t i=0; i < size; i++)
  {
    cmyk >> c >> m >> y >> k;
    w = 255 - k;
    r = w - c;
    g = w - m;
    b = w - y;
    rgb << r << g << b;
  }
  cmyk = rgb;
}
