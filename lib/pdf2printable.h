#ifndef PDF2PRINTABLE_H
#define PDF2PRINTABLE_H

#include "error.h"
#include "functions.h"
#include "printparameters.h"

Error pdf_to_printable(std::string infile, WriteFun writeFun, const PrintParameters& params,
                       ProgressFun progressFun = nullptr, bool verbose = false);

#endif //PDF2PRINTABLE_H
