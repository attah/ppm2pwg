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
  IppPrinter(IppAttrs printerAttrs) : _printerAttrs(std::move(printerAttrs))
  {}
  Error refresh();

  IppAttrs attributes() const
  {
    return _printerAttrs;
  }

  Error error() const
  {
    return _error;
  }

  IppPrintJob createJob() const
  {
    return IppPrintJob(_printerAttrs, additionalDocumentFormats());
  }

  Error runJob(IppPrintJob& job, const std::string& inFile, const std::string& inFormat, int pages,
               const ProgressFun& progressFun = noOpProgressfun);

  std::string name() const;
  std::string uuid() const;
  std::string makeAndModel() const;
  std::string location() const;
  int state() const;
  std::string stateMessage() const;
  List<std::string> stateReasons() const;
  List<std::string> ippVersionsSupported() const;
  List<std::string> ippFeaturesSupported() const;
  int pagesPerMinute() const;
  int pagesPerMinuteColor() const;
  List<Supply> supplies() const;
  List<Firmware> firmware() const;
  List<std::string> settableAttributes() const;
  List<std::string> icons() const;
  List<std::string> urisSupported() const;
  std::string strings() const;
  List<std::string> documentFormats() const;
  List<std::string> additionalDocumentFormats() const;
  List<std::string> possibleInputFormats() const;
  bool supportsPrinterRaster() const;

  bool isWarningState() const;

  bool identifySupported() const;
  Error identify() const;
  Error setAttributes(const List<std::pair<std::string, std::string>>& attrStrs);
  Error getJobs(List<JobInfo>& jobInfos) const;
  Error cancelJob(int jobId) const;

  void printJobId(bool printJobId)
  {
    _printJobId = printJobId;
  }

private:
  Error _doRequest(IppMsg::Operation op, IppMsg& resp) const;
  Error _doRequest(const IppMsg& req, IppMsg& resp) const;
  IppMsg _mkMsg(uint16_t opOrStatus,
                IppAttrs opAttrs=IppAttrs(),
                const IppAttrs& jobAttrs=IppAttrs(),
                const IppAttrs& printerAttrs=IppAttrs()) const;
  void _applyOverrides();

  std::string _addr;
  bool _ignoreSslErrors = true;
  bool _printJobId = false;

  Error _error;
  IppAttrs _printerAttrs;

  Error doPrint(IppPrintJob& job, const std::string& inFile, Bytestream&& hdr,
                const Converter::ConvertFun& convertFun, const ProgressFun& progressFun);
  Error doPrintToFile(IppPrintJob& job, const std::string& inFile,
                      const Converter::ConvertFun& convertFun, const ProgressFun& progressFun);

};

#endif //IPPPRINTER_H
