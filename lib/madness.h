#ifndef MADNESS_H
#define MADNESS_H

#include <dlfcn.h>
#include <stdexcept>
#include <string>

class LibLoader
{
public:
  LibLoader(const LibLoader&) = delete;
  LibLoader& operator=(const LibLoader&) = delete;
  LibLoader(const std::string& libName)
  {
    handle = dlopen(libName.c_str(), RTLD_LAZY);
    if(handle == nullptr)
    {
      throw std::runtime_error("failed to load library:"+libName);
    }
  }
  ~LibLoader()
  {
    dlclose(handle);
  }

  void* handle;
};

#define LIB(name, filename) \
        LibLoader lib##name(filename)

#define FUNC(libLoader, ret, name, ...) \
          typedef ret (*name##_p)(__VA_ARGS__); \
          name##_p name = (name##_p)dlsym(lib##libLoader.handle, #name)

#endif // MADNESS_H
