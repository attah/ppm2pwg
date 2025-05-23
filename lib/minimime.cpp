#include "minimime.h"

#include "bytestream.h"
#include "stringutils.h"

#include <fstream>
#include <map>
#include <set>

const std::string MiniMime::OctetStream = "application/octet-stream";

const std::string MiniMime::PDF = "application/pdf";
const std::string MiniMime::Postscript = "application/postscript";
const std::string MiniMime::PWG = "image/pwg-raster";
const std::string MiniMime::URF = "image/urf";

const std::string MiniMime::PNG = "image/png";
const std::string MiniMime::GIF = "image/gif";
const std::string MiniMime::JPEG = "image/jpeg";
const std::string MiniMime::TIFF = "image/tiff";

std::string MiniMime::getMimeType(const std::string& fileName)
{
  std::ifstream ifs(fileName, std::ios::in | std::ios::binary);
  Bytestream bts(ifs, 7);

  if(bts.size() == 0)
  {
    return "";
  }
  else if(bts >>= "%PDF")
  {
    return PDF;
  }
  else if(bts >>= "%!PS")
  {
    return Postscript;
  }
  else if(bts >>= "RaS2")
  {
    return PWG;
  }
  else if(bts >>= "UNIRAST")
  {
    return URF;
  }
  else if(bts >>= "\x89PNG")
  {
    return PNG;
  }
  else if(bts >>= "GIF8")
  {
    return GIF;
  }
  else if(bts >>= "\xFF\xD8\xFF")
  {
    return JPEG;
  }
  else if((bts >>= "II*\x00") || (bts >>= "MM\x00*"))
  {
    return TIFF;
  }
  return OctetStream;
}

bool MiniMime::isKnownImageFormat(const std::string& mimeType)
{
  static std::set<std::string> knownImageFormats({PNG, GIF, JPEG, TIFF});
  return knownImageFormats.find(mimeType) != knownImageFormats.cend();
}

bool MiniMime::isImage(const std::string& mimeType)
{
  return string_starts_with(mimeType, "image/") && !isPrinterRaster(mimeType);
}

bool MiniMime::isPrinterRaster(const std::string& mimeType)
{
  static std::set<std::string> printerRasterFormats({PWG, URF});
  return printerRasterFormats.find(mimeType) != printerRasterFormats.cend();
}

bool MiniMime::isMultiPage(const std::string& mimeType)
{
  static std::set<std::string> multiPageFormats({PDF, Postscript, PWG, URF});
  return multiPageFormats.find(mimeType) != multiPageFormats.cend();
}

std::string MiniMime::defaultExtension(const std::string& mimeType)
{
  static std::map<std::string, std::string> extensions {{PDF, ".pdf"},
                                                        {Postscript, ".ps"},
                                                        {PWG, ".pwg"},
                                                        {URF, ".urf"},
                                                        {PNG, ".png"},
                                                        {GIF, ".gif"},
                                                        {JPEG, ".jpg"},
                                                        {TIFF, ".tiff"}};
  return extensions.at(mimeType);
}
