#ifndef PDF2PRINTABLE_H
#define PDF2PRINTABLE_H

#include <functional>
#include "printparameters.h"

typedef std::function<bool(unsigned char const*, unsigned int)> write_fun;
typedef std::function<void(size_t page, size_t total)> progress_fun;

int pdf_to_printable(std::string Infile, write_fun WriteFun, PrintParameters Params,
                     progress_fun ProgressFun = nullptr, bool Verbose = false);

#endif //PDF2PRINTABLE_H
