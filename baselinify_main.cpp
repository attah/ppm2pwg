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

  std::string InFile(argv[1]);
  std::string OutFile(argv[2]);

  std::ifstream ifs;
  std::istream* in;
  std::ofstream ofs;
  std::ostream* out;

  if(InFile == "-")
  {
    in = &std::cin;
    std::ios_base::sync_with_stdio(false);
  }
  else
  {
    ifs = std::ifstream(InFile, std::ios::in | std::ios::binary);
    in = &ifs;
  }

  if(OutFile == "-")
  {
    out = &std::cout;
  }
  else
  {
    ofs = std::ofstream(OutFile, std::ios::out | std::ios::binary);
    out = &ofs;
  }

  Bytestream InBts(*in);
  Bytestream OutBts;

  baselinify(InBts, OutBts);

  *out << OutBts;
}
