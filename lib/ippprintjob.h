#ifndef IPPPARAMETERS_H
#define IPPPARAMETERS_H

#include "printparameters.h"
#include "ippmsg.h"
#include "setting.h"
#include "error.h"

class IppPrintJob
{
public:
  IppPrintJob(IppAttrs printerAttrs, List<std::string> additionalDocumentFormats = {})
  : _printerAttrs(printerAttrs), _additionalDocumentFormats(additionalDocumentFormats)
  {}

  IppPrintJob& operator=(const IppPrintJob& other)
  {
    opAttrs = other.opAttrs;
    jobAttrs = other.jobAttrs;
    printParams = other.printParams;
    _printerAttrs = other._printerAttrs;
    targetFormat = other.targetFormat;
    oneStage = other.oneStage;
    margins = other.margins;
    return *this;
  };

  IppPrintJob(const IppPrintJob& other)
  {
    *this = other;
  }

  ChoiceSetting<std::string> sides = ChoiceSetting<std::string>(&_printerAttrs, &jobAttrs, IppTag::Keyword, "sides");
  PreferredChoiceSetting<std::string> media = PreferredChoiceSetting<std::string>(&_printerAttrs, &jobAttrs, IppTag::Keyword, "media", "ready");

  IntegerSetting copies = IntegerSetting(&_printerAttrs, &jobAttrs, IppTag::Integer, "copies");
  ChoiceSetting<std::string> multipleDocumentHandling = ChoiceSetting<std::string>(&_printerAttrs, &jobAttrs, IppTag::Keyword, "multiple-document-handling");

  IntegerRangeListSetting pageRanges = IntegerRangeListSetting(&_printerAttrs, &jobAttrs, IppTag::IntegerRange, "page-ranges");

  IntegerChoiceSetting numberUp = IntegerChoiceSetting(&_printerAttrs, &jobAttrs, IppTag::Integer, "number-up");

  ChoiceSetting<std::string> colorMode = ChoiceSetting<std::string>(&_printerAttrs, &jobAttrs, IppTag::Keyword, "print-color-mode");
  ChoiceSetting<int> printQuality = ChoiceSetting<int>(&_printerAttrs, &jobAttrs, IppTag::Enum, "print-quality");
  ChoiceSetting<IppResolution> resolution = ChoiceSetting<IppResolution>(&_printerAttrs, &jobAttrs, IppTag::Resolution, "printer-resolution");
  ChoiceSetting<std::string> scaling = ChoiceSetting<std::string>(&_printerAttrs, &jobAttrs, IppTag::Keyword, "print-scaling");

  ChoiceSetting<std::string> documentFormat = ChoiceSetting<std::string>(&_printerAttrs, &opAttrs, IppTag::MimeMediaType, "document-format");
  ChoiceSetting<std::string> compression = ChoiceSetting<std::string>(&_printerAttrs, &opAttrs, IppTag::Keyword, "compression");

  ChoiceSetting<std::string> mediaType = ChoiceSetting<std::string>(&_printerAttrs, &jobAttrs, IppTag::Keyword, "media-type", "media-col");
  ChoiceSetting<std::string> mediaSource = ChoiceSetting<std::string>(&_printerAttrs, &jobAttrs, IppTag::Keyword, "media-source", "media-col");
  ChoiceSetting<std::string> outputBin = ChoiceSetting<std::string>(&_printerAttrs, &jobAttrs, IppTag::Keyword, "output-bin");
  ChoiceSetting<int> finishings = ChoiceSetting<int>(&_printerAttrs, &jobAttrs, IppTag::Enum, "finishings");

  ChoiceSetting<int> topMargin = ChoiceSetting<int>(&_printerAttrs, &jobAttrs, IppTag::Integer, "media-top-margin", "media-col");
  ChoiceSetting<int> bottomMargin = ChoiceSetting<int>(&_printerAttrs, &jobAttrs, IppTag::Integer, "media-bottom-margin", "media-col");
  ChoiceSetting<int> leftMargin = ChoiceSetting<int>(&_printerAttrs, &jobAttrs, IppTag::Integer, "media-left-margin", "media-col");
  ChoiceSetting<int> rightMargin = ChoiceSetting<int>(&_printerAttrs, &jobAttrs, IppTag::Integer, "media-right-margin", "media-col");

  Error finalize(std::string inputFormat, int pages=0);

  bool canSaveSettings();
  void restoreSettings();
  bool saveSettings();

  IppAttrs opAttrs;
  IppAttrs jobAttrs;
  PrintParameters printParams;

  std::string targetFormat;

  bool oneStage = false;

  struct Margins
  {
    int top = 0;
    int bottom = 0;
    int left = 0;
    int right = 0;
  };

  Margins margins;

private:

  std::string determineTargetFormat(std::string inputFormat);
  bool isImage(std::string format);
  void adjustRasterSettings(int pages);

  IppAttrs _printerAttrs;
  List<std::string> _additionalDocumentFormats;
};

#endif // IPPPARAMETERS_H
