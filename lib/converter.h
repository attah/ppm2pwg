#include <string>
#include <functional>
#include <optional>
#include <map>

#include "pdf2printable.h"
#include "baselinify.h"
#include "ippprintjob.h"
#include "stringsplit.h"
#include "functions.h"
#include "minimime.h"
#include "binfile.h"
#include "error.h"

class Converter
{
private:
  Converter() {}
public:
  Converter(const Converter&) = delete;
  Converter& operator=(const Converter &) = delete;
  Converter(Converter &&) = delete;
  Converter & operator=(Converter &&) = delete;

  static auto& instance()
  {
    static Converter converter;
    return converter;
  }

  typedef std::pair<std::string, std::string> ConvertKey;
  typedef std::function<Error(std::string inFileName, WriteFun writeFun, const IppPrintJob& job, ProgressFun progressFun, bool verbose)> ConvertFun;

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

  bool canConvert(std::string inputFormat, std::string targetFormat)
  {
    return Pipelines.find({inputFormat, targetFormat}) != Pipelines.end();
  }

  std::optional<ConvertFun> getConvertFun(std::string inputFormat, std::string targetFormat)
  {
    std::optional<ConvertFun> convertFun;
    std::map<ConvertKey, ConvertFun>::iterator pit =
      Pipelines.find(Converter::ConvertKey {inputFormat, targetFormat});

    if(pit != Converter::instance().Pipelines.end())
    {
      convertFun = pit->second;
    }
    else if(inputFormat == targetFormat)
    {
      convertFun = JustUpload;
    }
    return convertFun;
  }

  std::optional<std::string> getTargetFormat(std::string inputFormat, List<std::string> supportedFormats)
  {
    std::optional<std::string> targetFormat;
    for(const std::pair<const ConvertKey, ConvertFun>& p : Pipelines)
    {
      if(p.first.first == inputFormat && supportedFormats.contains(p.first.second))
      {
        targetFormat = p.first.second;
        break;
      }
    }
    return targetFormat;
  }
};
