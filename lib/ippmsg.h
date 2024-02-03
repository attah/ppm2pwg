#ifndef IPPMSG_H
#define IPPMSG_H

#include <map>
#include <list>
#include <string>
#include <bytestream.h>
#include "ippattr.h"

class IppMsg
{
public:

  enum Operation : uint16_t
  {
    PrintJob        = 0x0002,
    PrintUri        = 0x0003,
    ValidateJob     = 0x0004,
    CreateJob       = 0x0005,
    SendDocument    = 0x0006,
    SendUri         = 0x0007,
    CancelJob       = 0x0008,
    GetJobAttrs     = 0x0009,
    GetJobs         = 0x000A,
    GetPrinterAttrs = 0x000B,
    HoldJob         = 0x000C,
    ReleaseJob      = 0x000D,
    RestartJob      = 0x000E,
    PausePrinter    = 0x0010,
    ResumePrinter   = 0x0011,
    PurgeJobs       = 0x0012,
    IdentifyPrinter = 0x003C
  };

  enum Tag : uint8_t
  {
    OpAttrs             = 0x01,
    JobAttrs            = 0x02,
    EndAttrs            = 0x03,
    PrinterAttrs        = 0x04,
    UnsupportedAttrs    = 0x05,
    Unsupported         = 0x10,
    Integer             = 0x21,
    Boolean             = 0x22,
    Enum                = 0x23,
    OctetStringUnknown  = 0x30,
    DateTime            = 0x31,
    Resolution          = 0x32,
    IntegerRange        = 0x33,
    BeginCollection     = 0x34,
    TextWithLanguage    = 0x35,
    NameWithLanguage    = 0x36,
    EndCollection       = 0x37,
    TextWithoutLanguage = 0x41,
    NameWithoutLanguage = 0x42,
    Keyword             = 0x44,
    Uri                 = 0x45,
    UriScheme           = 0x46,
    Charset             = 0x47,
    NaturalLanguage     = 0x48,
    MimeMediaType       = 0x49,
    MemberName          = 0x4A
  };

  IppMsg() = default;
  IppMsg(Bytestream& msg);
  IppMsg(uint16_t opOrStatus, IppAttrs opAttrs, IppAttrs jobAttrs=IppAttrs(),
         uint8_t majVsn=1, uint8_t minVsn=1, IppAttrs printerAttrs=IppAttrs());
  IppMsg(const IppMsg& other) = default;
  ~IppMsg() = default;

  IppAttrs getPrinterAttrs() const {return _printerAttrs;}
  std::list<IppAttrs> getJobAttrs() const {return _jobAttrs;}
  IppAttrs getOpAttrs() const {return _opAttrs;}
  uint16_t getStatus() const {return _opOrStatus;}

  Bytestream encode() const;
  void setOpAttr(std::string name, IppAttr attr);

  static IppAttrs baseOpAttrs(std::string url);

  static void setReqId(uint32_t reqId)
  {
    _reqId = reqId;
  }

private:
  IppValue decodeValue(uint8_t tag, Bytestream& data) const;
  List<IppAttr> getUnnamedAttributes(Bytestream& data) const;
  IppValue collectAttributes(List<IppAttr>& attrs) const;
  std::string consumeAttributes(IppAttrs& attrs, Bytestream& data) const;
  void encodeAttributes(Bytestream& msg, const IppAttrs& attrs) const;
  void encodeAttribute(Bytestream& msg, std::string name, IppAttr attr, bool subCollection=false) const;
  void encodeValue(Bytestream& msg, uint8_t tag, IppValue val) const;

  uint16_t _opOrStatus = 0;

  uint8_t _majVsn = 1;
  uint8_t _minVsn = 1;

  IppAttrs _opAttrs;
  std::list<IppAttrs> _jobAttrs;
  IppAttrs _printerAttrs;

  static uint32_t _reqId;

};

#endif // IPPMSG_H
