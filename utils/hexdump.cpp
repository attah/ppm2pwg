#include <fstream>
#include <iostream>

#include "argget.h"
#include "binfile.h"
#include "bytestream.h"

inline void print_error(const std::string& hint, const std::string& argHelp)
{
  std::cerr << hint << std::endl << std::endl << argHelp << std::endl;
}

int main(int argc, char** argv)
{
  bool help = false;

  std::string inFileName;

  SwitchArg<bool> helpOpt(help, {"-h", "--help"}, "Print this help text");

  PosArg inArg(inFileName, "in-file");

  ArgGet args({&helpOpt}, {&inArg},
              "Use \"-\" as filename for stdin.");

  bool correctArgs = args.get_args(argc, argv);
  if(help)
  {
    std::cout << args.argHelp() << std::endl;
    return 0;
  }
  else if(!correctArgs)
  {
    print_error(args.errmsg(), args.argHelp());
    return 1;
  }

  InBinFile inFile(argv[1]);
  if(!inFile)
  {
    std::cerr << "Failed to open input" << std::endl;
    return 1;
  }

  Bytestream file(inFile);
  std::cout << file.hexdump(file.size());
}
