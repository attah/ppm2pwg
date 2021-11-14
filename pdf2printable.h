#ifndef PDF2PRINTABLE_H
#define PDF2PRINTABLE_H

#include <functional>

typedef std::function<bool(unsigned char const*, unsigned int)> write_fun;
typedef std::function<void(size_t page, size_t total)> progress_fun;

enum Format
{
  PDF,
  Postscript,
  PWG,
  URF
};

int pdf_to_printable(std::string Infile, write_fun WriteFun, size_t Colors, size_t Quality,
                     std::string PaperSizeName, float PaperSizeX, float PaperSizeY,
                     size_t HwResX, size_t HwResY, Format TargetFormat,
                     bool Duplex, bool Tumble, bool BackHFlip, bool BackVFlip,
                     size_t FromPage, size_t ToPage, progress_fun ProgressFun = nullptr);

#endif //PDF2PRINTABLE_H
