#include "pwg2ppm.h"

#include <fstream>

void invert(Bytestream& bts);

void cmyk2rgb(Bytestream& cmyk);

void raster_to_bmp(Bytestream& outBts, Bytestream& file,
                   size_t byteWidth, size_t height, size_t colors, bool urf)
{
  Bytestream grey8White {(uint8_t)0};
  Bytestream RGBWhite {(uint8_t)0xff, (uint8_t)0xff, (uint8_t)0xff};
  Bytestream CMYKWhite {(uint8_t)0x00, (uint8_t)0x00, (uint8_t)0x00, (uint8_t)0x00};
  Bytestream white = (colors == 1 ? grey8White : colors == 4 ? CMYKWhite : RGBWhite);

  while(height)
  {
    Bytestream line;
    uint8_t lineRepeat;
    file >> lineRepeat;

    while(line.size() != byteWidth)
    {
      uint8_t count;
      file >> count;

      if(urf && count==128)
      { // URF special case: 128 means fill line with white
        while(line.size() != byteWidth)
        {
          line << white;
        }
      }
      else if(count < 128)
      { // repeats
        size_t repeats = count+1;
        Bytestream tmp = file.getBytestream(colors);
        for(size_t i=0; i<repeats; i++)
        {
          line << tmp;
        }
      }
      else
      { // verbatim
        size_t verbatim = (-1*((int)count-257));
        Bytestream tmp = file.getBytestream(verbatim*colors);
        line << tmp;
      }
    }

    outBts << line;
    height--;
    for(size_t i=0; i < lineRepeat; i++)
    {
      outBts << line;
    }
    height-=lineRepeat;
  }
}

void write_ppm(Bytestream& outBts, size_t width, size_t height,
               size_t colors, size_t bits, bool black,
               const std::string& outfilePrefix, int page)
{
  if((bits == 1 && !black) || (bits == 8 && black))
  {
    invert(outBts);
  }
  else if(colors == 4)
  {
    cmyk2rgb(outBts);
  }
  std::string outFileSuffix = (colors > 2 ? ".ppm" : bits == 8 ? ".pgm" : ".pbm");
  std::string outFileName = outfilePrefix+std::to_string(page) + outFileSuffix;
  std::ofstream outFile(outFileName, std::ofstream::out);
  outFile << (colors > 2 ? "P6" : (bits == 8 ? "P5" : "P4"))
          << '\n' << width << ' ' << height << '\n' << 255 << '\n';

  outFile << outBts;
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
  Bytestream rgb((cmyk.size() * 3) / 4);
  uint8_t* _cmyk = cmyk.raw();
  uint8_t* _rgb = rgb.raw();
  size_t size = cmyk.size();

  for(size_t i=0, j=0; i < size; i+=4, j+=3)
  {
    uint8_t kInv = 255 - _cmyk[i+3];
    _rgb[j] = kInv - _cmyk[i];
    _rgb[j+1] = kInv - _cmyk[i+1];
    _rgb[j+2] = kInv - _cmyk[i+2];
  }
  cmyk = rgb;
}
