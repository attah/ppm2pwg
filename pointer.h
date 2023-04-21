#ifndef POINTER_H
#define POINTER_H

#include <memory>
#include <functional>

template<typename T, typename D = std::function<void(T*)>>
class Pointer : public std::unique_ptr<T, D>
{
public:
  Pointer(T* ptr, D deleter)
  : std::unique_ptr<T, D>(ptr, [deleter](T* p){if(p) deleter(p);})
  {}
  Pointer<T, D>& operator=(T* Ptr)
  {
    std::unique_ptr<T, D>::reset(Ptr);
    return *this;
  }
  operator T*() const
  {
    return std::unique_ptr<T, D>::get();
  }
};

#endif //POINTER_H
