#include "ippprinter.h"
#include "curlrequester.h"
#include "stringsplit.h"

IppPrinter::IppPrinter(std::string addr) : _addr(addr)
{
  _error = refresh();
}

Error IppPrinter::refresh()
{
  IppMsg resp;
  Error error = _doRequest(IppMsg::GetPrinterAttrs, resp);
  _printerAttrs = resp.getPrinterAttrs();
  return error;
}

std::string IppPrinter::name()
{
  return _printerAttrs.get<std::string>("printer-name");
}

std::string IppPrinter::uuid()
{
  return _printerAttrs.get<std::string>("printer-uuid");
}

std::string IppPrinter::makeAndModel()
{
  return _printerAttrs.get<std::string>("printer-make-and-model");
}

std::string IppPrinter::location()
{
  return _printerAttrs.get<std::string>("printer-location");
}

int IppPrinter::state()
{
  return _printerAttrs.get<int>("printer-state");
}

std::string IppPrinter::stateMessage()
{
  return _printerAttrs.get<std::string>("printer-state-message");
}

List<std::string> IppPrinter::stateReasons()
{
  return _printerAttrs.getList<std::string>("printer-state-reasons");
}

List<std::string> IppPrinter::ippVersionsSupported()
{
  return _printerAttrs.getList<std::string>("ipp-versions-supported");
}

List<std::string> IppPrinter::ippFeaturesSupported()
{
  return _printerAttrs.getList<std::string>("ipp-features-supported");
}

int IppPrinter::pagesPerMinute()
{
  return _printerAttrs.get<int>("pages-per-minute");
}

int IppPrinter::pagesPerMinuteColor()
{
  return _printerAttrs.get<int>("pages-per-minute-color");
}

bool IppPrinter::identifySupported()
{
  return _printerAttrs.has("identify-actions-supported");
}

List<IppPrinter::Supply> IppPrinter::supplies()
{
  List<IppPrinter::Supply> supplies;
  List<std::string> names = _printerAttrs.getList<std::string>("marker-names");
  List<std::string> types = _printerAttrs.getList<std::string>("marker-types");
  List<std::string> colors = _printerAttrs.getList<std::string>("marker-colors");
  List<int> levels = _printerAttrs.getList<int>("marker-levels");
  List<int> lowLevels = _printerAttrs.getList<int>("marker-low-levels");
  List<int> highLevels = _printerAttrs.getList<int>("marker-high-levels");

  List<std::string>::iterator name = names.begin();
  List<std::string>::iterator type = types.begin();
  List<std::string>::iterator color = colors.begin();
  List<int>::iterator level = levels.begin();
  List<int>::iterator lowLevel = lowLevels.begin();
  List<int>::iterator highLevel = highLevels.begin();

  for(;
      name != names.end() && type != types.end() && color != colors.end() &&
      level != levels.end() && lowLevel != lowLevels.end() && highLevel != highLevels.end();
      name++, type++, color++, level++, lowLevel++, highLevel++)
  {
    List<std::string> colorList;
    for(std::string colorString : split_string(*color, "#"))
    {
      colorList.push_back("#" + colorString);
    }
    supplies.push_back({*name, *type, colorList, *level, *lowLevel, *highLevel});
  }
  return supplies;
}

List<IppPrinter::Firmware> IppPrinter::firmware()
{
  List<IppPrinter::Firmware> firmware;
  List<std::string> names = _printerAttrs.getList<std::string>("printer-firmware-name");
  List<std::string> versions = _printerAttrs.getList<std::string>("printer-firmware-string-version");

  for(List<std::string>::iterator name = names.begin(), version = versions.begin();
      name != names.end() && version != versions.end();
      name++, version++)
  {
    firmware.push_back({*name, *version});
  }
  return firmware;
}

List<std::string> IppPrinter::settableAttributes()
{
  return _printerAttrs.getList<std::string>("printer-settable-attributes-supported");
}

int IppPrinter::Supply::getPercent() const
{
  return (level*100.0)/(highLevel != 0 ? highLevel : 100);
}

bool IppPrinter::Supply::isLow() const
{
  return level <= lowLevel;
}

bool IppPrinter::Supply::operator==(const IppPrinter::Supply& other) const
{
  return other.name == name &&
         other.type == type &&
         other.colors == colors &&
         other.level == level &&
         other.lowLevel == lowLevel &&
         other.highLevel == highLevel;
}

bool IppPrinter::Firmware::operator==(const IppPrinter::Firmware& other) const
{
  return other.name == name &&
         other.version == version;
}

Error IppPrinter::identify()
{
  if(!identifySupported())
  {
    return Error("Identify not supported.");
  }
  IppMsg resp;
  Error error = _doRequest(IppMsg::IdentifyPrinter, resp);
  return error;
}

Error IppPrinter::setAttributes(List<std::pair<std::string, std::string>> attrStrs)
{
  Error err;
  IppAttrs attrs;
  for(const auto& [name, value] : attrStrs)
  {
    if(_printerAttrs.has(name))
    {
      IppTag tag = _printerAttrs.at(name).tag();
      try
      {
        attrs.set(name, IppAttr::fromString(value, tag));
      }
      catch(const std::exception& e)
      {
        return Error(e.what());
      }
    }
  }
  IppAttrs opAttrs = IppMsg::baseOpAttrs(_addr);
  IppMsg req(IppMsg::SetPrinterAttrs, opAttrs, {}, attrs);
  IppMsg resp;
  err = _doRequest(req, resp);
  return err;
}

Error IppPrinter::_doRequest(IppMsg::Operation op, IppMsg& resp)
{
  IppAttrs opAttrs = IppMsg::baseOpAttrs(_addr);
  IppMsg req(op, opAttrs);
  return _doRequest(req, resp);
}

Error IppPrinter::_doRequest(const IppMsg& req, IppMsg& resp)
{
  Error error;

  CurlIppPoster getPrinterAttrsReq(_addr, req.encode(), _ignoreSslErrors, _verbose);
  Bytestream getPrinterAttrsResult;
  CURLcode res0 = getPrinterAttrsReq.await(&getPrinterAttrsResult);
  if(res0 == CURLE_OK)
  {
    try
    {
      resp = IppMsg(getPrinterAttrsResult);
    }
    catch(const std::exception& e)
    {
      error = e.what();
    }
  }
  else
  {
    error = curl_easy_strerror(res0);
  }
  return error;
}
