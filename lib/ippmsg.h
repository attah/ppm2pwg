#ifndef IPPMSG_H
#define IPPMSG_H

#include "bytestream.h"
#include "ippattr.h"

#include <list>
#include <string>

class IppMsg
{
public:

  enum Operation : uint16_t
  {
    PrintJob                  = 0x0002,
    PrintUri                  = 0x0003,
    ValidateJob               = 0x0004,
    CreateJob                 = 0x0005,
    SendDocument              = 0x0006,
    SendUri                   = 0x0007,
    CancelJob                 = 0x0008,
    GetJobAttrs               = 0x0009,
    GetJobs                   = 0x000A,
    GetPrinterAttrs           = 0x000B,
    HoldJob                   = 0x000C,
    ReleaseJob                = 0x000D,
    RestartJob                = 0x000E,
    PausePrinter              = 0x0010,
    ResumePrinter             = 0x0011,
    PurgeJobs                 = 0x0012,
    SetPrinterAttrs           = 0x0013,
    SetJobAttrs               = 0x0014,
    GetPrinterSupportedValues = 0x0015,
    IdentifyPrinter           = 0x003C
  };

  IppMsg() = default;
  IppMsg(Bytestream& msg);
  IppMsg(uint16_t opOrStatus, const IppAttrs& opAttrs,
         const IppAttrs& jobAttrs=IppAttrs(), const IppAttrs& printerAttrs=IppAttrs());
  IppMsg(const IppMsg& other) = default;
  ~IppMsg() = default;

  IppAttrs getPrinterAttrs() const {return _printerAttrs;}
  std::list<IppAttrs> getJobAttrs() const {return _jobAttrs;}
  IppAttrs getOpAttrs() const {return _opAttrs;}
  uint16_t getStatus() const {return _opOrStatus;}

  Bytestream encode() const;
  void setOpAttr(const std::string& name, const IppAttr& attr);
  void setVersion(uint8_t majVsn, uint8_t minVsn);

  static IppAttrs baseOpAttrs(const std::string& url);

  static void setReqId(uint32_t reqId)
  {
    _reqId = reqId;
  }

private:
  IppValue decodeValue(IppTag tag, Bytestream& data) const;
  List<IppAttr> getUnnamedAttributes(Bytestream& data) const;
  IppValue collectAttributes(List<IppAttr>& attrs) const;
  std::string consumeAttributes(IppAttrs& attrs, Bytestream& data) const;
  void encodeAttributes(Bytestream& msg, const IppAttrs& attrs) const;
  void encodeAttribute(Bytestream& msg, std::string name, const IppAttr& attr,
                       bool subCollection=false) const;
  void encodeValue(Bytestream& msg, IppTag tag, const IppValue& val) const;

  uint16_t _opOrStatus = 0;

  uint8_t _majVsn = 1;
  uint8_t _minVsn = 1;

  IppAttrs _opAttrs;
  std::list<IppAttrs> _jobAttrs;
  IppAttrs _printerAttrs;

  static uint32_t _reqId;

};

#endif // IPPMSG_H
