#ifndef IPPPRINTER_H
#define IPPPRINTER_H

#include "converter.h"
#include "error.h"
#include "ippattr.h"
#include "ippmsg.h"
#include "ippprintjob.h"

#include <string>

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
  struct JobInfo
  {
    int id = 0;
    std::string name;
    int state = 0;
    std::string stateMessage;
  };

  IppPrinter(std::string addr, bool ignoreSslErrors=true);
  IppPrinter(IppAttrs printerAttrs) : _printerAttrs(printerAttrs)
  {}
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
    return IppPrintJob(_printerAttrs, additionalDocumentFormats());
  }

  Error runJob(IppPrintJob& job, const std::string& inFile, const std::string& inFormat, int pages, const ProgressFun& progressFun = noOpProgressfun);

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
  List<std::string> icons();
  std::string strings();
  List<std::string> documentFormats();
  List<std::string> additionalDocumentFormats();
  List<std::string> possibleInputFormats();
  bool supportsPrinterRaster();

  bool isWarningState();

  bool identifySupported();
  Error identify();
  Error setAttributes(const List<std::pair<std::string, std::string>>& attrStrs);
  Error getJobs(List<JobInfo>& jobInfos);
  Error cancelJob(int jobId);

  void printJobId(bool printJobId)
  {
    _printJobId = printJobId;
  }

private:
  Error _doRequest(IppMsg::Operation op, IppMsg& resp);
  Error _doRequest(const IppMsg& req, IppMsg& resp);
  IppMsg _mkMsg(uint16_t opOrStatus, IppAttrs opAttrs=IppAttrs(), const IppAttrs& jobAttrs=IppAttrs(), const IppAttrs& printerAttrs=IppAttrs());
  void _applyOverrides();

  std::string _addr;
  bool _ignoreSslErrors = true;
  bool _printJobId = false;

  Error _error;
  IppAttrs _printerAttrs;

  Error doPrint(IppPrintJob& job, const std::string& inFile, const Converter::ConvertFun& convertFun, Bytestream&& hdr, const ProgressFun& progressFun);
  Error doPrintToFile(IppPrintJob& job, const std::string& inFile, const Converter::ConvertFun& convertFun, const ProgressFun& progressFun);

};

#endif //IPPPRINTER_H
