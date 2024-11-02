#include <fstream>
#include <iostream>

#include "bytestream.h"

int main(int argc, char** argv)
{
  if(argc > 2)
  {
    std::cerr << "Usage: hexdump [file]" << std::endl;
    return 1;
  }

  std::ifstream ifs;
  std::istream* in;

  if(argc == 2)
  {
    ifs = std::ifstream(argv[1], std::ios::in | std::ios::binary);
    in = &ifs;
  }
  else
  {
    in = &std::cin;
    std::ios_base::sync_with_stdio(false);
  }

  Bytestream file(*in);
  std::cout << file.hexdump(file.size());
}
