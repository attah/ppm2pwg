#include <iostream>

#include "argget.h"
#include "binfile.h"
#include "bytestream.h"

#define HELPTEXT "Negative numbers means \"all but\" number of bytes from the beginning/end."

inline void print_error(const std::string& hint, const std::string& argHelp)
{
  std::cerr << hint << std::endl << std::endl << argHelp << std::endl << HELPTEXT << std::endl;
}

int main(int argc, char** argv)
{
  bool help = false;
  int head = 0;
  int tail = 0;
  std::string inFileName;
  std::string outFileName;

  SwitchArg<bool> helpOpt(help, {"-h", "--help"}, "Print this help text");
  SwitchArg<int> headOpt(head, {"--head"}, "Get head number of bytes from the beginning");
  SwitchArg<int> tailOpt(tail, {"--tail"}, "Get tail number of bytes from the end");

  PosArg inFileArg(inFileName, "input file");
  PosArg outFileArg(outFileName, "output file");

  ArgGet args({&helpOpt, &headOpt, &tailOpt},
              {&inFileArg, &outFileArg});

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

  InBinFile inFile(inFileName);
  if(!inFile)
  {
    std::cerr << "Failed to open input" << std::endl;
    return 1;
  }

  Bytestream bts(inFile);

  if(headOpt.isSet() == tailOpt.isSet())
  {
    std::cerr << "Please give --head *or* --tail." << std::endl;
    return 1;
  }
  else if(headOpt.isSet())
  {
    if(!headOpt.isSet())
    {
      head = bts.size();
    }
    else if(head < 0)
    {
      head = bts.size() + head;
    }
    bts = bts.getBytestream(head);
  }
  else if(tailOpt.isSet())
  {
    if(!tailOpt.isSet())
    {
      tail = bts.size();
    }
    else if(tail < 0)
    {
      tail = bts.size() + tail;
    }
    bts += (bts.size() - tail);
    bts = bts.getBytestream(tail);
  }

  OutBinFile outFile(outFileName);

  outFile << bts;

}
