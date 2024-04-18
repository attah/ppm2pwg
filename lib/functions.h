#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <functional>
#include <bytestream.h>

typedef std::function<bool(Bytestream&& data)> WriteFun;
typedef std::function<void(size_t page, size_t total)> ProgressFun;

#endif // FUNCTIONS_H
