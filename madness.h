#ifndef MADNESS_H
#define MADNESS_H

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

#define LIB(name, filename) \
        LibLoader name(filename)

#define FUNC(lib, ret, name, ...) \
          typedef ret (*name##_p)(__VA_ARGS__); \
          name##_p name = (name##_p)dlsym(lib.handle, #name)

#endif // MADNESS_H
