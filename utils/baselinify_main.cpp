#include <iostream>

#include "argget.h"
#include "baselinify.h"
#include "binfile.h"

#define HELPTEXT "Use \"-\" as filename for stdin/stdout."

inline void print_error(const std::string& hint, const std::string& argHelp)
{
  std::cerr << hint << std::endl << std::endl << argHelp << std::endl << HELPTEXT << std::endl;
}

int main(int argc, char** argv)
{
  bool help = false;

  std::string inFileName;
  std::string outFileName;

  SwitchArg<bool> helpOpt(help, {"-h", "--help"}, "Print this help text");

  PosArg inArg(inFileName, "in-file");
  PosArg outArg(outFileName, "out-file prefix");

  ArgGet args({&helpOpt}, {&inArg, &outArg});

  bool correctArgs = args.get_args(argc, argv);
  if(help)
  {
    std::cout << args.argHelp() << std::endl << HELPTEXT << std::endl;
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

  OutBinFile outFile(argv[2]);

  Bytestream inBts(inFile);
  Bytestream outBts;

  baselinify(inBts, outBts);

  outFile << outBts;
}
