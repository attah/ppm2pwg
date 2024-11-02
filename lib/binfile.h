#ifndef BINFILE_H
#define BINFILE_H

#include <fstream>
#include <iostream>
#include <string>

class InBinFile
{
public:
  InBinFile() = delete;
  InBinFile(const InBinFile&) = delete;
  InBinFile& operator=(const InBinFile&) = delete;

  InBinFile(std::string name)
  {
    if(name == "-")
    {
      in = &std::cin;
    }
    else
    {
      ifs = std::ifstream(name, std::ios::in | std::ios::binary);
      in = &ifs;
    }
  }
  operator std::istream&()
  {
    return *in;
  }
  std::istream& operator*()
  {
    return *in;
  }
  std::istream* operator->()
  {
    return in;
  }
  template <typename T>
  std::istream& operator>>(T& t)
  {
    *in >> t;
    return *in;
  }
  explicit operator bool() const
  {
    return in->operator bool();
  }
private:
  std::ifstream ifs;
  std::istream* in;
};

class OutBinFile
{
public:
  OutBinFile() = delete;
  OutBinFile(const InBinFile&) = delete;
  OutBinFile& operator=(const OutBinFile&) = delete;

  using value_type = std::ostream&;

  OutBinFile(std::string name)
  {
    if(name == "-")
    {
      out = &std::cout;
    }
    else
    {
      ofs = std::ofstream(name, std::ios::out | std::ios::binary);
      out = &ofs;
    }
  }
  operator std::ostream&()
  {
    return *out;
  }
  std::ostream& operator*()
  {
    return *out;
  }
  std::ostream* operator->()
  {
    return out;
  }
  explicit operator bool() const
  {
    return out->operator bool();
  }
private:
  std::ofstream ofs;
  std::ostream* out;
};

#endif //BINFILE_H
