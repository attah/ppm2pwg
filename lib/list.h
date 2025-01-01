#ifndef LIST_H
#define LIST_H

#include <list>
#include <stdexcept>

template <typename T>
class List: public std::list<T>
{
public:
  using std::list<T>::list;

  T takeFront()
  {
    if(std::list<T>::empty())
    {
      throw std::logic_error("List is empty");
    }

    T tmp(std::list<T>::front());
    std::list<T>::pop_front();
    return tmp;
  }

  bool contains(const T& elem) const
  {
    for(const T& e : *this)
    {
      if(e == elem)
      {
        return true;
      }
    }
    return false;
  }

  List<T>& operator+=(const List<T>& other)
  {
    this->insert(this->cend(), other.cbegin(), other.cend());
    return *this;
  }
};

#endif // LIST_H
