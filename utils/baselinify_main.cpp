#include <fstream>
#include <iostream>

#include "baselinify.h"
#include "binfile.h"

int main(int argc, char** argv)
{
  if(argc != 3)
  {
    std::cerr << "Usage: baselinify <infile> <outfile> (\"-\" for stdin/stdout)" << std::endl;
    return 1;
  }

  InBinFile inFile(argv[1]);
  if(!inFile)
  {
    std::cerr << "Failed to open input" << std::endl;
    return 1;
  }

  OutBinFile outFile(argv[2]);

  Bytestream inBts(inFile);
  Bytestream outBts;

  baselinify(inBts, outBts);

  outFile << outBts;
}
