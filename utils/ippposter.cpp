#include <bytestream.h>
#include <iostream>
#include "argget.h"
#include "curlrequester.h"
#include "ippmsg.h"
#include "ippprintjob.h"

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

int main(int argc, char** argv)
{
  bool help = false;
  bool verbose = false;
  std::string pages;
  int copies;
  std::string paperSize;
  uint32_t hwRes;
  uint32_t hwResX;
  uint32_t hwResY;
  bool duplex = false;
  bool tumble = false;
  std::string colorMode;
  int quality;

  std::string scaling;
  std::string format;
  std::string mimeType = "application/pdf";
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
  std::string infile;

  SwitchArg<bool> helpOpt(help, {"-h", "--help"}, "Print this help text");
  SwitchArg<bool> verboseOpt(verbose, {"-v", "--verbose"}, "Be verbose, print headers and progress");
  SwitchArg<std::string> pagesOpt(pages, {"-p", "--pages"}, "What pages to process, e.g.: 1,17-42");
  SwitchArg<int> copiesOpt(copies, {"--copies"}, "Number of copies to output");

  SwitchArg<std::string> paperSizeOpt(paperSize, {"--paper-size"}, "Paper size to output, e.g.: iso_a4_210x297mm");
  SwitchArg<uint32_t> resolutionOpt(hwRes, {"-r", "--resolution"}, "Resolution (in DPI) for rasterization");
  SwitchArg<uint32_t> resolutionXOpt(hwResX, {"-rx", "--resolution-x"}, "Resolution (in DPI) for rasterization, x-axis");
  SwitchArg<uint32_t> resolutionYOpt(hwResY, {"-ry", "--resolution-y"}, "Resolution (in DPI) for rasterization, y-axis");
  SwitchArg<bool> duplexOpt(duplex, {"-d", "--duplex"}, "Do duplex printing");
  SwitchArg<bool> tumbleOpt(tumble, {"-t", "--tumble"}, "Do tumbled duplex printing");

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
  PosArg pdfArg(infile, "input file");

  ArgGet args({&helpOpt, &verboseOpt, &pagesOpt,
               &copiesOpt, &paperSizeOpt, &resolutionOpt,
               &resolutionXOpt, &resolutionYOpt, &duplexOpt, &tumbleOpt,
               &colorModeOpt, &qualityOpt, &scalingOpt, &formatOpt, &mimeTypeOpt,
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

  if(verbose)
  {
    std::cerr << "Supported settings:" << std::endl << std::endl;

    std::cerr << "sides: " << ip.sides.getDefault() << std::endl;
    std::cerr << ip.sides.getSupported() << std::endl << std::endl;

    std::cerr << "media: " << ip.media.getDefault() << std::endl;
    std::cerr << ip.media.getSupported() << std::endl;
    std::cerr << ip.media.getPreferred() << std::endl << std::endl;

    std::cerr << "copies: " << ip.copies.getDefault() << std::endl;
    std::cerr << ip.copies.getSupportedMin() << " - "
              << ip.copies.getSupportedMax() << std::endl << std::endl;

    std::cerr << "collated copies: " << ip.multipleDocumentHandling.getDefault() << std::endl;
    std::cerr << ip.multipleDocumentHandling.getSupported() << std::endl << std::endl;

    std::cerr << "page ranges: " << ip.pageRanges.getSupported() << std::endl << std::endl;

    std::cerr << "number up: " << ip.numberUp.getDefault() << std::endl;
    std::cerr << ip.numberUp.getSupported() << std::endl << std::endl;

    std::cerr << "color: " << ip.colorMode.getDefault() << std::endl;
    std::cerr << ip.colorMode.getSupported() << std::endl << std::endl;

    std::cerr << "quality: " << ip.printQuality.getDefault() << std::endl;
    std::cerr << ip.printQuality.getSupported() << std::endl << std::endl;

    std::cerr << "resolution: " << ip.resolution.getDefault() << std::endl;
    std::cerr << ip.resolution.getSupported() << std::endl << std::endl;

    std::cerr << "scaling: " << ip.scaling.getDefault() << std::endl;
    std::cerr << ip.scaling.getSupported() << std::endl << std::endl;

    std::cerr << "format: " << ip.documentFormat.getDefault() << std::endl;
    std::cerr << ip.documentFormat.getSupported() << std::endl << std::endl;

    std::cerr << "media type: " << ip.mediaType.getDefault() << std::endl;
    std::cerr << ip.mediaType.getSupported() << std::endl << std::endl;

    std::cerr << "media source: " << ip.mediaSource.getDefault() << std::endl;
    std::cerr << ip.mediaSource.getSupported() << std::endl << std::endl;

    std::cerr << "output bin: " << ip.outputBin.getDefault() << std::endl;
    std::cerr << ip.outputBin.getSupported() << std::endl << std::endl;

    std::cerr << "media top margin: " << ip.topMargin.getDefault() << std::endl;
    std::cerr << ip.topMargin.getSupported() << std::endl << std::endl;

    std::cerr << "media bottom margin: " << ip.bottomMargin.getDefault() << std::endl;
    std::cerr << ip.bottomMargin.getSupported() << std::endl << std::endl;

    std::cerr << "media left margin: " << ip.leftMargin.getDefault() << std::endl;
    std::cerr << ip.leftMargin.getSupported() << std::endl << std::endl;

    std::cerr << "media right margin: " << ip.rightMargin.getDefault() << std::endl;
    std::cerr << ip.rightMargin.getSupported() << std::endl << std::endl;

  }

  // Error on unsupported - add force-option
  // check invalid/redundant combinations

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
  if(copiesOpt.isSet())
  {
    ip.copies.set(copies);
  }
  if(copiesOpt.isSet())
  {
    ip.copies.set(copies);
  }
  if(paperSizeOpt.isSet())
  {
    ip.media.set(paperSize);
  }
  if(resolutionOpt.isSet())
  {
    ip.resolution.set(IppResolution {hwRes, hwRes, IppResolution::DPI});
  }
  if(resolutionXOpt.isSet() && resolutionYOpt.isSet())
  {
    ip.resolution.set(IppResolution {hwResX, hwResY, IppResolution::DPI});
  }
  if(tumble)
  {
    ip.sides.set("two-sided-short-edge");
  }
  if(duplex)
  {
    ip.sides.set("two-sided-long-edge");
  }
  if(colorModeOpt.isSet())
  {
    ip.colorMode.set(colorMode);
  }
  if(qualityOpt.isSet())
  {
    ip.printQuality.set(quality);
  }
  if(scalingOpt.isSet())
  {
    ip.scaling.set(scaling);
  }
  if(formatOpt.isSet())
  {
    ip.documentFormat.set(format);
  }
  if(mediaTypeOpt.isSet())
  {
    ip.mediaType.set(mediaType);
  }
  if(mediaSourceOpt.isSet())
  {
    ip.mediaSource.set(mediaSource);
  }
  if(outputBinOpt.isSet())
  {
    ip.outputBin.set(outputBin);
  }
  if(marginOpt.isSet())
  {
    ip.topMargin.set(margin);
    ip.bottomMargin.set(margin);
    ip.leftMargin.set(margin);
    ip.rightMargin.set(margin);
  }
  if(topMarginOpt.isSet())
  {
    ip.topMargin.set(topMargin);
  }
  if(bottomMarginOpt.isSet())
  {
    ip.bottomMargin.set(bottomMargin);
  }
  if(leftMarginOpt.isSet())
  {
    ip.leftMargin.set(leftMargin);
  }
  if(rightMarginOpt.isSet())
  {
    ip.rightMargin.set(rightMargin);
  }

  Error error = ip.run(addr, infile, mimeType, verbose);
  if(error)
  {
    std::cerr << "Print failed: " << error.value() << std::endl;
    return 1;
  }
  return 0;
}
