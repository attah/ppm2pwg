#include <iostream>

#include "argget.h"
#include "minimime.h"

int main(int argc, char** argv)
{
  bool help = false;

  std::string inFileName;

  SwitchArg<bool> helpOpt(help, {"-h", "--help"}, "Print this help text");

  PosArg inArg(inFileName, "in-file");

  ArgGet args({&helpOpt}, {&inArg});

  bool correctArgs = args.get_args(argc, argv);
  if(help)
  {
    std::cout << args.argHelp() << std::endl;
    return 0;
  }
  else if(!correctArgs)
  {
    std::cerr << args.errmsg() << std::endl << std::endl << args.argHelp() << std::endl;
    return 1;
  }

  std::cout << MiniMime::getMimeType(inFileName) << std::endl;
}
