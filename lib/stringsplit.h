#ifndef STRINGSPLIT_H
#define STRINGSPLIT_H

#include <string>
#include <list.h>

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

inline std::string join_string(List<std::string> list,  const std::string& sep)
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

#endif //STRINGSPLIT_H
