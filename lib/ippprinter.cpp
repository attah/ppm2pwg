#include "ippprinter.h"
#include "curlrequester.h"

IppPrinter::IppPrinter(std::string addr) : _addr(addr)
{
  _error = refresh();
}

Error IppPrinter::refresh()
{
  Error error;
  IppAttrs getPrinterAttrsJobAttrs = IppMsg::baseOpAttrs(_addr);
  IppMsg getPrinterAttrsMsg(IppMsg::GetPrinterAttrs, getPrinterAttrsJobAttrs);

  CurlIppPoster getPrinterAttrsReq(_addr, getPrinterAttrsMsg.encode(), _ignoreSslErrors, _verbose);
  Bytestream getPrinterAttrsResult;
  CURLcode res0 = getPrinterAttrsReq.await(&getPrinterAttrsResult);
  if(res0 == CURLE_OK)
  {
    try
    {
      IppMsg getPrinterAttrsResp(getPrinterAttrsResult);
      _printerAttrs = getPrinterAttrsResp.getPrinterAttrs();
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
