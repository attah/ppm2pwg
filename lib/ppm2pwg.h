#ifndef PPM2PWG_H
#define PPM2PWG_H

#include <bytestream.h>
#include "printparameters.h"
#include "PwgPgHdr.h"

Bytestream make_pwg_file_hdr();

Bytestream make_urf_file_hdr(uint32_t pages);

void make_pwg_hdr(Bytestream& outBts, const PrintParameters& params, bool backside, bool verbose);
void make_urf_hdr(Bytestream& outBts, const PrintParameters& params, bool verbose);


void bmp_to_pwg(Bytestream& bmpBts, Bytestream& outBts, size_t page,
                const PrintParameters& params, bool verbose = false);

void compress_lines(Bytestream& bmpBts, Bytestream& outBts,
                    const PrintParameters& params, bool backside);

void compress_line(uint8_t* raw, size_t len, Bytestream& outBts, int Colors);

PwgPgHdr::MediaPosition_enum media_position_from_name(std::string name);

#endif //PPM2PWG_H
