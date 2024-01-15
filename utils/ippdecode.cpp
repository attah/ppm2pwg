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
  try
  {
    IppMsg msg(inBts);
    std::cout << msg.getPrinterAttrs() << std::endl;
    for(const IppAttrs& ia : msg.getJobAttrs())
    {
      std::cout << ia << std::endl;
    }
  }
  catch(const std::exception& e)
  {
    std::cerr << "Exception caught." << std::endl;
  }
}
