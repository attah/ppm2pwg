#ifndef MAP_H
#define MAP_H

#include <map>

template <typename K, typename V>
class Map: public std::map<K, V>
{
public:
  using std::map<K, V>::map;

#if __cplusplus < 202002L
  bool contains(const K elem) const
  {
    return std::map<K, V>::find(elem) != std::map<K, V>::cend();
  }
#endif

};

#endif // MAP_H
