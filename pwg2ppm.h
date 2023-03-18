#ifndef PWG2PPM_H
#define PWG2PPM_H

#include <bytestream.h>

void raster_to_bmp(Bytestream& outBts, Bytestream& file,
                   size_t width, size_t height, size_t colors, bool urf);

void write_ppm(Bytestream& outBts,size_t width, size_t height,
               size_t colors, size_t bits, bool black,
               std::string outFilePrefix, int page);

void invert(Bytestream& bts);

void cmyk2rgb(Bytestream& cmyk);

#endif //PWG2PPM_H
