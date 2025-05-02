#include "ippprinter.h"

#include "configdir.h"
#include "log.h"
#include "stringutils.h"

#include <filesystem>

IppPrinter::IppPrinter(Url addr, CurlRequester::SslConfig sslConfig)
: _addr(std::move(addr)), _sslConfig(std::move(sslConfig))
{
  _error = refresh();
}

Error IppPrinter::refresh()
{
  Error error;
  if(!_addr.isValid())
  {
    return Error("Invalid printer address.");
  }
  else if(_addr.getScheme() == "file")
  {
    std::ifstream ifs(_addr.getPath(), std::ios::in | std::ios::binary);
    if(!ifs)
    {
      return Error("Failed to open fake-printer file.");
    }
    Bytestream bts(ifs);
    try
    {
      if(bts.peek<int8_t>() == '{')
      {
        std::string errStr;
        Json json = Json::parse(bts.getString(bts.size()), errStr);
        if(!errStr.empty())
        {
          return Error(errStr);
        }
        _printerAttrs = IppAttrs::fromJSON(json.object_items());
      }
      else
      {
        IppMsg resp = IppMsg(bts);
        _printerAttrs = resp.getPrinterAttrs();
      }
    }
    catch(const std::exception& e)
    {
      error = e.what();
    }
  }
  else
  {
    IppMsg resp;
    error = _doRequest(IppMsg::GetPrinterAttrs, resp);
    _printerAttrs = resp.getPrinterAttrs();
  }
  _applyOverrides();
  return error;
}

Error IppPrinter::runJob(IppPrintJob& job, const std::string& inFile, const std::string& inFormat,
                         int pages, const ProgressFun& progressFun)
{
  Error error;
  try
  {
    List<int> supportedOperations = _printerAttrs.getList<int>("operations-supported");
    std::string fileName = std::filesystem::path(inFile).filename();

    error = job.finalize(inFormat, pages);
    if(error)
    {
      return error;
    }

    std::optional<Converter::ConvertFun> convertFun =
      Converter::instance().getConvertFun(inFormat, job.targetFormat);

    if(!convertFun)
    {
      return Error("No conversion method found for " + inFormat + " to " + job.targetFormat);
    }
    else if(_addr.getScheme() == "file")
    {
      doPrintToFile(job, inFile, convertFun.value(), progressFun);
    }
    else
    {
      if(!job.oneStage &&
        supportedOperations.contains(IppMsg::CreateJob) &&
        supportedOperations.contains(IppMsg::SendDocument))
      {
        IppAttrs createJobOpAttrs = {{"job-name", {IppTag::NameWithoutLanguage, fileName}}};
        IppMsg createJobMsg = _mkMsg(IppMsg::CreateJob, createJobOpAttrs, job.jobAttrs);
        IppMsg createJobResp;
        error = _doRequest(createJobMsg, createJobResp);

        if(error)
        {
          error = std::string("Create job failed: ") + error.value();
        }
        else
        {
          IppAttrs createJobRespJobAttrs;
          if(!createJobResp.getJobAttrs().empty())
          {
            createJobRespJobAttrs = createJobResp.getJobAttrs().front();
          }
          if(createJobResp.getStatus() <= 0xff && createJobRespJobAttrs.has("job-id"))
          {
            int jobId = createJobRespJobAttrs.get<int>("job-id");
            IppAttrs sendDocumentOpAttrs = job.opAttrs;
            sendDocumentOpAttrs.set("job-id", IppAttr {IppTag::Integer, jobId});
            sendDocumentOpAttrs.set("last-document", IppAttr {IppTag::Boolean, true});
            IppMsg sendDocMsg = _mkMsg(IppMsg::SendDocument, sendDocumentOpAttrs);
            error = doPrint(job, inFile, sendDocMsg.encode(), convertFun.value(), progressFun);
          }
          else
          {
            error = "Create job failed: "
                  + createJobResp.getOpAttrs().get<std::string>("status-message", "unknown");
          }
        }
      }
      else
      {
        IppAttrs printJobOpAttrs = job.opAttrs;
        printJobOpAttrs.set("job-name", IppAttr {IppTag::NameWithoutLanguage, fileName});
        IppMsg printJobMsg = _mkMsg(IppMsg::PrintJob, printJobOpAttrs, job.jobAttrs);
        error = doPrint(job, inFile, printJobMsg.encode(), convertFun.value(), progressFun);
      }
    }
  }
  catch(const std::exception& e)
  {
    error = std::string("Exception caught while printing: ") + e.what();
  }
  return error;
}

Error IppPrinter::doPrint(IppPrintJob& job, const std::string& inFile, Bytestream&& hdr,
                          const Converter::ConvertFun& convertFun, const ProgressFun& progressFun)
{
  Error error;
  CurlIppStreamer cr(_addr, _sslConfig);
  cr.write(std::move(hdr));

  if(job.compression.get() == "gzip")
  {
    cr.setCompression(CurlIppStreamer::Gzip);
  }
  else if(job.compression.get() == "deflate")
  {
    cr.setCompression(CurlIppStreamer::Deflate);
  }

  WriteFun writeFun([&cr](Bytestream&& data) -> bool
           {
             if(data.size() == 0)
             {
               return true;
             }
             return cr.write(std::move(data));
           });

  error = convertFun(inFile, job, writeFun, progressFun);
  if(error)
  {
    return error;
  }

  Bytestream result;
  CURLcode cres = cr.await(&result);
  if(cres == CURLE_OK)
  {
    IppMsg response(result);
    if(response.getStatus() < 0xff)
    {
      if(!response.getJobAttrs().empty() && _printJobId)
      {
        int jobId = response.getJobAttrs().front().get<int>("job-id");
        std::cout << "Job submitted successfully, with id: " << jobId << std::endl;
      }
    }
    else
    {
      error = "Print job failed: "
            + response.getOpAttrs().get<std::string>("status-message", "unknown");
    }
  }
  else
  {
    error = curl_easy_strerror(cres);
  }

  return error;
}

Error IppPrinter::doPrintToFile(IppPrintJob& job, const std::string& inFile,
                                const Converter::ConvertFun& convertFun,
                                const ProgressFun& progressFun)
{
  std::string fileName = "ippclient-debug-" + std::filesystem::path(inFile).filename().string();
  fileName += MiniMime::defaultExtension(job.targetFormat);
  std::filesystem::path tmpFile = std::filesystem::temp_directory_path() / fileName;
  std::ofstream ofs(tmpFile, std::ios::out | std::ios::binary);

  WriteFun writeFun([&ofs](Bytestream&& data) -> bool
           {
             ofs << data;
             return (bool)ofs;
           });

  Error error = convertFun(inFile, job, writeFun, progressFun);
  if(error)
  {
    return error;
  }
  std::cerr << "Wrote " << tmpFile.native() << std::endl;
  return Error();
}

std::string IppPrinter::name() const
{
  return _printerAttrs.get<std::string>("printer-name");
}

std::string IppPrinter::uuid() const
{
  return _printerAttrs.get<std::string>("printer-uuid");
}

std::string IppPrinter::makeAndModel() const
{
  return _printerAttrs.get<std::string>("printer-make-and-model");
}

std::string IppPrinter::location() const
{
  return _printerAttrs.get<std::string>("printer-location");
}

int IppPrinter::state() const
{
  return _printerAttrs.get<int>("printer-state");
}

std::string IppPrinter::stateMessage() const
{
  return _printerAttrs.get<std::string>("printer-state-message");
}

List<std::string> IppPrinter::stateReasons() const
{
  return _printerAttrs.getList<std::string>("printer-state-reasons");
}

List<std::string> IppPrinter::ippVersionsSupported() const
{
  return _printerAttrs.getList<std::string>("ipp-versions-supported");
}

List<std::string> IppPrinter::ippFeaturesSupported() const
{
  return _printerAttrs.getList<std::string>("ipp-features-supported");
}

int IppPrinter::pagesPerMinute() const
{
  return _printerAttrs.get<int>("pages-per-minute");
}

int IppPrinter::pagesPerMinuteColor() const
{
  return _printerAttrs.get<int>("pages-per-minute-color");
}

bool IppPrinter::identifySupported() const
{
  return _printerAttrs.has("identify-actions-supported");
}

List<IppPrinter::Supply> IppPrinter::supplies() const
{
  List<IppPrinter::Supply> supplies;
  List<std::string> names = _printerAttrs.getList<std::string>("marker-names");
  List<std::string> types = _printerAttrs.getList<std::string>("marker-types");
  List<std::string> colors = _printerAttrs.getList<std::string>("marker-colors");
  List<int> levels = _printerAttrs.getList<int>("marker-levels");
  List<int> lowLevels = _printerAttrs.getList<int>("marker-low-levels");
  List<int> highLevels = _printerAttrs.getList<int>("marker-high-levels");

  List<std::string>::const_iterator name = names.cbegin();
  List<std::string>::const_iterator type = types.cbegin();
  List<std::string>::const_iterator color = colors.cbegin();
  List<int>::const_iterator level = levels.cbegin();
  List<int>::const_iterator lowLevel = lowLevels.cbegin();
  List<int>::const_iterator highLevel = highLevels.cbegin();

  for(;
      name != names.cend() && type != types.cend() && color != colors.cend() &&
      level != levels.cend() && lowLevel != lowLevels.cend() && highLevel != highLevels.cend();
      name++, type++, color++, level++, lowLevel++, highLevel++)
  {
    List<std::string> colorList;
    for(const std::string& colorString : split_string(*color, "#"))
    {
      if(colorString != "")
      {
        colorList.push_back(colorString == "none" ? colorString : "#" + colorString);
      }
    }
    supplies.push_back({*name, *type, colorList, *level, *lowLevel, *highLevel});
  }
  return supplies;
}

List<IppPrinter::Firmware> IppPrinter::firmware() const
{
  List<IppPrinter::Firmware> firmware;
  List<std::string> names = _printerAttrs.getList<std::string>("printer-firmware-name");
  List<std::string> versions =
    _printerAttrs.getList<std::string>("printer-firmware-string-version");

  for(List<std::string>::const_iterator name = names.cbegin(), version = versions.cbegin();
      name != names.cend() && version != versions.cend();
      name++, version++)
  {
    firmware.push_back({*name, *version});
  }
  return firmware;
}

List<std::string> IppPrinter::settableAttributes() const
{
  return _printerAttrs.getList<std::string>("printer-settable-attributes-supported");
}

List<std::string> IppPrinter::icons() const
{
  return _printerAttrs.getList<std::string>("printer-icons");
}

List<std::string> IppPrinter::urisSupported() const
{
  return _printerAttrs.getList<std::string>("printer-uri-supported");
}

std::string IppPrinter::strings() const
{
  return _printerAttrs.get<std::string>("printer-strings-uri");
}

List<std::string> IppPrinter::documentFormats() const
{
  return _printerAttrs.getList<std::string>("document-format-supported");
}

List<std::string> IppPrinter::additionalDocumentFormats() const
{
  List<std::string> additionalFormats;
  List<std::string> baseFormats = documentFormats();
  std::string printerDeviceId = _printerAttrs.get<std::string>("printer-device-id");
  if(!baseFormats.contains(MiniMime::PDF) &&
     string_contains(printerDeviceId, "PDF"))
  {
    additionalFormats.push_back(MiniMime::PDF);
  }
  if(!baseFormats.contains(MiniMime::Postscript) &&
     (string_contains(printerDeviceId, "POSTSCRIPT") ||
      string_contains(printerDeviceId, "PostScript")))
  {
    additionalFormats.push_back(MiniMime::Postscript);
  }
  if(!baseFormats.contains(MiniMime::PWG) &&
     string_contains(printerDeviceId, "PWG"))
  {
    additionalFormats.push_back(MiniMime::PWG);
  }
  if(!baseFormats.contains(MiniMime::URF) &&
     (string_contains(printerDeviceId, "URF") ||
      string_contains(printerDeviceId, "AppleRaster")))
  {
    additionalFormats.push_back(MiniMime::URF);
  }
  return additionalFormats;
}

List<std::string> IppPrinter::possibleInputFormats() const
{
  return Converter::instance().possibleInputFormats(documentFormats()
         += additionalDocumentFormats());
}

bool IppPrinter::supportsPrinterRaster() const
{
  List<std::string> formats = documentFormats();
  return formats.contains(MiniMime::PWG) || formats.contains(MiniMime::URF);
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

bool IppPrinter::isWarningState() const
{
  if(state() > 4)
  {
    return true;
  }
  for(const std::string& reason : stateReasons())
  {
    if(reason != "none" && !(string_ends_with(reason, "-report")))
    {
      return true;
    }
  }
  return false;
}

Error IppPrinter::identify() const
{
  if(!identifySupported())
  {
    return Error("Identify not supported.");
  }
  IppMsg resp;
  Error error = _doRequest(IppMsg::IdentifyPrinter, resp);
  return error;
}

Error IppPrinter::setAttributes(const List<std::pair<std::string, std::string>>& attrStrs)
{
  Error error;
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
  IppMsg req = _mkMsg(IppMsg::SetPrinterAttrs, {}, {}, attrs);
  IppMsg resp;
  error = _doRequest(req, resp);
  return error;
}

Error IppPrinter::getJobs(List<IppPrinter::JobInfo>& jobInfos) const
{
  IppAttrs getJobsOpAttrs = {{"requested-attributes", {IppTag::Keyword, "all"}}};
  IppMsg req = _mkMsg(IppMsg::GetJobs, getJobsOpAttrs);
  IppMsg resp;
  Error error = _doRequest(req, resp);
  if(error)
  {
    return error;
  }
  for(const IppAttrs& jobAttrs : resp.getJobAttrs())
  {
    jobInfos.push_back(JobInfo {jobAttrs.get<int>("job-id"),
                                jobAttrs.get<std::string>("job-name"),
                                jobAttrs.get<int>("job-state"),
                                jobAttrs.get<std::string>("job-printer-state-message")});
  }
  return error;
}

Error IppPrinter::cancelJob(int jobId) const
{
  Error error;
  IppAttrs cancelJobOpAttrs = {{"job-id", {IppTag::Integer, jobId}}};
  IppMsg req = _mkMsg(IppMsg::CancelJob, cancelJobOpAttrs);
  IppMsg resp;
  error = _doRequest(req, resp);
  if(error)
  {
    return error;
  }
  if(resp.getStatus() > 0xff)
  {
    error = resp.getOpAttrs().get<std::string>("status-message", "unknown");
  }
  return error;
}

Error IppPrinter::_doRequest(IppMsg::Operation op, IppMsg& resp) const
{
  IppMsg req = _mkMsg(op);
  return _doRequest(req, resp);
}

Error IppPrinter::_doRequest(const IppMsg& req, IppMsg& resp) const
{
  Error error;

  DBG(<< "RERQUEST IPP operation: " << req.getStatus());
  DBG(<< "Operation attrs: " << req.getOpAttrs().toJSON().dump());
  for(const IppAttrs& jobAttrs : req.getJobAttrs())
  {
      DBG(<< "Job attrs: " << jobAttrs.toJSON().dump());
  }
  DBG(<< "Printer attrs: " << req.getPrinterAttrs().toJSON().dump());

  CurlIppPoster reqPoster(_addr, req.encode(), _sslConfig);
  Bytestream respBts;
  CURLcode res0 = reqPoster.await(&respBts);
  if(res0 == CURLE_OK)
  {
    try
    {
      resp = IppMsg(respBts);
      DBG(<< "RESPONSE IPP status: " << resp.getStatus());
      DBG(<< "Operation attrs: " << resp.getOpAttrs().toJSON().dump());
      for(const IppAttrs& jobAttrs : resp.getJobAttrs())
      {
          DBG(<< "Job attrs: " << jobAttrs.toJSON().dump());
      }
      DBG(<< "Printer attrs: " << resp.getPrinterAttrs().toJSON().dump());
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

IppMsg IppPrinter::_mkMsg(uint16_t opOrStatus, IppAttrs opAttrs,
                          const IppAttrs& jobAttrs, const IppAttrs& printerAttrs) const
{
  IppAttrs baseOpAttrs = IppMsg::baseOpAttrs(_addr);
  opAttrs.insert(baseOpAttrs.cbegin(), baseOpAttrs.cend());

  IppMsg msg(opOrStatus, opAttrs, jobAttrs, printerAttrs);
  if(ippVersionsSupported().contains("2.0"))
  {
    msg.setVersion(2, 0);
  }
  return msg;
}

void IppPrinter::_applyOverrides()
{
  try
  {
    std::ifstream ifs(CONFIG_DIR + "/overrides", std::ios::in | std::ios::binary);
    if(ifs)
    {
      Bytestream bts(ifs);
      std::string errStr;
      Json overridesJson = Json::parse(bts.getString(bts.size()), errStr);
      if(!errStr.empty())
      {
        std::cerr << "Bad overrides file: " << errStr << std::endl;
      }
      for(const auto& [matchAttrName, matchAttr] : overridesJson.object_items())
      {
        for(const auto& [matchAttrValue, overrideObj] : matchAttr.object_items())
        {
          if(_printerAttrs.hasWithValue(matchAttrName, matchAttrValue))
          {
            IppAttrs overrideAttrs = IppAttrs::fromJSON(overrideObj.object_items());
            DBG(<< "Overriding printer attributes: " << overrideAttrs.toJSON().dump());
            for(const auto& [name, attr] : overrideAttrs)
            {
              _printerAttrs.insert_or_assign(name, attr);
            }
          }
        }
      }
    }
  }
  catch(const std::exception& e)
  {
      std::cerr << "Exception applying overrides: " << e.what() << std::endl;
  }
}
