#ifndef IPPPARAMETERS_H
#define IPPPARAMETERS_H

#include "printparameters.h"
#include "pdf2printable.h"
#include "baselinify.h"
#include "ippmsg.h"
#include "binfile.h"
#include "setting.h"
#include "error.h"
#include "minimime.h"
#include "stringsplit.h"

class IppPrintJob
{
public:
  IppPrintJob(IppAttrs printerAttrs) : _printerAttrs(printerAttrs)
  {}

  ChoiceSetting<std::string> sides = ChoiceSetting<std::string>(&_printerAttrs, &jobAttrs, IppMsg::Keyword, "sides");
  PreferredChoiceSetting<std::string> media = PreferredChoiceSetting<std::string>(&_printerAttrs, &jobAttrs, IppMsg::Keyword, "media", "ready");

  IntegerSetting copies = IntegerSetting(&_printerAttrs, &jobAttrs, IppMsg::Integer, "copies");
  ChoiceSetting<std::string> multipleDocumentHandling = ChoiceSetting<std::string>(&_printerAttrs, &jobAttrs, IppMsg::Keyword, "multiple-document-handling");

  IntegerRangeListSetting pageRanges = IntegerRangeListSetting(&_printerAttrs, &jobAttrs, IppMsg::IntegerRange, "page-ranges");

  IntegerChoiceSetting numberUp = IntegerChoiceSetting(&_printerAttrs, &jobAttrs, IppMsg::Integer, "number-up");

  ChoiceSetting<std::string> colorMode = ChoiceSetting<std::string>(&_printerAttrs, &jobAttrs, IppMsg::Keyword, "print-color-mode");
  ChoiceSetting<int> printQuality = ChoiceSetting<int>(&_printerAttrs, &jobAttrs, IppMsg::Enum, "print-quality");
  ChoiceSetting<IppResolution> resolution = ChoiceSetting<IppResolution>(&_printerAttrs, &jobAttrs, IppMsg::Resolution, "printer-resolution");
  ChoiceSetting<std::string> scaling = ChoiceSetting<std::string>(&_printerAttrs, &jobAttrs, IppMsg::Keyword, "print-scaling");

  ChoiceSetting<std::string> documentFormat = ChoiceSetting<std::string>(&_printerAttrs, &opAttrs, IppMsg::MimeMediaType, "document-format");
  ChoiceSetting<std::string> compression = ChoiceSetting<std::string>(&_printerAttrs, &opAttrs, IppMsg::Keyword, "compression");

  ChoiceSetting<std::string> mediaType = ChoiceSetting<std::string>(&_printerAttrs, &jobAttrs, IppMsg::Keyword, "media-type", "media-col");
  ChoiceSetting<std::string> mediaSource = ChoiceSetting<std::string>(&_printerAttrs, &jobAttrs, IppMsg::Keyword, "media-source", "media-col");
  ChoiceSetting<std::string> outputBin = ChoiceSetting<std::string>(&_printerAttrs, &jobAttrs, IppMsg::Keyword, "output-bin");

  ChoiceSetting<int> topMargin = ChoiceSetting<int>(&_printerAttrs, &jobAttrs, IppMsg::Integer, "media-top-margin", "media-col");
  ChoiceSetting<int> bottomMargin = ChoiceSetting<int>(&_printerAttrs, &jobAttrs, IppMsg::Integer, "media-bottom-margin", "media-col");
  ChoiceSetting<int> leftMargin = ChoiceSetting<int>(&_printerAttrs, &jobAttrs, IppMsg::Integer, "media-left-margin", "media-col");
  ChoiceSetting<int> rightMargin = ChoiceSetting<int>(&_printerAttrs, &jobAttrs, IppMsg::Integer, "media-right-margin", "media-col");

  List<std::string> additionalDocumentFormats();

  typedef std::pair<std::string, std::string> ConvertKey;
  typedef std::function<Error(std::string inFileName, WriteFun writeFun, const IppPrintJob& job, ProgressFun progressFun, bool verbose)> ConvertFun;

  Error finalize(std::string inputFormat, int pages=0);
  Error run(std::string addr, std::string inFile, std::string inFormat, int pages=0, bool verbose=false);
  Error doPrint(std::string addr, std::string inFile, ConvertFun convertFun, Bytestream hdr, bool verbose);

  ConvertFun Pdf2Printable = [](std::string inFileName, WriteFun writeFun, const IppPrintJob& job, ProgressFun progressFun, bool verbose)
                             {
                               return pdf_to_printable(inFileName, writeFun, job.printParams, progressFun, verbose);
                             };

  ConvertFun Baselinify = [](std::string inFileName, WriteFun writeFun, const IppPrintJob&, ProgressFun progressFun, bool)
                          {
                            InBinFile in(inFileName);
                            if(!in)
                            {
                              return Error("Failed to open input");
                            }
                            Bytestream inBts(in);
                            Bytestream baselinified;
                            baselinify(inBts, baselinified);
                            writeFun(baselinified.raw(), baselinified.size());
                            progressFun(1, 1);
                            // We'll check on the cURL status in just a bit, so no point in returning errors here.
                            return Error();
                          };

  ConvertFun JustUpload = [](std::string inFileName, WriteFun writeFun, const IppPrintJob&, ProgressFun progressFun, bool)
                          {
                            InBinFile in(inFileName);
                            if(!in)
                            {
                              return Error("Failed to open input");
                            }
                            Bytestream inBts(in);
                            writeFun(inBts.raw(), inBts.size());
                            progressFun(1, 1);
                            return Error();
                          };

  ConvertFun FixupText = [](std::string inFileName, WriteFun writeFun, const IppPrintJob&, ProgressFun progressFun, bool)
                         {
                           InBinFile in(inFileName);
                           if(!in)
                           {
                             return Error("Failed to open input");
                           }
                           Bytestream inBts(in);
                           std::string allText = inBts.getString(inBts.size());

                           List<std::string> lines;
                           for(const std::string& rnline : split_string(allText, "\r\n"))
                           {
                             lines += split_string(rnline, "\n");
                           }
                           std::string outString = join_string(lines, "\r\n");

                           writeFun((uint8_t*)outString.c_str(), outString.length());
                           progressFun(1, 1);
                           return Error();
                         };

  std::map<ConvertKey, ConvertFun> Pipelines {{{MiniMime::PDF, MiniMime::PDF}, Pdf2Printable},
                                              {{MiniMime::PDF, MiniMime::Postscript}, Pdf2Printable},
                                              {{MiniMime::PDF, MiniMime::PWG}, Pdf2Printable},
                                              {{MiniMime::PDF, MiniMime::URF}, Pdf2Printable},
                                              {{MiniMime::JPEG, MiniMime::JPEG}, Baselinify},
                                              {{"text/plain", "text/plain"}, FixupText}};

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
};

#endif // IPPPARAMETERS_H
