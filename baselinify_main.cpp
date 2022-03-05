#include <fstream>
#include <iostream>
#include <baselinify.h>

int main(int argc, char** argv)
{
  if(argc != 3)
  {
    std::cerr << "Usage: baselinify <infile> <outfile>" << std::endl;
    return 1;
  }

  std::string InFile(argv[1]);
  std::string OutFile(argv[2]);

  std::ifstream ifs = std::ifstream(InFile, std::ios::in | std::ios::binary);
  std::ofstream ofs = std::ofstream(OutFile, std::ios::out | std::ios::binary);

  Bytestream InBts(ifs);
  Bytestream OutBts;

  baselinify(InBts, OutBts);

  ofs << OutBts;
}
