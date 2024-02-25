#include "ippprintjob.h"
#include "mediaposition.h"
#include "curlrequester.h"
#include "converter.h"
#include "stringutils.h"
#include "configdir.h"
#include <filesystem>

Error IppPrintJob::finalize(std::string inputFormat, int pages)
{
  targetFormat = determineTargetFormat(inputFormat);
  // Only set if regular supported format - else set OctetSteam
  if(documentFormat.getSupported().contains(targetFormat))
  {
    documentFormat.set(targetFormat);
  }
  else
  {
    documentFormat.set(MiniMime::OctetStream);
  }

  if(targetFormat == "" || targetFormat == MiniMime::OctetStream)
  {
    return Error("Failed to determine traget format");
  }

  if(MiniMime::isPrinterRaster(targetFormat))
  {
    List<std::string> compressionSupported = compression.getSupported();
    if(compressionSupported.contains("gzip"))
    {
      compression.set("gzip");
    }
    else if(compressionSupported.contains("deflate"))
    {
      compression.set("deflate");
    }
  }

  if(!printParams.setPaperSize(media.get(printParams.paperSizeName)))
  {
    return Error("Invalid paper size name");
  }

  // Only keep margin setting for image formats
  if(!MiniMime::isImage(targetFormat))
  {
    margins.top = topMargin.get();
    margins.bottom = bottomMargin.get();
    margins.left = leftMargin.get();
    margins.right = rightMargin.get();

    topMargin.unset();
    bottomMargin.unset();
    leftMargin.unset();
    rightMargin.unset();
  }

  if(jobAttrs.has("media-col") && media.isSet())
  {
    int x = printParams.getPaperSizeWInMillimeters()*100;
    int y = printParams.getPaperSizeHInMillimeters()*100;

    IppCollection dimensions {{"x-dimension", IppAttr(IppTag::Integer, x)},
                              {"y-dimension", IppAttr(IppTag::Integer, y)}};

    IppCollection mediaCol = jobAttrs.get<IppCollection>("media-col");
    mediaCol.set("media-size", IppAttr(IppTag::BeginCollection, dimensions));
    jobAttrs.set("media-col", IppAttr(IppTag::BeginCollection, mediaCol));
    media.unset();
  }

  if(inputFormat == MiniMime::PDF)
  {
    if(targetFormat == MiniMime::PDF)
    {
      printParams.format = PrintParameters::PDF;
    }
    else if(targetFormat == MiniMime::Postscript)
    {
      printParams.format = PrintParameters::Postscript;
    }
    else if(targetFormat == MiniMime::PWG)
    {
      printParams.format = PrintParameters::PWG;
    }
    else if(targetFormat == MiniMime::URF)
    {
      printParams.format = PrintParameters::URF;
    }
    else
    {
      printParams.format = PrintParameters::Invalid;
    }
  }
  else
  {
    printParams.format = PrintParameters::Invalid;
  }

  IppResolution ippResolution = resolution.get({printParams.hwResW, printParams.hwResH, IppResolution::DPI});
  printParams.hwResW = ippResolution.x;
  printParams.hwResH = ippResolution.y;

  // Effected locally for PDF (and upstream formats)
  if(inputFormat == MiniMime::PDF)
  {
    for(const IppIntRange& range : jobAttrs.getList<IppIntRange>("page-ranges"))
    {
      printParams.pageRangeList.push_back({range.low, range.high});
    }
    pageRanges.unset();
  }

  adjustRasterSettings(pages);

  if(sides.get() == "two-sided-long-edge")
  {
    printParams.duplexMode = PrintParameters::TwoSidedLongEdge;
  }
  else if(sides.get() == "two-sided-short-edge")
  {
    printParams.duplexMode = PrintParameters::TwoSidedShortEdge;
  }

  switch (printQuality.get())
  {
  case 3:
    printParams.quality = PrintParameters::DraftQuality;
    break;
  case 4:
    printParams.quality = PrintParameters::NormalQuality;
    break;
  case 5:
    printParams.quality = PrintParameters::HighQuality;
    break;
  default:
    printParams.quality = PrintParameters::DefaultQuality;
    break;
  }

  bool supportsColor = colorMode.getSupported().contains("color");

  printParams.colorMode = colorMode.get().find("color") != std::string::npos ? PrintParameters::sRGB24
                        : colorMode.get().find("monochrome") != std::string::npos
                          || !supportsColor ? PrintParameters::Gray8
                        : printParams.colorMode;

  std::map<std::string, PrintParameters::MediaPosition> MediaPositionMap MEDIA_POSITION_MAP;
  if(mediaSource.isSet())
  {
    printParams.mediaPosition = MediaPositionMap.at(mediaSource.get());
  }
  printParams.mediaType = mediaType.get();

  return Error();
}

std::string IppPrintJob::determineTargetFormat(std::string inputFormat)
{
  std::string targetFormat = documentFormat.get(MiniMime::OctetStream);
  bool canConvert = Converter::instance().canConvert(inputFormat, targetFormat);

  if(!documentFormat.isSet() && !canConvert)
  { // User made no choice, and we don't know the target format - treat as if auto
    targetFormat = MiniMime::OctetStream;
  }

  if(targetFormat == MiniMime::OctetStream)
  {
    List<std::string> supportedFormats = documentFormat.getSupported();
    supportedFormats += _additionalDocumentFormats;
    std::optional<std::string> betterFormat = Converter::instance().getTargetFormat(inputFormat, supportedFormats);
    if(betterFormat)
    {
      targetFormat = betterFormat.value();
    }
    else if(supportedFormats.contains(inputFormat))
    { // Last-resort: if we have a supported format in, assume we can send it as-is
      targetFormat = inputFormat;
    }
  }
  return targetFormat;
}

void IppPrintJob::adjustRasterSettings(int pages)
{
  if(printParams.format != PrintParameters::PWG && printParams.format != PrintParameters::URF)
  {
    return;
  }

  resolution.unset();

  if(printParams.format == PrintParameters::PWG)
  {
    uint32_t diff = std::numeric_limits<uint32_t>::max();
    uint32_t AdjustedHwResX = printParams.hwResW;
    uint32_t AdjustedHwResY = printParams.hwResH;

    for(const IppResolution& res : _printerAttrs.getList<IppResolution>("pwg-raster-document-resolution-supported"))
    {
      if(res.units != IppResolution::DPI)
      {
        continue;
      }
      uint32_t tmpDiff = std::abs(int(printParams.hwResW-res.x)) + std::abs(int(printParams.hwResH-res.y));
      if(tmpDiff < diff)
      {
        diff = tmpDiff;
        AdjustedHwResX = res.x;
        AdjustedHwResY = res.y;
      }
    }
    printParams.hwResW = AdjustedHwResX;
    printParams.hwResH = AdjustedHwResY;
  }
  else if(printParams.format == PrintParameters::URF)
  { // Ensure symmetric resolution for URF
    printParams.hwResW = printParams.hwResH = std::min(printParams.hwResW, printParams.hwResH);

    uint32_t diff = std::numeric_limits<uint32_t>::max();
    uint32_t AdjustedHwRes = printParams.hwResW;
    List<std::string> urfSupported = _printerAttrs.getList<std::string>("urf-supported");

    for(const std::string& us : urfSupported)
    {
      if(string_starts_with(us, "RS"))
      { //RS300[-600]
        std::string rs = us.substr(2);
        size_t pos = 0;
        while(pos <= rs.length())
        {
          size_t found = std::min(rs.length(), rs.find("-", pos));
          std::string substr(rs, pos, (found-pos));
          pos = found+1;

          int intRes = std::stoi(substr);
          uint32_t tmpDiff = std::abs(int(printParams.hwResW - intRes));
          if(tmpDiff < diff)
          {
            diff = tmpDiff;
            AdjustedHwRes = intRes;
          }
        }
        printParams.hwResW = printParams.hwResH = AdjustedHwRes;
        break;
      }
    }
  }

  if(printParams.format == PrintParameters::PWG)
  {
    std::string DocumentSheetBack = _printerAttrs.get<std::string>("pwg-raster-document-sheet-back");
    if(DocumentSheetBack=="flipped")
    {
      printParams.backXformMode=PrintParameters::Flipped;
    }
    else if(DocumentSheetBack=="rotated")
    {
      printParams.backXformMode=PrintParameters::Rotated;
    }
    else if(DocumentSheetBack=="manual-tumble")
    {
      printParams.backXformMode=PrintParameters::ManualTumble;
    }
  }
  else if(printParams.format == PrintParameters::URF)
  {
    List<std::string> urfSupported = _printerAttrs.getList<std::string>("urf-supported");
    if(urfSupported.contains("DM2"))
    {
      printParams.backXformMode=PrintParameters::Flipped;
    }
    else if(urfSupported.contains("DM3"))
    {
      printParams.backXformMode=PrintParameters::Rotated;
    }
    else if(urfSupported.contains("DM4"))
    {
      printParams.backXformMode=PrintParameters::ManualTumble;
    }
  }

  int copiesRequested = copies.get(1);
  // Actual (non-support) value can be 1, non-presense will be 0
  bool supportsCopies = _printerAttrs.get<IppIntRange>("copies-supported").high > 1;

  List<std::string> varyingAttributes = _printerAttrs.getList<std::string>("document-format-varying-attributes");
  if(varyingAttributes.contains("copies") || varyingAttributes.contains("copies-supported"))
  {
    supportsCopies = false;
  }

  if(copiesRequested > 1 && !supportsCopies)
  {
    std::string copyMode = multipleDocumentHandling.get();
    printParams.copies = copiesRequested;
    if(copyMode == "separate-documents-uncollated-copies")
    { // Only do silly copies if explicitly requested
      printParams.collatedCopies = false;
    }
    copies.unset();

    if(sides.get() != "one-sided")
    {
      PageSequence seq = printParams.getPageSequence(pages);
                             // No two different elements...
      bool singlePageRange = std::adjacent_find(seq.begin(), seq.end(), std::not_equal_to()) == seq.end();

      if(pages == 1 || singlePageRange)
      {
        sides.set("one-sided");
      }
    }
  }
}

std::filesystem::path settings_dir()
{
  return std::filesystem::path(CONFIG_DIR) / "saved_settings";
}

void IppPrintJob::restoreSettings()
{
  if(_printerAttrs.has("printer-uuid"))
  {
    std::string uuid = _printerAttrs.get<std::string>("printer-uuid");
    std::ifstream ifs = std::ifstream(settings_dir() / uuid, std::ios::in | std::ios::binary);
    if(ifs)
    {
      Bytestream bts(ifs);
      std::string errStr;
      Json::object jsonObj = Json::parse(bts.getString(bts.size()), errStr).object_items();
      Json::object opJson = jsonObj["op-attrs"].object_items();
      Json::object jobJson = jsonObj["job-attrs"].object_items();
      IppAttrs savedOpSettings = IppAttrs::fromJSON(opJson);
      IppAttrs savedJobSettings = IppAttrs::fromJSON(jobJson);
      for(const auto& [name, attr] : savedOpSettings)
      {
        opAttrs.insert_or_assign(name, attr);
      }
      for(const auto& [name, attr] : savedJobSettings)
      {
        jobAttrs.insert_or_assign(name, attr);
      }
    }
  }
}

bool IppPrintJob::saveSettings()
{
  if(_printerAttrs.has("printer-uuid"))
  {
    std::string uuid = _printerAttrs.get<std::string>("printer-uuid");
    std::filesystem::create_directories(settings_dir());
    std::ofstream ofs = std::ofstream(settings_dir() / uuid, std::ios::out | std::ios::binary);
    Json::object json;
    if(!opAttrs.empty())
    {
      json["op-attrs"] = opAttrs.toJSON();
    }
    if(!jobAttrs.empty())
    {
      json["job-attrs"] = jobAttrs.toJSON();
    }
    ofs << Json(json).dump();
    return true;
  }
  return false;
}
