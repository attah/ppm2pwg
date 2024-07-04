#include <poppler.h>
#include <poppler-document.h>

#include <cairo.h>
#include <cairo-ps.h>
#include <cairo-pdf.h>

#include <filesystem>
#include <math.h>
#include <unistd.h>
#include "madness.h"

#include <bytestream.h>
#include <array.h>
#include "pointer.h"
#include "ppm2pwg.h"
#include "pdf2printable.h"

#define R_RELATIVE_LUMINOSITY 0.299
#define G_RELATIVE_LUMINOSITY 0.587
#define B_RELATIVE_LUMINOSITY 0.114
#define RGB32_R(RGB) ((RGB>>16)&0xff)
#define RGB32_G(RGB) ((RGB>>8)&0xff)
#define RGB32_B(RGB) (RGB&0xff)
#define RGB32_GRAY(RGB) (((RGB32_R(RGB)*R_RELATIVE_LUMINOSITY) \
                        + (RGB32_G(RGB)*G_RELATIVE_LUMINOSITY) \
                        + (RGB32_B(RGB)*B_RELATIVE_LUMINOSITY)))

#define MAX3(A,B,C) std::max(std::max(A, B), C)
#define SIXTEENTHS(parts, value) (parts*(value/16))

#ifndef PDF_CREATOR
#define PDF_CREATOR "pdf2printable"
#endif

#define CHECK(call) if(!(call)) {return Error("Write error");}

void copy_raster_buffer(Bytestream& bmpBts, uint32_t* data, const PrintParameters& params);

void fixup_scale(double& xScale, double& yScale, double& xOffset, double& yOffset, bool& rotate,
                 double wIn, double hIn, const PrintParameters& params);

inline cairo_status_t bytestream_writer(void* bts, const unsigned char* data, unsigned int length)
{
  ((Bytestream*)bts)->putBytes(data, length);
  return CAIRO_STATUS_SUCCESS;
}

inline double round2(double d)
{
  return round(d*100)/100;
}

Error pdf_to_printable(std::string inFile, WriteFun writeFun, const PrintParameters& params,
                       ProgressFun progressFun)
{
  if(params.format == PrintParameters::URF && (params.hwResW != params.hwResH))
  {
    return Error("URF must have a symmetric resolution.");
  }

  #if MADNESS
  #include "libfuncs"
  #endif

  bool raster = params.format == PrintParameters::PWG ||
                params.format == PrintParameters::URF;

  Pointer<cairo_surface_t> surface(nullptr, cairo_surface_destroy);
  Pointer<cairo_t> cairo(nullptr, cairo_destroy);
  cairo_status_t status;
  Bytestream bmpBts;
  Bytestream outBts;

  Pointer<PopplerDocument> doc(nullptr, g_object_unref);
  GError* error = nullptr;

  if(inFile == "-")
  {
    doc = poppler_document_new_from_fd(STDIN_FILENO, nullptr, &error);
  }
  else
  {
    inFile = std::filesystem::absolute(inFile);
    std::string url("file://");
    url.append(inFile);
    doc = poppler_document_new_from_file(url.c_str(), nullptr, &error);
  }

  Pointer<GError> error_p(error, g_error_free);

  if(doc == nullptr)
  {
    std::string errStr(error->message);
    return Error("Failed to open PDF: " + errStr + " (" + inFile + ")");
  }

  size_t pages = poppler_document_get_n_pages(doc);
  PageSequence seq = params.getPageSequence(pages);

  size_t outPageNo = 0;

  if(raster)
  {
    surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
                                         params.getPaperSizeWInPixels(),
                                         params.getPaperSizeHInPixels());
    if(params.format == PrintParameters::URF)
    {
      outBts = make_urf_file_hdr(seq.size());
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

  for(size_t pageNo : seq)
  {
    outPageNo++;

    cairo = cairo_create(surface);

    if(raster)
    {
      if(!params.antiAlias)
      {
        cairo_set_antialias(cairo, CAIRO_ANTIALIAS_NONE);
        Pointer<cairo_font_options_t> fontOptions(cairo_font_options_create(),
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
      Pointer<PopplerPage> page(poppler_document_get_page(doc, pageNo-1), g_object_unref);
      double pageWidth, pageHeight;
      double xScale, yScale, xOffset, yOffset;
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

    if(raster)
    {
      cairo_surface_flush(surface);
      uint32_t* data = (uint32_t*)cairo_image_surface_get_data(surface);
      copy_raster_buffer(bmpBts, data, params);
      bmp_to_pwg(bmpBts, outBts, outPageNo, params);
    }

    CHECK(writeFun(std::move(outBts)));
    outBts.reset();

    if(progressFun != nullptr)
    {
      progressFun(outPageNo, seq.size());
    }

  }

  cairo_surface_finish(surface);
  // PDF and PS will have written something now, write it out
  if(outBts.size() != 0)
  {
    CHECK(writeFun(std::move(outBts)));
  }

  return Error();
}

void copy_raster_buffer(Bytestream& bmpBts, uint32_t* data, const PrintParameters& params)
{
  size_t size = params.getPaperSizeInPixels();
  bool black = params.isBlack();

  if(bmpBts.size() != params.getPaperSizeInBytes())
  {
    bmpBts = Bytestream(params.getPaperSizeInBytes());
  }

  uint8_t* tmp = bmpBts.raw();

  if(params.colorMode == PrintParameters::Gray1 || params.colorMode == PrintParameters::Black1)
  {
    size_t paperSizeWInPixels = params.getPaperSizeWInPixels();
    size_t paperSizeWInBytes = params.getPaperSizeWInBytes();
    size_t paperSizeHInPixels = params.getPaperSizeHInPixels();
    int nextDebt, pixel, newpixel, debt;
    Array<int> debtArray(paperSizeWInPixels+2);
    memset(debtArray, 0, (paperSizeWInPixels+2)*sizeof(int));
    memset(tmp, black ? 0 : 0xff, bmpBts.size());
    for(size_t line=0; line < paperSizeHInPixels; line++)
    {
      nextDebt = 0; // Don't carry over forward debt from previous line
      for(size_t col=0; col < paperSizeWInPixels; col++)
      { // Do Floyd-Steinberg dithering to keep grayscales readable in 1-bit
        pixel = RGB32_GRAY(data[line*paperSizeWInPixels+col]) + nextDebt;
        newpixel = pixel < 128 ? 0 : 255;
        debt = pixel - newpixel;
        nextDebt = debtArray[col+2] + SIXTEENTHS(7, debt);
        debtArray[col] += SIXTEENTHS(3, debt);
        debtArray[col+1] += SIXTEENTHS(5, debt);
        debtArray[col+2] = SIXTEENTHS(1, debt);
        if(newpixel == 0)
        {
          if(black)
          {
            tmp[line*paperSizeWInBytes+col/8] |= (0x80 >> (col % 8));
          }
          else
          {
            tmp[line*paperSizeWInBytes+col/8] &= ~(0x80 >> (col % 8));
          }
        }
      }
    }
  }
  else if(params.colorMode == PrintParameters::Gray8 || params.colorMode == PrintParameters::Black8)
  {
    for(size_t i=0; i < size; i++)
    {
      tmp[i] = black ? (255 - RGB32_GRAY(data[i])) : RGB32_GRAY(data[i]);
    }
  }
  else if(params.colorMode == PrintParameters::sRGB24)
  {
    for(size_t i=0; i < size; i++)
    {
      tmp[i*3] = RGB32_R(data[i]);
      tmp[i*3+1] = RGB32_G(data[i]);
      tmp[i*3+2] = RGB32_B(data[i]);
    }
  }
  else if(params.colorMode == PrintParameters::CMYK32)
  {
    for(size_t i=0; i < size; i++)
    {
      uint32_t white = MAX3(RGB32_R(data[i]), RGB32_G(data[i]), RGB32_B(data[i]));
      tmp[i*4] = (white - RGB32_R(data[i]));
      tmp[i*4+1] = (white - RGB32_G(data[i]));
      tmp[i*4+2] = (white - RGB32_B(data[i]));
      tmp[i*4+3] = 255-white;
    }
  }
}


void fixup_scale(double& xScale, double& yScale, double& xOffset, double& yOffset, bool& rotate,
                 double wIn, double hIn, const PrintParameters& params)
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

  bool raster = params.format == PrintParameters::PWG ||
                params.format == PrintParameters::URF;

  size_t hOut = raster ? tmp.getPaperSizeHInPixels() : tmp.getPaperSizeHInPoints();
  size_t wOut = raster ? tmp.getPaperSizeWInPixels() : tmp.getPaperSizeWInPoints();
  double scale = round2(std::min(wOut/wIn, hOut/hIn));
  xOffset = roundf((wOut-(wIn*scale))/2);
  yOffset = roundf((hOut-(hIn*scale))/2);

  xScale = scale;
  yScale = scale;

  // Finally, if we have an asymmetric resolution
  // and are not working in absolute dimensions (points), compensate for it.
  if(raster)
  { // URF will/should not end up here, but still...
    if(params.hwResW > params.hwResH)
    {
      xScale *= (params.hwResW/params.hwResH);
      xOffset *= (params.hwResW/params.hwResH);
    }
    else if(params.hwResH > params.hwResW)
    {
      yScale *= (params.hwResH/params.hwResW);
      yOffset *= (params.hwResH/params.hwResW);
    }
  }
}
