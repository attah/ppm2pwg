#ifndef PWG2PPM_H
#define PWG2PPM_H

#include <bytestream.h>

void raster_to_bmp(Bytestream& OutBts, Bytestream& file,
                   size_t width, size_t height, size_t colors, bool urf);

void write_ppm(Bytestream& OutBts,size_t width, size_t height, size_t colors,
              std::string outfile_prefix, int page);

#endif //PWG2PPM_H
