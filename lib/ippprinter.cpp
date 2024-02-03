#include "ippprinter.h"
#include "curlrequester.h"

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

bool IppPrinter::identifySupported()
{
  return _printerAttrs.has("identify-actions-supported");
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

Error IppPrinter::_doRequest(IppMsg::Operation op, IppMsg& resp)
{
  Error error;
  IppAttrs getPrinterAttrsJobAttrs = IppMsg::baseOpAttrs(_addr);
  IppMsg getPrinterAttrsMsg(op, getPrinterAttrsJobAttrs);

  CurlIppPoster getPrinterAttrsReq(_addr, getPrinterAttrsMsg.encode(), _ignoreSslErrors, _verbose);
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
