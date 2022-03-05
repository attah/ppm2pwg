#include <fstream>
#include <iostream>
#include <bytestream.h>

int main(int argc, char** argv)
{
  if(argc != 2)
  {
    std::cerr << "Usage: hexdump <file>" << std::endl;
    return 1;
  }

  std::string filename(argv[1]);

  std::ifstream ifs(filename, std::ios::in | std::ios::binary);
  Bytestream file(ifs);
  std::cout << file.hexdump(file.size());
}
