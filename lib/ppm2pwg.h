#ifndef PPM2PWG_H
#define PPM2PWG_H

#include "bytestream.h"
#include "printparameters.h"

#include <string>

Bytestream make_pwg_file_hdr();

Bytestream make_urf_file_hdr(uint32_t pages);

void bmp_to_pwg(Bytestream& bmpBts, Bytestream& outBts, size_t page, const PrintParameters& params);

bool isUrfMediaType(std::string mediaType);

#endif //PPM2PWG_H
