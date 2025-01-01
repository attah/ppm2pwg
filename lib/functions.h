#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "bytestream.h"

#include <functional>

using WriteFun = std::function<bool(Bytestream&& data)>;
using ProgressFun = std::function<void(size_t page, size_t total)>;

static ProgressFun noOpProgressfun([](size_t, size_t) -> void {});

#endif // FUNCTIONS_H
