#ifndef MINIMIME_H
#define MINIMIME_H

#include <string>

class MiniMime
{
public:

  MiniMime() = delete;

  static const char* OctetStream;

  static const char* PDF;
  static const char* Postscript;
  static const char* PWG;
  static const char* URF;

  static const char* PNG;
  static const char* GIF;
  static const char* JPEG;
  static const char* TIFF;

  static std::string getMimeType(const std::string& fileName);

  static bool isKnownImageFormat(const std::string& mimeType);
  static bool isImage(const std::string& mimeType);
  static bool isPrinterRaster(const std::string& mimeType);
  static bool isMultiPage(const std::string& mimeType);
  static std::string defaultExtension(const std::string& mimeType);
};

#endif // MINIMIME_H
