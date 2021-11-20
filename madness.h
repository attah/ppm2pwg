#ifndef MADNESS_H
#define MADNESS_H

#if MADNESS

#include <dlfcn.h>
class LibLoader
{
public:
  LibLoader(std::string LibName)
  {
    handle = dlopen(LibName.c_str(), RTLD_LAZY);
    if(handle == nullptr)
    {
      throw std::runtime_error("failed to load library:"+LibName);
    }
  }
  ~LibLoader()
  {
    dlclose(handle);
  }

  void* handle;

private:
  LibLoader(const LibLoader&);
  LibLoader& operator=(const LibLoader&);
};

#define STR(s) STR_(#s)
#define STR_(s) s

#define LIB(name, filename) \
        LibLoader name(filename)

#define FUNC(lib, ret, name, ...) \
          typedef ret (*name##_p)(__VA_ARGS__); \
          name##_p name = (name##_p)dlsym(lib.handle, STR(name))

#else // MADNESS

#define LIB(name, filename)
#define FUNC(lib, ret, name, ...)

#endif // MADNESS
#endif // MADNESS_H
