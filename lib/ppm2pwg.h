#ifndef PPM2PWG_H
#define PPM2PWG_H

#include <bytestream.h>
#include "printparameters.h"
#include <string>

Bytestream make_pwg_file_hdr();

Bytestream make_urf_file_hdr(uint32_t pages);

void make_pwg_hdr(Bytestream& outBts, const PrintParameters& params, bool backside);
void make_urf_hdr(Bytestream& outBts, const PrintParameters& params);


void bmp_to_pwg(Bytestream& bmpBts, Bytestream& outBts, size_t page,
                const PrintParameters& params);

void compress_lines(Bytestream& bmpBts, Bytestream& outBts,
                    const PrintParameters& params, bool backside);

void compress_line(uint8_t* raw, size_t len, Bytestream& outBts, int Colors);

bool isUrfMediaType(std::string mediaType);

#endif //PPM2PWG_H
