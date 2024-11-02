#include <iostream>

#include "minimime.h"

int main(int argc, char** argv)
{
  if(argc != 2)
  {
    std::cerr << "Usage: minimime <file>" << std::endl;
    return 1;
  }
  std::cout << MiniMime::getMimeType(argv[1]) << std::endl;
}
