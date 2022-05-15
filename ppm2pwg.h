#ifndef PPM2PWG_H
#define PPM2PWG_H

#include <bytestream.h>
#include "printparameters.h"

Bytestream make_pwg_file_hdr();

Bytestream make_urf_file_hdr(uint32_t pages);

void make_pwg_hdr(Bytestream& OutBts, PrintParameters Params, bool Backside, bool Verbose);
void make_urf_hdr(Bytestream& OutBts, PrintParameters Params, bool Verbose);


void bmp_to_pwg(Bytestream& bmp_bts, Bytestream& OutBts, size_t page,
                const PrintParameters& params, bool Verbose = false);

void compress_lines(Bytestream& bmp_bts, Bytestream& OutBts,
                    const PrintParameters& Params, bool backside);

void compress_line(uint8_t* raw, size_t len, Bytestream& OutBts, int Colors);

#endif //PPM2PWG_H
