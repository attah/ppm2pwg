#ifndef IPPPRINTER_H
#define IPPPRINTER_H

#include <string>
#include "error.h"
#include "ippattr.h"
#include "ippmsg.h"
#include "ippprintjob.h"

class IppPrinter
{
public:
  IppPrinter() = delete;
  IppPrinter(std::string addr);
  Error refresh();

  IppAttrs attributes()
  {
    return _printerAttrs;
  }

  Error error()
  {
    return _error;
  }

  IppPrintJob createJob()
  {
    return IppPrintJob(_printerAttrs);
  }

  bool identifySupported();
  Error identify();

private:
  Error _doRequest(IppMsg::Operation op, IppMsg& resp);

  std::string _addr;
  bool _verbose = false;
  bool _ignoreSslErrors = true;

  Error _error;
  IppAttrs _printerAttrs;

};

#endif //IPPPRINTER_H
