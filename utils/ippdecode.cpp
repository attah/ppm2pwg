#include <iostream>

#include "argget.h"
#include "binfile.h"
#include "bytestream.h"
#include "ippmsg.h"

#define HELPTEXT "Use \"-\" as filename for stdin."

inline void print_error(const std::string& hint, const std::string& argHelp)
{
  std::cerr << hint << std::endl << std::endl << argHelp << std::endl << HELPTEXT << std::endl;
}

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

  Bytestream inBts(inFile);
  try
  {
    IppMsg msg(inBts);
    std::cout << "IPP operation or status: " << msg.getStatus() << std::endl;
    std::cout << "Operation attrs: " << std::endl << msg.getOpAttrs().toJSON().dump() << std::endl;
    for(const IppAttrs& jobAttrs : msg.getJobAttrs())
    {
        std::cout << "Job attrs: " << std::endl << jobAttrs.toJSON().dump() << std::endl;
    }
    std::cout << "Printer attrs: " << std::endl << msg.getPrinterAttrs().toJSON().dump() << std::endl;
    }
  catch(const std::exception& e)
  {
    std::cerr << "Exception caught." << std::endl;
  }
}
