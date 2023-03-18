#ifndef PDF2PRINTABLE_H
#define PDF2PRINTABLE_H

#include <functional>
#include "printparameters.h"

typedef std::function<bool(unsigned char const*, unsigned int)> WriteFun;
typedef std::function<void(size_t page, size_t total)> ProgressFun;

int pdf_to_printable(std::string infile, WriteFun writeFun, const PrintParameters& params,
                     ProgressFun progressFun = nullptr, bool verbose = false);

#endif //PDF2PRINTABLE_H
