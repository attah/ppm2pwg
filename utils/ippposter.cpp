#include <bytestream.h>
#include <iostream>
#include <filesystem>

#include <poppler.h>
#include <poppler-document.h>

#include "argget.h"
#include "curlrequester.h"
#include "ippmsg.h"
#include "ippprintjob.h"
#include "minimime.h"
#include "pointer.h"

#define HELPTEXT ""

inline void print_error(std::string hint, std::string argHelp)
{
  std::cerr << hint << std::endl << std::endl << argHelp << std::endl << HELPTEXT << std::endl;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, List<T> bl)
{
  if(bl.size() > 0)
  {
    os << "[" << bl.takeFront();
    for(T b : bl)
    {
      os << ", " << b;
    }
    os << "]";
  }
  else
  {
    os << "[]";
  }
  return os;
}

std::string resolution_list(List<IppResolution> l)
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

  if(ss.str().length() == 0)
  {
    ss << "empty";
  }
  return ss.str();
}

template <typename O, typename S, typename V>
void set_or_fail(const O& opt, S& setting, V value, bool force)
{
  std::string docName = opt.docName() + " (ipp: " + setting.name() + ")";
  set_or_fail(opt.isSet(), docName, "Valid values are: " + setting.supportedStr(), setting, value, force);
}

template <typename S, typename V>
void set_or_fail(bool isSet, std::string docName, std::string inputHint, S& setting, V value, bool force)
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
  std::string pages;
  int copies;
  std::string collatedCopies;
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

  int margin;
  int topMargin;
  int bottomMargin;
  int leftMargin;
  int rightMargin;

  bool antiAlias;

  std::string addr;
  std::string inFile;

  SwitchArg<bool> helpOpt(help, {"-h", "--help"}, "Print this help text");
  SwitchArg<bool> verboseOpt(verbose, {"-v", "--verbose"}, "Be verbose, print headers and progress");
  SwitchArg<bool> forceOpt(verbose, {"-f", "--force"}, "Force use of unsupported options");

  SwitchArg<std::string> pagesOpt(pages, {"-p", "--pages"}, "What pages to process, e.g.: 1,17-42");
  SwitchArg<int> copiesOpt(copies, {"--copies"}, "Number of copies to output");
  EnumSwitchArg<std::string> collatedCopiesOpt(collatedCopies, {{"yes", "separate-documents-collated-copies"},
                                                                {"no", "separate-documents-uncollated-copies"}},
                                               {"--collated-copies"}, "Request collated copies");

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

  SwitchArg<int> marginOpt(margin, {"-m", "--margin"}, "Margin (as per IPP)");
  SwitchArg<int> topMarginOpt(topMargin, {"-tm", "--top-margin"}, "Top margin (as per IPP)");
  SwitchArg<int> bottomMarginOpt(bottomMargin, {"-bm", "--bottom-margin"}, "Bottom margin (as per IPP)");
  SwitchArg<int> leftMarginOpt(leftMargin, {"-lm", "--left-margin"}, "Left margin (as per IPP)");
  SwitchArg<int> rightMarginOpt(rightMargin, {"-rm", "--right-margin"}, "Right margin (as per IPP)");

  SwitchArg<bool> antiAliasOpt(antiAlias, {"-aa", "--antaialias"}, "Enable antialiasing in rasterization");

  PosArg addrArg(addr, "printer address");
  PosArg pdfArg(inFile, "input file");

  ArgGet args({&helpOpt, &verboseOpt, &forceOpt,
               &pagesOpt, &copiesOpt, &collatedCopiesOpt, &paperSizeOpt,
               &resolutionOpt, &resolutionXOpt, &resolutionYOpt,
               &sidesOpt, &colorModeOpt, &qualityOpt, &scalingOpt,
               &formatOpt, &mimeTypeOpt,
               &mediaTypeOpt, &mediaSourceOpt, &outputBinOpt,
               &marginOpt, &topMarginOpt, &bottomMarginOpt, &leftMarginOpt, &rightMarginOpt,
               &antiAliasOpt},
              {&addrArg, &pdfArg});

  bool correctArgs = args.get_args(argc, argv);
  if(help)
  {
    std::cout << args.argHelp() << std::endl << HELPTEXT << std::endl;
    return 0;
  }
  else if(!correctArgs)
  {
    print_error(args.errmsg(), args.argHelp());
    return 1;
  }

  IppAttrs getPrinterAttrsJobAttrs = IppMsg::baseOpAttrs(addr);
  IppMsg getPrinterAttrsMsg(IppMsg::GetPrinterAttrs, getPrinterAttrsJobAttrs);
  Bytestream getPrinterAttrsEnc = getPrinterAttrsMsg.encode();

  CurlIppPoster getPrinterAttrsReq(addr, getPrinterAttrsEnc, true, verbose);
  getPrinterAttrsReq.write((char*)(getPrinterAttrsEnc.raw()), getPrinterAttrsEnc.size());
  Bytestream getPrinterAttrsResult;
  CURLcode res0 = getPrinterAttrsReq.await(&getPrinterAttrsResult);
  IppAttrs printerAttrs;
  if(res0 == CURLE_OK)
  {
    IppMsg getPrinterAttrsResp(getPrinterAttrsResult);
    printerAttrs = getPrinterAttrsResp.getPrinterAttrs();
  }
  else
  {
    std::cerr << "Could not get printer attributes: " << curl_easy_strerror(res0) << std::endl;
    return 1;
  }

  IppPrintJob ip(printerAttrs);

  if(antiAliasOpt.isSet())
  {
    ip.printParams.antiAlias = antiAlias;
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
    PageRangeList pageRanges = PrintParameters::parsePageRange(pages);
    if(pageRanges.empty())
    {
      print_error("Invalid page range.", args.argHelp());
      return 1;
    }
    IppOneSetOf ippPageRanges;
    for(std::pair<int32_t, int32_t> pageRange : pageRanges)
    {
      ippPageRanges.push_back(IppIntRange {pageRange.first, pageRange.second});
    }
    ip.pageRanges.set(ippPageRanges);
  }

  set_or_fail(copiesOpt, ip.copies, copies, force);
  set_or_fail(collatedCopiesOpt, ip.multipleDocumentHandling, collatedCopies, force);
  set_or_fail(paperSizeOpt, ip.media, paperSize, force);

  std::string resolutionDoc = resolutionOpt.docName() + " (ipp: " + ip.resolution.name() + ")";
  std::string resolutionSupported = "Valid values are: " + resolution_list(ip.resolution.getSupported());
  set_or_fail(resolutionOpt.isSet(), resolutionDoc, resolutionSupported,
              ip.resolution, IppResolution {hwRes, hwRes, IppResolution::DPI}, force);

  std::string resolutionDoc2 = resolutionXOpt.docName() + " and " + resolutionYOpt.docName()
                             + " (ipp: " + ip.resolution.name() + ")";
  set_or_fail(resolutionXOpt.isSet() && resolutionYOpt.isSet(), resolutionDoc2, resolutionSupported,
              ip.resolution, IppResolution {hwResX, hwResY, IppResolution::DPI}, force);

  set_or_fail(sidesOpt, ip.sides, sides, force);
  set_or_fail(colorModeOpt, ip.colorMode, colorMode, force);
  set_or_fail(qualityOpt, ip.printQuality, quality, force);
  set_or_fail(scalingOpt, ip.scaling, scaling, force);
  set_or_fail(formatOpt, ip.documentFormat, format, force);
  set_or_fail(mediaTypeOpt, ip.mediaType, mediaType, force);
  set_or_fail(mediaSourceOpt, ip.mediaSource, mediaSource, force);
  set_or_fail(outputBinOpt, ip.outputBin, outputBin, force);

  if(marginOpt.isSet())
  {
    if(!force && (!ip.topMargin.isSupported() || !ip.bottomMargin.isSupported() ||
                  !ip.leftMargin.isSupported() || !ip.rightMargin.isSupported()))
    {
      std::cerr << "Argument " << marginOpt.docName() << " (ipp: margin-*) "
                << "is not supported by this printer." << std::endl;
      exit(1);
    }
    set_or_fail(true, marginOpt.docName(),
                "Valid values for top margin are: " +  ip.topMargin.supportedStr(),
                ip.topMargin, margin, force);
    set_or_fail(true, marginOpt.docName(),
                "Valid values for bottom margin are: " +  ip.bottomMargin.supportedStr(),
                ip.bottomMargin, margin, force);
    set_or_fail(true, marginOpt.docName(),
                "Valid values for left margin are: " +  ip.leftMargin.supportedStr(),
                ip.leftMargin, margin, force);
    set_or_fail(true, marginOpt.docName(),
                "Valid values for right margin are: " +  ip.rightMargin.supportedStr(),
                ip.rightMargin, margin, force);
  }

  set_or_fail(topMarginOpt, ip.topMargin, topMargin, force);
  set_or_fail(bottomMarginOpt, ip.bottomMargin, bottomMargin, force);
  set_or_fail(leftMarginOpt, ip.leftMargin, leftMargin, force);
  set_or_fail(rightMarginOpt, ip.rightMargin, rightMargin, force);

  int nPages = 0; // Unknown - assume multiple is the format allows

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

  Error error = ip.run(addr, inFile, mimeType, nPages, verbose);
  if(error)
  {
    std::cerr << "Print failed: " << error.value() << std::endl;
    return 1;
  }
  return 0;
}
