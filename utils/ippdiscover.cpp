#include <iostream>

#include "argget.h"
#include "ippdiscovery.h"
#include "log.h"

inline void print_error(const std::string& hint, const std::string& argHelp)
{
  std::cerr << hint << std::endl << std::endl << argHelp << std::endl;
}

int main(int argc, char** argv)
{
  bool help = false;
  bool verbose = false;

  SwitchArg<bool> helpOpt(help, {"-h", "--help"}, "Print this help text");
  SwitchArg<bool> verboseOpt(verbose, {"-v", "--verbose"}, "Be verbose");

  ArgGet args({&helpOpt, &verboseOpt});

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

  if(verbose)
  {
    LogController::instance().enable(LogController::Debug);
  }

  IppDiscovery ippDiscovery([](const std::string& addr){std::cout << addr << std::endl;});
  ippDiscovery.discover();
}
