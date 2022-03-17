#ifndef PPM2PWG_H
#define PPM2PWG_H

#include "printparameters.h"

Bytestream make_pwg_file_hdr();

Bytestream make_urf_file_hdr(uint32_t pages);

void make_pwg_hdr(Bytestream& OutBts, PrintParameters Params, bool Backside, bool Verbose);
void make_urf_hdr(Bytestream& OutBts, PrintParameters Params, bool Verbose);


void bmp_to_pwg(Bytestream& bmp_bts, Bytestream& OutBts,
                size_t page, PrintParameters params, bool Verbose = false);

#endif //PPM2PWG_H
