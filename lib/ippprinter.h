#ifndef IPPPRINTER_H
#define IPPPRINTER_H

#include <string>
#include "converter.h"
#include "error.h"
#include "ippattr.h"
#include "ippmsg.h"
#include "ippprintjob.h"

class IppPrinter
{
public:
  struct Supply
  {
    std::string name;
    std::string type;
    List<std::string> colors;
    int level = 0;
    int lowLevel = 0;
    int highLevel = 0;
    int getPercent() const;
    bool isLow() const;
    bool operator==(const Supply& other) const;
  };
  struct Firmware
  {
    std::string name;
    std::string version;
    bool operator==(const Firmware& other) const;
  };
  struct Version
  {
    uint8_t minor = 1;
    uint8_t major = 1;
  };

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

  Error runJob(IppPrintJob job, std::string inFile, std::string inFormat, int pages, bool verbose);

  std::string name();
  std::string uuid();
  std::string makeAndModel();
  std::string location();
  int state();
  std::string stateMessage();
  List<std::string> stateReasons();
  List<std::string> ippVersionsSupported();
  List<std::string> ippFeaturesSupported();
  int pagesPerMinute();
  int pagesPerMinuteColor();
  List<Supply> supplies();
  List<Firmware> firmware();
  List<std::string> settableAttributes();
  List<Version> getVersions();

  bool identifySupported();
  Error identify();
  Error setAttributes(List<std::pair<std::string, std::string>> attrStrs);

private:
  Error _doRequest(IppMsg::Operation op, IppMsg& resp);
  Error _doRequest(const IppMsg& req, IppMsg& resp);
  IppMsg _mkMsg(uint16_t opOrStatus, IppAttrs opAttrs=IppAttrs(), IppAttrs jobAttrs=IppAttrs(), IppAttrs printerAttrs=IppAttrs());

  std::string _addr;
  bool _verbose = false;
  bool _ignoreSslErrors = true;

  Error _error;
  IppAttrs _printerAttrs;

  Error doPrint(IppPrintJob& job, std::string inFile, Converter::ConvertFun convertFun, Bytestream hdr, bool verbose);

};

#endif //IPPPRINTER_H