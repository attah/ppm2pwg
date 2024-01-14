#include <iostream>
#include <bytestream.h>
#include <ippmsg.h>
#include <binfile.h>

int main(int argc, char** argv)
{
  if(argc != 2)
  {
    std::cerr << "Usage: ippdecode [file]" << std::endl;
    return 1;
  }

  InBinFile inFile(argv[1]);
  Bytestream inBts(inFile);
  IppMsg msg(inBts);
}
