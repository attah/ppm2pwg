#include "pdf2printable.h"

#include "array.h"
#include "bytestream.h"
#include "madness.h"
#include "ppm2pwg.h"
#include "uniquepointer.h"

#include <poppler.h>
#include <poppler-document.h>

#include <cairo.h>
#include <cairo-ps.h>
#include <cairo-pdf.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <unistd.h>

#define R_RELATIVE_LUMINOSITY 0.299
#define G_RELATIVE_LUMINOSITY 0.587
#define B_RELATIVE_LUMINOSITY 0.114
#define RGB32_R(RGB) ((RGB>>16)&0xff)
#define RGB32_G(RGB) ((RGB>>8)&0xff)
#define RGB32_B(RGB) (RGB&0xff)
#define RGB32_GRAY(RGB) (((RGB32_R(RGB)*R_RELATIVE_LUMINOSITY) \
                        + (RGB32_G(RGB)*G_RELATIVE_LUMINOSITY) \
                        + (RGB32_B(RGB)*B_RELATIVE_LUMINOSITY)))

#define SIXTEENTHS(parts, value) (parts*(value/16))

#ifndef PDF_CREATOR
#define PDF_CREATOR "pdf2printable"
#endif

#define CHECK(call) if(!(call)) {return Error("Write error");}

void copy_raster_buffer(Bytestream& bmpBts, const uint32_t* data, const PrintParameters& params);

void fixup_scale(double& xScale, double& yScale, double& xOffset, double& yOffset, bool& rotate,
                 double& wIn, double& hIn, const PrintParameters& params);

inline cairo_status_t bytestream_writer(void* bts, const unsigned char* data, unsigned int length)
{
  ((Bytestream*)bts)->putBytes(data, length);
  return CAIRO_STATUS_SUCCESS;
}

inline double round2(double d)
{
  return round(d*100)/100;
}

Error pdf_to_printable(const std::string& inFile, const PrintParameters& params,
                       const WriteFun& writeFun, const ProgressFun& progressFun)
{
  if(params.format == PrintParameters::URF && (params.hwResW != params.hwResH))
  {
    return Error("URF must have a symmetric resolution.");
  }

  #if MADNESS
  #include "libfuncs"
  #endif

  UniquePointer<cairo_surface_t> surface(nullptr, cairo_surface_destroy);
  UniquePointer<cairo_t> cairo(nullptr, cairo_destroy);
  cairo_status_t status;
  Bytestream bmpBts;
  Bytestream outBts;

  UniquePointer<PopplerDocument> doc(nullptr, g_object_unref);
  GError* error = nullptr;

  if(inFile == "-")
  {
    doc = poppler_document_new_from_fd(STDIN_FILENO, nullptr, &error);
  }
  else
  {
    std::string url("file://");
    url += std::filesystem::absolute(inFile);
    doc = poppler_document_new_from_file(url.c_str(), nullptr, &error);
  }

  UniquePointer<GError> error_p(error, g_error_free);

  if(doc == nullptr)
  {
    std::string errStr(error->message);
    return Error("Failed to open PDF: " + errStr + " (" + inFile + ")");
  }

  size_t pages = poppler_document_get_n_pages(doc);
  PageSequence pageSequence = params.getPageSequence(pages);

  size_t outPageNo = 0;

  if(params.isRasterFormat())
  {
    surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
                                         params.getPaperSizeWInPixels(),
                                         params.getPaperSizeHInPixels());
    if(params.format == PrintParameters::URF)
    {
      outBts = make_urf_file_hdr(pageSequence.size());
    }
    else
    {
      outBts = make_pwg_file_hdr();
    }
  }
  else if(params.format == PrintParameters::PDF)
  {
    surface = cairo_pdf_surface_create_for_stream(bytestream_writer, &outBts,
                                                  params.getPaperSizeWInPoints(),
                                                  params.getPaperSizeHInPoints());
    #if CAIRO_VERSION < CAIRO_VERSION_ENCODE(1, 17, 6)
    // Cairo is robust against setting "too new" versions
    // - pretend we know about 1.7 even if we don't
    #define CAIRO_PDF_VERSION_1_7 (cairo_pdf_version_t)(CAIRO_PDF_VERSION_1_5 + 2)
    #endif
    // 1.7 aka ISO32000 is the recommended version according to PWG5100.14
    cairo_pdf_surface_restrict_to_version(surface, CAIRO_PDF_VERSION_1_7);
    cairo_pdf_surface_set_metadata(surface, CAIRO_PDF_METADATA_CREATOR, PDF_CREATOR);
  }
  else if(params.format == PrintParameters::Postscript)
  {
    surface = cairo_ps_surface_create_for_stream(bytestream_writer, &outBts,
                                                 params.getPaperSizeWInPoints(),
                                                 params.getPaperSizeHInPoints());
    cairo_ps_surface_restrict_to_level(surface, CAIRO_PS_LEVEL_2);
  }
  else
  {
    return Error("Unknown format");
  }

  cairo_surface_set_fallback_resolution(surface, params.hwResW, params.hwResH);

  for(size_t pageNo : pageSequence)
  {
    outPageNo++;

    cairo = cairo_create(surface);

    if(params.isRasterFormat())
    {
      if(!params.antiAlias)
      {
        cairo_set_antialias(cairo, CAIRO_ANTIALIAS_NONE);
        UniquePointer<cairo_font_options_t> fontOptions(cairo_font_options_create(),
                                                        cairo_font_options_destroy);
        cairo_get_font_options(cairo, fontOptions);
        cairo_font_options_set_antialias(fontOptions, CAIRO_ANTIALIAS_NONE);
        cairo_set_font_options(cairo, fontOptions);
      }
      cairo_save(cairo);
      cairo_set_source_rgb(cairo, 1, 1, 1);
      cairo_paint(cairo);
      cairo_restore(cairo);
    }

    if(pageNo != INVALID_PAGE)
    { // We are actually rendering a page and not just a blank...
      UniquePointer<PopplerPage> page(poppler_document_get_page(doc, pageNo-1), g_object_unref);
      double pageWidth;
      double pageHeight;
      double xScale;
      double yScale;
      double xOffset;
      double yOffset;
      bool rotate = false;

      poppler_page_get_size(page, &pageWidth, &pageHeight);
      fixup_scale(xScale, yScale, xOffset, yOffset, rotate, pageWidth, pageHeight, params);

      cairo_translate(cairo, xOffset, yOffset);
      cairo_scale(cairo, xScale, yScale);

      if(rotate)
      { // Rotate to portrait
        cairo_matrix_t matrix;
        cairo_matrix_init(&matrix, 0, -1, 1, 0, 0, pageHeight);
        cairo_transform(cairo, &matrix);
      }

      poppler_page_render_for_printing(page, cairo);
    }

    status = cairo_status(cairo);
    if(status)
    {
      return Error(std::string("Cairo error: ") + cairo_status_to_string(status));
    }
    cairo_surface_show_page(surface);

    if(params.isRasterFormat())
    {
      cairo_surface_flush(surface);
      uint32_t* data = (uint32_t*)cairo_image_surface_get_data(surface);
      copy_raster_buffer(bmpBts, data, params);
      bmp_to_pwg(bmpBts, outBts, outPageNo, params);
    }

    CHECK(writeFun(std::move(outBts)));
    outBts = Bytestream();

    progressFun(outPageNo, pageSequence.size());
  }

  cairo_surface_finish(surface);
  // PDF and PS will have written something now, write it out
  if(outBts.size() != 0)
  {
    CHECK(writeFun(std::move(outBts)));
  }

  return Error();
}

void copy_raster_buffer(Bytestream& bmpBts, const uint32_t* data, const PrintParameters& params)
{
  size_t size = params.getPaperSizeInPixels();
  bool black = params.isBlack();

  if(bmpBts.size() != params.getPaperSizeInBytes())
  {
    bmpBts = Bytestream(params.getPaperSizeInBytes());
  }

  uint8_t* tmp = bmpBts.raw();

  switch(params.colorMode)
  {
    case PrintParameters::Gray1:
    case PrintParameters::Black1:
    {
      size_t paperSizeWInPixels = params.getPaperSizeWInPixels();
      size_t paperSizeWInBytes = params.getPaperSizeWInBytes();
      size_t paperSizeHInPixels = params.getPaperSizeHInPixels();
      Array<int> debtArray(paperSizeWInPixels+2);
      memset(debtArray, 0, (paperSizeWInPixels+2)*sizeof(int));
      memset(tmp, black ? 0 : 0xff, bmpBts.size());
      for(size_t line=0; line < paperSizeHInPixels; line++)
      {
        size_t lineStartInBytes = line * paperSizeWInBytes;
        size_t lineStartInPixels = line * paperSizeWInPixels;
        int nextDebt = 0; // Don't carry over forward debt from previous line
        for(size_t col=0; col < paperSizeWInPixels; col++)
        { // Do Floyd-Steinberg dithering to keep grayscales readable in 1-bit
          int pixel = RGB32_GRAY(data[lineStartInPixels+col]) + nextDebt;
          int newpixel = pixel < 128 ? 0 : 255;
          int debt = pixel - newpixel;
          nextDebt = debtArray[col+2] + SIXTEENTHS(7, debt);
          debtArray[col] += SIXTEENTHS(3, debt);
          debtArray[col+1] += SIXTEENTHS(5, debt);
          debtArray[col+2] = SIXTEENTHS(1, debt);
          if(newpixel == 0)
          {
            if(black)
            {
              tmp[lineStartInBytes+(col/8)] |= (0x80 >> (col % 8));
            }
            else
            {
              tmp[lineStartInBytes+(col/8)] &= ~(0x80 >> (col % 8));
            }
          }
        }
      }
      break;
    }
    case PrintParameters::Gray8:
    case PrintParameters::Black8:
    {
      for(size_t i=0; i < size; i++)
      {
        tmp[i] = black ? (255 - RGB32_GRAY(data[i])) : RGB32_GRAY(data[i]);
      }
      break;
    }
    case PrintParameters::sRGB24:
    {
      for(size_t i=0, j=0; i < size; i++, j+=3)
      {
        tmp[j] = RGB32_R(data[i]);
        tmp[j+1] = RGB32_G(data[i]);
        tmp[j+2] = RGB32_B(data[i]);
      }
      break;
    }
    case PrintParameters::CMYK32:
    {
      for(size_t i=0, j=0; i < size; i++, j+=4)
      {
        uint32_t blackDiff = std::max({RGB32_R(data[i]), RGB32_G(data[i]), RGB32_B(data[i])});
        tmp[j] = (blackDiff - RGB32_R(data[i]));
        tmp[j+1] = (blackDiff - RGB32_G(data[i]));
        tmp[j+2] = (blackDiff - RGB32_B(data[i]));
        tmp[j+3] = 255 - blackDiff;
      }
      break;
    }
    default:
    {
      throw std::logic_error("Unhandled color mode");
    }
  }
}


void fixup_scale(double& xScale, double& yScale, double& xOffset, double& yOffset, bool& rotate,
                 double& wIn, double& hIn, const PrintParameters& params)
{
  // If the page is landscape, contunue as if it is not, but remember to rotate
  if(wIn > hIn)
  {
    std::swap(wIn, hIn);
    rotate = true;
  }

  // Scale to fit as if we had a symmetric resolution
  // ...this makes determining fitment easier
  PrintParameters tmp = params;
  tmp.hwResW = std::min(params.hwResW, params.hwResH);
  tmp.hwResH = std::min(params.hwResW, params.hwResH);

  size_t hOut = params.isRasterFormat() ? tmp.getPaperSizeHInPixels()
                                        : tmp.getPaperSizeHInPoints();
  size_t wOut = params.isRasterFormat() ? tmp.getPaperSizeWInPixels()
                                        : tmp.getPaperSizeWInPoints();
  double scale = round2(std::min(wOut/wIn, hOut/hIn));
  xOffset = roundf((wOut-(wIn*scale))/2);
  yOffset = roundf((hOut-(hIn*scale))/2);

  xScale = scale;
  yScale = scale;

  // Finally, if we have an asymmetric resolution
  // and are not working in absolute dimensions (points), compensate for it.
  // NB: This expects an integer scale.
  if(params.isRasterFormat())
  { // URF will/should not end up here, but still...
    if(params.hwResW > params.hwResH)
    {
      size_t scale = params.hwResW / params.hwResH;
      xScale *= scale;
      xOffset *= scale;
    }
    else if(params.hwResH > params.hwResW)
    {
      size_t scale = params.hwResH / params.hwResW;
      yScale *= scale;
      yOffset *= scale;
    }
  }
}
