#ifndef UNIQUEPOINTER_H
#define UNIQUEPOINTER_H

#include <functional>
#include <memory>

template<typename T, typename D = std::function<void(T*)>>
class UniquePointer : public std::unique_ptr<T, D>
{
public:
  UniquePointer(T* ptr, const D& deleter)
  : std::unique_ptr<T, D>(ptr, [deleter](T* p){if(p) {deleter(p);}})
  {}
  UniquePointer<T, D>& operator=(T* Ptr)
  {
    std::unique_ptr<T, D>::reset(Ptr);
    return *this;
  }
  operator T*() const
  {
    return std::unique_ptr<T, D>::get();
  }
};

#endif // UNIQUEPOINTER_H
