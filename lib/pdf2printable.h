#ifndef PDF2PRINTABLE_H
#define PDF2PRINTABLE_H

#include "error.h"
#include "functions.h"
#include "printparameters.h"

#include <string>

Error pdf_to_printable(const std::string& infile, const PrintParameters& params,
                       const WriteFun& writeFun, const ProgressFun& progressFun = noOpProgressfun);

#endif //PDF2PRINTABLE_H
