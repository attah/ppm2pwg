#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <string>

#include "list.h"

inline bool string_starts_with(std::string s, std::string start)
{
  if(start.length() <= s.length())
  {
    return s.substr(0, start.length()) == start;
  }
  return false;
}

inline bool string_ends_with(std::string s, std::string ending)
{
  if(ending.length() <= s.length())
  {
    return s.substr(s.length()-ending.length(), ending.length()) == ending;
  }
  return false;
}

inline bool string_contains(std::string s, std::string what)
{
  return s.find(what) != std::string::npos;
}

inline List<std::string> split_string(const std::string& s, const std::string& tok)
{
  size_t pos = 0;
  List<std::string> res;
  while(pos <= s.length())
  {
    size_t found = std::min(s.length(), s.find(tok, pos));
    res.push_back(s.substr(pos, (found - pos)));
    pos = found + 1;
  }
  return res;
}

inline std::string join_string(List<std::string> list, const std::string& sep)
{
  std::string res;
  if(!list.empty())
  {
    res = list.takeFront();
    for(const std::string& e : list)
    {
      res += sep + e;
    }
  }
  return res;
}

#endif //STRINGUTILS_H
