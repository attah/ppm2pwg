#include <bytestream.h>
#include <iostream>
#include <fstream>
#include "curlrequester.h"

// WIP: use ArgGet and stream from stdin before it's official

int main(int argc, char** argv)
{
  if(argc != 3)
  {
    return 1;
  }

  std::string addr = argv[1];
  std::string inFile = argv[2];

  std::ifstream ifs(inFile, std::ios::in | std::ios::binary);

  Bytestream hdr(ifs);
  Bytestream data(std::cin);

  CurlIppStreamer req(addr);

  req.write(hdr.raw(), hdr.size());
  req.write(data.raw(), data.size());

  Bytestream result;

  req.await(&result);

  std::cerr << "resp: \n" << result.hexdump(result.size()) << std::endl;

}
