#include "baselinify.h"
#include "binfile.h"
#include "error.h"
#include "functions.h"
#include "ippprintjob.h"
#include "minimime.h"
#include "pdf2printable.h"
#include "stringutils.h"

#include <functional>
#include <map>
#include <optional>
#include <string>

class Converter
{
private:
  Converter() {}
public:
  Converter(const Converter&) = delete;
  Converter& operator=(const Converter &) = delete;
  Converter(Converter &&) = delete;
  Converter & operator=(Converter &&) = delete;

  static Converter& instance()
  {
    static Converter converter;
    return converter;
  }

  using ConvertKey = std::pair<std::string, std::string>;
  using ConvertFun = std::function<Error(const std::string& inFileName, const IppPrintJob& job,
                                         const WriteFun& writeFun, const ProgressFun& progressFun)>;

  ConvertFun Pdf2Printable =
    [](const std::string& inFileName, const IppPrintJob& job,
       const WriteFun& writeFun, const ProgressFun& progressFun)
    {
      return pdf_to_printable(inFileName, job.printParams, writeFun, progressFun);
    };

  ConvertFun Baselinify =
    [](const std::string& inFileName, const IppPrintJob&,
       const WriteFun& writeFun, const ProgressFun& progressFun)
    {
      InBinFile in(inFileName);
      if(!in)
      {
        return Error("Failed to open input");
      }
      Bytestream inBts(in);
      Bytestream baselinified;
      baselinify(inBts, baselinified);
      writeFun(std::move(baselinified));
      progressFun(1, 1);
      // We'll check on the cURL status in just a bit, so no point in returning errors here.
      return Error();
    };

  ConvertFun JustUpload =
    [](const std::string& inFileName, const IppPrintJob&,
       const WriteFun& writeFun, const ProgressFun& progressFun)
    {
      InBinFile in(inFileName);
      if(!in)
      {
        return Error("Failed to open input");
      }
      Bytestream inBts(in);
      writeFun(std::move(inBts));
      progressFun(1, 1);
      return Error();
    };

  ConvertFun FixupText =
  [](const std::string& inFileName, const IppPrintJob&,
     const WriteFun& writeFun, const ProgressFun& progressFun)
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

    writeFun(outString);
    progressFun(1, 1);
    return Error();
  };

  List<std::pair<ConvertKey, ConvertFun>> Pipelines
    {{{MiniMime::PDF, MiniMime::PDF}, Pdf2Printable},
     {{MiniMime::PDF, MiniMime::Postscript}, Pdf2Printable},
     {{MiniMime::PDF, MiniMime::PWG}, Pdf2Printable},
     {{MiniMime::PDF, MiniMime::URF}, Pdf2Printable},
     {{MiniMime::JPEG, MiniMime::JPEG}, Baselinify},
     {{"text/plain", "text/plain"}, FixupText}};

  std::optional<ConvertFun> getConvertFun(const std::string& inputFormat,
                                          const std::string& targetFormat)
  {
    for(const auto& [convertKey, convertFun] : Pipelines)
    {
      if(convertKey == ConvertKey {inputFormat, targetFormat})
      {
        return convertFun;
      }
    }
    if(inputFormat == targetFormat)
    {
      return JustUpload;
    }
    return {};
  }

  bool canConvert(const std::string& inputFormat, const std::string& targetFormat)
  {
    return (bool)getConvertFun(inputFormat, targetFormat);
  }

  std::optional<std::string> getTargetFormat(const std::string& inputFormat,
                                             const List<std::string>& supportedFormats)
  {
    for(const auto& [convertKey, convertFun] : Pipelines)
    {
      if(convertKey.first == inputFormat && supportedFormats.contains(convertKey.second))
      {
        return convertKey.second;
      }
    }
    if(supportedFormats.contains(inputFormat))
    {
      return inputFormat;
    }
    return {};
  }

  List<std::string> possibleInputFormats(const List<std::string>& supportedFormats)
  {
    List<std::string> inputFormats;
    for(const auto& [convertKey, convertFun] : Pipelines)
    {
      if(supportedFormats.contains(convertKey.second) && !inputFormats.contains(convertKey.first))
      {
        inputFormats.push_back(convertKey.first);
      }
    }
    for(const std::string& format : supportedFormats)
    {
      if(!inputFormats.contains(format))
      {
        inputFormats.push_back(format);
      }
    }
    return inputFormats;
  }
};
