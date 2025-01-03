#include <filesystem>
#include <iostream>
#include <regex>

#include <poppler.h>
#include <poppler-document.h>

#include "argget.h"
#include "bytestream.h"
#include "ippprinter.h"
#include "log.h"
#include "minimime.h"
#include "pointer.h"

#define HELPTEXT ""

inline void print_error(const std::string& hint, const std::string& argHelp)
{
  std::cerr << hint << std::endl << std::endl << argHelp << std::endl << HELPTEXT << std::endl;
}

std::string print_colors(const List<std::string>& colors)
{
  std::stringstream res;
  std::smatch match;
  const std::regex colorRegex("^#([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})$");
  for(const std::string& color : colors)
  {
    if(std::regex_match(color, match, colorRegex))
    {
      unsigned long r = std::stoul(match[1], nullptr, 16);
      unsigned long g = std::stoul(match[2], nullptr, 16);
      unsigned long b = std::stoul(match[3], nullptr, 16);
      res << "\x1b[48;2;" << r << ";" << g << ";" << b << "m" << " " << "\x1b[0m";
    }
  }
  return res.str();
}

template<typename T, typename U>
T pos_clamp(U value)
{
  return std::min<U>(value, std::numeric_limits<T>::max());
}

std::ostream& operator<<(std::ostream& os, const List<IppPrinter::Supply>& supplies)
{
  for(List<IppPrinter::Supply>::const_iterator supply = supplies.cbegin(); supply != supplies.cend(); supply++)
  {
    std::string color = print_colors(supply->colors);
    os << "* " << supply->name << std::endl
       << "  " << ((color != "") ? color+" " : "")
       << supply->getPercent() << "%" << (supply->isLow() ? "(low)" : "")
       << " " << supply->type;
    if(std::next(supply) != supplies.cend())
    {
      os << std::endl;
    }
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const List<IppPrinter::Firmware>& firmwares)
{
  for(List<IppPrinter::Firmware>::const_iterator firmware = firmwares.cbegin(); firmware != firmwares.cend(); firmware++)
  {
    os << firmware->name << ": " << firmware->version;
    if(std::next(firmware) != firmwares.cend())
    {
      os << std::endl;
    }
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const List<std::string>& sl)
{
  os << join_string(sl, ", ");
  return os;
}

std::string resolution_list(const List<IppResolution>& l)
{
  bool first = true;
  std::stringstream ss;
  for(const IppResolution& r : l)
  {
    if(r.units != IppResolution::DPI)
    {
      continue;
    }
    ss << (first ? "" : ",") << r.toStr();
  }

  if(ss.str().empty())
  {
    ss << "empty";
  }
  return ss.str();
}

std::ostream& operator<<(std::ostream& os, const List<IppPrinter::JobInfo>& jobInfos)
{
  for(const IppPrinter::JobInfo& jobInfo : jobInfos)
  {
    os << jobInfo.id << ": " << jobInfo.name << std::endl;
    os << "State: " << jobInfo.state;
    if(jobInfo.stateMessage != "")
    {
      os << " (" << jobInfo.stateMessage << ")";
    }
    os << std::endl;
  }
  return os;
}

template <typename T>
void print_if_set(const std::string& title, const T& value)
{
  if(value != T())
  {
    std::cout << title << std::endl << value << std::endl << std::endl;
  }
}

template <typename O, typename S, typename V>
void set_or_fail(const O& opt, S& setting, const V& value, bool force)
{
  std::string docName = opt.docName() + " (ipp: " + setting.name() + ")";
  set_or_fail(opt.isSet(), docName, "Valid values are: " + setting.supportedStr(), setting, value, force);
}

template <typename S, typename V>
void set_or_fail(bool isSet, const std::string& docName, const std::string& inputHint, S& setting, const V& value, bool force)
{
  if(isSet)
  {
    if(setting.isSupportedValue(value) || force)
    {
      setting.set(value);
    }
    else
    {
      std::cerr << "Argument " << docName << " ";
      if(!setting.isSupported())
      {
        std::cerr << "is not supported by this printer." << std::endl;
      }
      else
      {
        std::cerr << "has an invalid value. " << std::endl << inputHint << std::endl;
      }
      exit(1);
    }
  }
}

int main(int argc, char** argv)
{
  bool help = false;
  bool verbose = false;
  bool force = false;
  bool oneStage = false;
  std::string pages;
  int copies;
  std::string collatedCopies;
  int numberUp;
  std::string paperSize;
  uint32_t hwRes;
  uint32_t hwResX;
  uint32_t hwResY;
  std::string sides;
  std::string colorMode;
  int quality;

  std::string scaling;
  std::string format;
  std::string mimeType;
  std::string mediaType;
  std::string mediaSource;
  std::string outputBin;

  int finishing;
  int margin;
  int topMargin;
  int bottomMargin;
  int leftMargin;
  int rightMargin;

  bool antiAlias;
  bool printJobId = false;
  bool save = false;

  int id;

  std::string addr;
  std::string attrs;
  std::string inFile;

  SwitchArg<bool> helpOpt(help, {"-h", "--help"}, "Print this help text");
  SwitchArg<bool> verboseOpt(verbose, {"-v", "--verbose"}, "Be verbose, print headers and progress");
  SwitchArg<bool> forceOpt(force, {"-f", "--force"}, "Force use of unsupported options");
  SwitchArg<bool> oneStageOpt(oneStage, {"--one-stage"}, "Force use of one-stage print job");

  SwitchArg<std::string> pagesOpt(pages, {"-p", "--pages"}, "What pages to process, e.g.: 1,17-42,69-");
  SwitchArg<int> copiesOpt(copies, {"--copies"}, "Number of copies to output");
  EnumSwitchArg<std::string> collatedCopiesOpt(collatedCopies, {{"yes", "separate-documents-collated-copies"},
                                                                {"no", "separate-documents-uncollated-copies"}},
                                               {"--collated-copies"}, "Request collated copies (yes/no)");

  SwitchArg<int> numberUpOpt(numberUp, {"-n", "--number-up"}, "Pager per page (as per IPP)");

  SwitchArg<std::string> paperSizeOpt(paperSize, {"--paper-size"}, "Paper size to output, e.g.: iso_a4_210x297mm");
  SwitchArg<uint32_t> resolutionOpt(hwRes, {"-r", "--resolution"}, "Resolution (in DPI) for rasterization");
  SwitchArg<uint32_t> resolutionXOpt(hwResX, {"-rx", "--resolution-x"}, "Resolution (in DPI) for rasterization, x-axis");
  SwitchArg<uint32_t> resolutionYOpt(hwResY, {"-ry", "--resolution-y"}, "Resolution (in DPI) for rasterization, y-axis");

  SwitchArg<std::string> sidesOpt(sides, {"-s", "--sides"}, "Sides (as per IPP)");
  SwitchArg<std::string> colorModeOpt(colorMode, {"-c", "--color-mode"}, "Color mode (as per IPP)");
  EnumSwitchArg<int> qualityOpt(quality, {{"draft", PrintParameters::DraftQuality},
                                          {"normal", PrintParameters::NormalQuality},
                                          {"high", PrintParameters::HighQuality}},
                                          {"-q", "--quality"},
                                          "Quality (draft/normal/high)");
  SwitchArg<std::string> scalingOpt(scaling, {"--scaling"}, "Scaling (as per IPP)");

  SwitchArg<std::string> formatOpt(format, {"--format"}, "Transfer format");
  SwitchArg<std::string> mimeTypeOpt(mimeType, {"--mime-type"}, "Input file mime-type");

  SwitchArg<std::string> mediaTypeOpt(mediaType, {"--media-type"}, "Media type (as per IPP)");
  SwitchArg<std::string> mediaSourceOpt(mediaSource, {"--media-source"}, "Media source (as per IPP)");
  SwitchArg<std::string> outputBinOpt(outputBin, {"--output-bin"}, "Output bin (as per IPP)");
  SwitchArg<int> finishingsOpt(finishing, {"--finishings"}, "Finishing to apply (as per IPP)");

  SwitchArg<int> marginOpt(margin, {"-m", "--margin"}, "Margin (as per IPP)");
  SwitchArg<int> topMarginOpt(topMargin, {"-tm", "--top-margin"}, "Top margin (as per IPP)");
  SwitchArg<int> bottomMarginOpt(bottomMargin, {"-bm", "--bottom-margin"}, "Bottom margin (as per IPP)");
  SwitchArg<int> leftMarginOpt(leftMargin, {"-lm", "--left-margin"}, "Left margin (as per IPP)");
  SwitchArg<int> rightMarginOpt(rightMargin, {"-rm", "--right-margin"}, "Right margin (as per IPP)");

  SwitchArg<bool> antiAliasOpt(antiAlias, {"-aa", "--antialias"}, "Enable antialiasing in rasterization");
  SwitchArg<bool> printJobIdOpt(printJobId, {"--print-job-id"}, "Print job id on successful submission");
  SwitchArg<bool> saveOpt(save, {"--save"}, "Save options as local defaults for future jobs");

  SwitchArg<int> idOpt(id, {"--id"}, "Id of print job.");

  PosArg addrArg(addr, "printer address");
  PosArg attrsArg(attrs, "name=value[,name=value]");
  PosArg pdfArg(inFile, "input file");

  SubArgGet args({{"info", {{&helpOpt, &verboseOpt},
                            {&addrArg}}},
                  {"identify", {{&helpOpt, &verboseOpt},
                                {&addrArg}}},
                  {"get-attrs", {{&helpOpt, &verboseOpt},
                                 {&addrArg}}},
                  {"set-attrs", {{&helpOpt, &verboseOpt},
                                 {&addrArg, &attrsArg}}},
                  {"get-jobs", {{&helpOpt, &verboseOpt},
                                {&addrArg}}},
                  {"cancel-job", {{&helpOpt, &verboseOpt, &idOpt},
                                  {&addrArg}}},
                  {"print", {{&helpOpt, &verboseOpt, &forceOpt, &oneStageOpt,
                              &pagesOpt, &copiesOpt, &collatedCopiesOpt, &numberUpOpt, &paperSizeOpt,
                              &resolutionOpt, &resolutionXOpt, &resolutionYOpt,
                              &sidesOpt, &colorModeOpt, &qualityOpt, &scalingOpt,
                              &formatOpt, &mimeTypeOpt,
                              &mediaTypeOpt, &mediaSourceOpt, &outputBinOpt, &finishingsOpt,
                              &marginOpt, &topMarginOpt, &bottomMarginOpt, &leftMarginOpt, &rightMarginOpt,
                              &antiAliasOpt, &printJobIdOpt, &saveOpt},
                             {&addrArg, &pdfArg}}}});

  bool correctArgs = args.get_args(argc, argv);
  if(help)
  {
    std::cout << args.argHelp(args.subCommand()) << std::endl << HELPTEXT << std::endl;
    return 0;
  }
  else if(!correctArgs)
  {
    print_error(args.errmsg(), args.argHelp(args.subCommand()));
    return 1;
  }

  if(verbose)
  {
    LogController::instance().enable(LogController::Debug);
  }

  IppPrinter printer(addr);
  Error error = printer.error();
  if(error)
  {
    std::cerr << "Could not get printer attributes: " << error.value() << std::endl;
    return 1;
  }

  if(args.subCommand() == "info")
  {
    print_if_set("Name:", printer.name());
    print_if_set("Make and model:", printer.makeAndModel());
    print_if_set("Location:", printer.location());
    print_if_set("UUID:", printer.uuid());
    print_if_set("Printer state message:", printer.stateMessage());
    print_if_set("Printer state reasons:", join_string(printer.stateReasons(), "\n"));
    print_if_set("IPP versions:", join_string(printer.ippVersionsSupported(), ", "));
    print_if_set("IPP features:", join_string(printer.ippFeaturesSupported(), "\n"));
    print_if_set("Pages per minute:", printer.pagesPerMinute());
    print_if_set("Pages per minute (color):", printer.pagesPerMinuteColor());
    print_if_set("Supplies:", printer.supplies());
    print_if_set("Firmware:", printer.firmware());
    print_if_set("Settable attributes:", join_string(printer.settableAttributes(), "\n"));
    print_if_set("Icons:", join_string(printer.icons(), "\n"));
    print_if_set("Strings:", printer.strings());
    print_if_set("Document formats (native):", join_string(printer.documentFormats(), "\n"));
    print_if_set("Document formats (guessed):", join_string(printer.additionalDocumentFormats(), "\n"));
    print_if_set("Document formats (with conversion):", join_string(printer.possibleInputFormats(), "\n"));
  }
  else if(args.subCommand() == "identify")
  {
    error = printer.identify();
    if(error)
    {
      std::cerr << "Identify failed: " << error.value() << std::endl;
      return 1;
    }
  }
  else if(args.subCommand() == "get-attrs")
  {
    std::cout << printer.attributes().toJSON().dump();
  }
  else if(args.subCommand() == "set-attrs")
  {
    List<std::pair<std::string, std::string>> kvs;
    std::smatch match;
    const std::regex kvRegex("^([^=]*)=([^=]*)$");
    for(const std::string& kv : split_string(attrs, ","))
    {
      if(std::regex_match(kv, match, kvRegex))
      {
        kvs.push_back({match[1], match[2]});
      }
      else
      {
        std::cerr << "Bad key:value: " << kv << std::endl;
        return 1;
      }
    }
    error = printer.setAttributes(kvs);
    if(error)
    {
      std::cerr << "Set attributes failed: " << error.value() << std::endl;
      return 1;
    }
  }
  else if(args.subCommand() == "get-jobs")
  {
    List<IppPrinter::JobInfo> jobInfos;
    error = printer.getJobs(jobInfos);
    if(error)
    {
      std::cerr << "Get-jobs failed: " << error.value() << std::endl;
      return 1;
    }
    std::cout << jobInfos;
  }
  else if(args.subCommand() == "cancel-job")
  {
    if(!idOpt.isSet())
    {
        std::cerr << "Please specify a job id with --id." << std::endl;
        return 1;
    }
    error = printer.cancelJob(id);
    if(error)
    {
      std::cerr << "Cancel-job failed: " << error.value() << std::endl;
      return 1;
    }
  }
  else if(args.subCommand() == "print")
  {
    IppPrintJob job = printer.createJob();
    if(!save)
    {
      job.restoreSettings();
    }

    if(oneStageOpt.isSet())
    {
      job.oneStage = oneStage;
    }

    if(antiAliasOpt.isSet())
    {
      job.printParams.antiAlias = antiAlias;
    }

    if(!mimeTypeOpt.isSet())
    {
      if(inFile != "-")
      {
        mimeType = MiniMime::getMimeType(inFile);
      }
      if(mimeType == "" || mimeType == MiniMime::OctetStream)
      {
        std::cerr << "Failed to determine input file format. (Specify with --mime-type)." << std::endl;
        return 1;
      }
    }

    if(pagesOpt.isSet())
    {
      PageRangeList pageRanges = PrintParameters::parsePageSelection(pages);
      if(pageRanges.empty())
      {
        print_error("Invalid page range.", args.argHelp(args.subCommand()));
        return 1;
      }
      IppOneSetOf ippPageRanges;
      for(const auto& [first, last]: pageRanges)
      {
        ippPageRanges.push_back(IppIntRange {pos_clamp<int32_t>(first), pos_clamp<int32_t>(last)});
      }
      job.pageRanges.set(ippPageRanges);
    }

    set_or_fail(copiesOpt, job.copies, copies, force || printer.supportsPrinterRaster());
    set_or_fail(collatedCopiesOpt, job.multipleDocumentHandling, collatedCopies, force);
    set_or_fail(numberUpOpt, job.numberUp, numberUp, force);
    set_or_fail(paperSizeOpt, job.media, paperSize, force);

    if(resolutionXOpt.isSet() != resolutionYOpt.isSet())
    {
        print_error("Resolution must be specified for both dimensions.", args.argHelp(args.subCommand()));
        return 1;
    }

    std::string resolutionDoc = resolutionOpt.docName() + " (ipp: " + job.resolution.name() + ")";
    std::string resolutionSupported = "Valid values are: " + resolution_list(job.resolution.getSupported());
    set_or_fail(resolutionOpt.isSet(), resolutionDoc, resolutionSupported,
                job.resolution, IppResolution {hwRes, hwRes, IppResolution::DPI}, force);

    std::string resolutionDoc2 = resolutionXOpt.docName() + " and/or " + resolutionYOpt.docName()
                              + " (ipp: " + job.resolution.name() + ")";
    set_or_fail(resolutionXOpt.isSet() && resolutionYOpt.isSet(), resolutionDoc2, resolutionSupported,
                job.resolution, IppResolution {hwResX, hwResY, IppResolution::DPI}, force);

    set_or_fail(sidesOpt, job.sides, sides, force);
    set_or_fail(colorModeOpt, job.colorMode, colorMode, force);
    List<std::string> supportedQuality;
    for(int q : job.printQuality.getSupported())
    {
      supportedQuality.push_back(qualityOpt.getAlias(q));
    }
    std::string qualityHint = "Valid values are: " + join_string(supportedQuality, ", ");
    set_or_fail(qualityOpt.isSet(), qualityOpt.docName(), qualityHint, job.printQuality, quality, force);
    set_or_fail(scalingOpt, job.scaling, scaling, force);
    set_or_fail(formatOpt, job.documentFormat, format, force);
    set_or_fail(mediaTypeOpt, job.mediaType, mediaType, force);
    set_or_fail(mediaSourceOpt, job.mediaSource, mediaSource, force);
    set_or_fail(outputBinOpt, job.outputBin, outputBin, force);
    set_or_fail(finishingsOpt, job.finishings, finishing, force);

    if(marginOpt.isSet())
    {
      if(!force && (!job.topMargin.isSupported() || !job.bottomMargin.isSupported() ||
                    !job.leftMargin.isSupported() || !job.rightMargin.isSupported()))
      {
        std::cerr << "Argument " << marginOpt.docName() << " (ipp: margin-*) "
                  << "is not supported by this printer." << std::endl;
        exit(1);
      }
      set_or_fail(true, marginOpt.docName(),
                  "Valid values for top margin are: " + job.topMargin.supportedStr(),
                  job.topMargin, margin, force);
      set_or_fail(true, marginOpt.docName(),
                  "Valid values for bottom margin are: " + job.bottomMargin.supportedStr(),
                  job.bottomMargin, margin, force);
      set_or_fail(true, marginOpt.docName(),
                  "Valid values for left margin are: " + job.leftMargin.supportedStr(),
                  job.leftMargin, margin, force);
      set_or_fail(true, marginOpt.docName(),
                  "Valid values for right margin are: " + job.rightMargin.supportedStr(),
                  job.rightMargin, margin, force);
    }

    set_or_fail(topMarginOpt, job.topMargin, topMargin, force);
    set_or_fail(bottomMarginOpt, job.bottomMargin, bottomMargin, force);
    set_or_fail(leftMarginOpt, job.leftMargin, leftMargin, force);
    set_or_fail(rightMarginOpt, job.rightMargin, rightMargin, force);

    int nPages = 0; // Unknown - assume multiple if the format allows

    if(mimeType == MiniMime::PDF)
    {
      GError* error = nullptr;
      inFile = std::filesystem::absolute(inFile);
      std::string url("file://");
      url.append(inFile);
      Pointer<PopplerDocument> doc(poppler_document_new_from_file(url.c_str(), nullptr, &error), g_object_unref);
      if(doc == nullptr)
      {
          std::cerr << "Failed to open PDF: " << error->message << " (" << inFile << ")" << std::endl;
          g_error_free(error);
          return 1;
      }
      nPages = poppler_document_get_n_pages(doc);
    }

    if(save)
    {
      if(!job.saveSettings())
      {
        std::cerr << "Could not save settings. Aborting." << std::endl;
        return 1;
      }
    }

    ProgressFun progressFun([](size_t page, size_t total) -> void
                {
                  DBG(<< page << "/" << total);
                });

    printer.printJobId(printJobId);
    error = printer.runJob(job, inFile, mimeType, nPages, progressFun);

    if(error)
    {
      std::cerr << "Print failed: " << error.value() << std::endl;
      return 1;
    }
  }
  return 0;
}
