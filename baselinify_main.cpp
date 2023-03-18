#include <fstream>
#include <iostream>
#include <baselinify.h>

int main(int argc, char** argv)
{
  if(argc != 3)
  {
    std::cerr << "Usage: baselinify <infile> <outfile> (\"-\" for stdin/stdout)" << std::endl;
    return 1;
  }

  std::string inFile(argv[1]);
  std::string outFile(argv[2]);

  std::ifstream ifs;
  std::istream* in;
  std::ofstream ofs;
  std::ostream* out;

  if(inFile == "-")
  {
    in = &std::cin;
    std::ios_base::sync_with_stdio(false);
  }
  else
  {
    ifs = std::ifstream(inFile, std::ios::in | std::ios::binary);
    in = &ifs;
  }

  if(outFile == "-")
  {
    out = &std::cout;
  }
  else
  {
    ofs = std::ofstream(outFile, std::ios::out | std::ios::binary);
    out = &ofs;
  }

  Bytestream inBts(*in);
  Bytestream outBts;

  baselinify(inBts, outBts);

  *out << outBts;
}
