#ifndef MINIMIME_H
#define MINIMIME_H

#include <string>

class MiniMime
{
public:

  MiniMime() = delete;

  static const std::string OctetStream;

  static const std::string PDF;
  static const std::string Postscript;
  static const std::string PWG;
  static const std::string URF;

  static const std::string PNG;
  static const std::string GIF;
  static const std::string JPEG;
  static const std::string TIFF;

  static std::string getMimeType(std::string fileName);

  static bool isKnownImageFormat(std::string mimeType);
  static bool isImage(std::string mimeType);
  static bool isPrinterRaster(std::string mimeType);
  static bool isMultiPage(std::string mimeType);
};

#endif // MINIMIME_H
