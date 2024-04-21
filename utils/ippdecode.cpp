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
  if(!inFile)
  {
    std::cerr << "Failed to open input" << std::endl;
    return 1;
  }

  Bytestream inBts(inFile);
  try
  {
    IppMsg msg(inBts);
    std::cout << "IPP operation or status: " << msg.getStatus() << std::endl;
    std::cout << "Operation attrs: "  << std::endl << msg.getOpAttrs().toJSON().dump() << std::endl;
    for(const IppAttrs& jobAttrs : msg.getJobAttrs())
    {
        std::cout << "Job attrs: "  << std::endl << jobAttrs.toJSON().dump() << std::endl;
    }
    std::cout << "Printer attrs: "  << std::endl << msg.getPrinterAttrs().toJSON().dump() << std::endl;
    }
  catch(const std::exception& e)
  {
    std::cerr << "Exception caught." << std::endl;
  }
}
