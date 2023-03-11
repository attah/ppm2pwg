#ifndef ARRAY_H
#define ARRAY_H

template<typename T>
class Array
{
public:
  Array(size_t size)
  {
    _array = new T[size];
  }
  ~Array()
  {
    delete[] _array;
  }
  Array(const Array&) = delete;
  Array& operator= (const Array&) = delete;
  operator T*()
  {
    return _array;
  }
private:
  T* _array;
};

#endif //ARRAY_H
