#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <functional>

typedef std::function<bool(unsigned char const*, unsigned int)> WriteFun;
typedef std::function<void(size_t page, size_t total)> ProgressFun;

#endif // FUNCTIONS_H
