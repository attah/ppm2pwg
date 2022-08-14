#include <poppler/glib/poppler.h>
#include <poppler/glib/poppler-document.h>

#include <cairo.h>
#include <cairo-ps.h>
#include <cairo-pdf.h>

#include <iostream>
#include <fstream>
#include <math.h>
#include "madness.h"

#include <bytestream.h>
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

#define CHECK(call) if(!(call)) {res = 1; goto error;}

void copy_raster_buffer(Bytestream& bmp_bts, uint32_t* dat, const PrintParameters& Params);

void fixup_scale(double& x_scale, double& y_scale, double& x_offset, double& y_offset,
                 bool& rotate, double& w_in, double& h_in, const PrintParameters& Params);

std::string free_cstr(char* CStr)
{
  std::string tmp(CStr);
  free(CStr);
  return tmp;
}

cairo_status_t bytestream_writer(void* bts, const unsigned char* data, unsigned int length)
{
  ((Bytestream*)bts)->putBytes(data, length);
  return CAIRO_STATUS_SUCCESS;
}

double round2(double d)
{
  return round(d*100)/100;
}

int pdf_to_printable(std::string Infile, write_fun WriteFun, const PrintParameters& Params,
                     progress_fun ProgressFun, bool Verbose)
{
  int res = 0;
  bool raster = Params.format == PrintParameters::PWG ||
                Params.format == PrintParameters::URF;

  cairo_surface_t* surface;
  cairo_t* cr;
  cairo_status_t status;
  Bytestream bmp_bts;
  Bytestream OutBts;

  if(Params.format == PrintParameters::URF && (Params.hwResW != Params.hwResH))
  { // URF must have a symmetric resolution
    return 1;
  }

  #if MADNESS
  #include "libfuncs"
  #endif

  if (!g_path_is_absolute(Infile.c_str()))
  {
      std::string dir = free_cstr(g_get_current_dir());
      Infile = free_cstr(g_build_filename(dir.c_str(), Infile.c_str(), NULL));
  }

  std::string url("file://");
  url.append(Infile);
  GError* error = nullptr;
  PopplerDocument* doc = poppler_document_new_from_file(url.c_str(), nullptr, &error);

  if(doc == NULL)
  {
    std::cerr << "Failed to open PDF: " << error->message
              << " (" << url << ")" << std::endl;
    return 1;
  }

  size_t pages = poppler_document_get_n_pages(doc);
  PageSequence seq = Params.getPageSequence(pages);

  size_t out_page_no = 0;

  if(raster)
  {
    surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
                                         Params.getPaperSizeWInPixels(),
                                         Params.getPaperSizeHInPixels());
    Bytestream FileHdr;
    if(Params.format == PrintParameters::URF)
    {
      FileHdr = make_urf_file_hdr(seq.size());
    }
    else
    {
      FileHdr = make_pwg_file_hdr();
    }
    CHECK(WriteFun(FileHdr.raw(), FileHdr.size()));
  }
  else if(Params.format == PrintParameters::PDF)
  {
    surface = cairo_pdf_surface_create_for_stream(bytestream_writer, &OutBts,
                                                  Params.getPaperSizeWInPoints(),
                                                  Params.getPaperSizeHInPoints());
    cairo_pdf_surface_set_metadata(surface, CAIRO_PDF_METADATA_CREATOR, PDF_CREATOR);
  }
  else if(Params.format == PrintParameters::Postscript)
  {
    surface = cairo_ps_surface_create_for_stream(bytestream_writer, &OutBts,
                                                 Params.getPaperSizeWInPoints(),
                                                 Params.getPaperSizeHInPoints());
    cairo_ps_surface_restrict_to_level(surface, CAIRO_PS_LEVEL_2);
  }
  else
  {
    g_object_unref(doc);
    return 1;
  }

  for(size_t page_no : seq)
  {
    out_page_no++;
    cr = cairo_create(surface);

    if(raster)
    {
      if(!Params.antiAlias)
      {
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
        cairo_font_options_t *font_options = cairo_font_options_create();
        cairo_get_font_options(cr, font_options);
        cairo_font_options_set_antialias(font_options, CAIRO_ANTIALIAS_NONE);
        cairo_set_font_options(cr, font_options);
        cairo_font_options_destroy(font_options);
      }
      cairo_save(cr);
      cairo_set_source_rgb(cr, 1, 1, 1);
      cairo_paint(cr);
      cairo_restore(cr);
    }

    if(page_no != INVALID_PAGE)
    { // We are actually rendering a page and not just a blank...
      PopplerPage* page = poppler_document_get_page(doc, page_no-1);
      double page_width, page_height;
      double x_scale, y_scale, x_offset, y_offset;
      bool rotate = false;

      poppler_page_get_size(page, &page_width, &page_height);
      fixup_scale(x_scale, y_scale, x_offset, y_offset, rotate,
                  page_width, page_height, Params);

      cairo_translate(cr, x_offset, y_offset);
      cairo_scale(cr, x_scale, y_scale);

      if (rotate)
      { // Rotate to portrait
        cairo_matrix_t m;
        cairo_matrix_init(&m, 0, -1, 1, 0, 0, page_height);
        cairo_transform(cr, &m);
      }

      poppler_page_render_for_printing(page, cr);
      g_object_unref(page);
    }

    status = cairo_status(cr);
    if (status)
    {
      std::cerr << "cairo error: " << cairo_status_to_string(status) << std::endl;
    }
    cairo_destroy(cr);
    cairo_surface_show_page(surface);

    if(raster)
    {
      cairo_surface_flush(surface);
      uint32_t* dat = (uint32_t*)cairo_image_surface_get_data(surface);
      copy_raster_buffer(bmp_bts, dat, Params);
      bmp_to_pwg(bmp_bts, OutBts, out_page_no, Params, Verbose);
    }

    CHECK(WriteFun(OutBts.raw(), OutBts.size()));
    OutBts.reset();

    if(ProgressFun != nullptr)
    {
      ProgressFun(out_page_no, seq.size());
    }

  }

  cairo_surface_finish(surface);
  // PDF and PS will have written something now, write it out
  if(OutBts.size() != 0)
  {
    CHECK(WriteFun(OutBts.raw(), OutBts.size()));
  }

error:
  cairo_surface_destroy(surface);
  g_object_unref(doc);
  return res;
}

void copy_raster_buffer(Bytestream& bmp_bts, uint32_t* dat, const PrintParameters& Params)
{
  size_t size = Params.getPaperSizeInPixels();

  if(bmp_bts.size() != Params.getPaperSizeInBytes())
  {
    bmp_bts = Bytestream(Params.getPaperSizeInBytes());
  }

  uint8_t* tmp = bmp_bts.raw();

  if(Params.colors == 1 && Params.bitsPerColor == 1)
  {
    size_t paperSizeWInPixels = Params.getPaperSizeWInPixels();
    size_t paperSizeWInBytes = Params.getPaperSizeWInBytes();
    size_t paperSizeHInPixels = Params.getPaperSizeHInPixels();
    int next_debt, pixel, newpixel, debt;
    int* debt_array = new int[paperSizeWInPixels+2];
    memset(debt_array, 0, (paperSizeWInPixels+2)*sizeof(int));
    memset(tmp, Params.black ? 0 : 0xff, bmp_bts.size());
    for(size_t line=0; line < paperSizeHInPixels; line++)
    {
      next_debt = 0; // Don't carry over forward debt from previous line
      for(size_t col=0; col < paperSizeWInPixels; col++)
      { // Do Floyd-Steinberg dithering to keep grayscales readable in 1-bit
        pixel = RGB32_GRAY(dat[line*paperSizeWInPixels+col]) + next_debt;
        newpixel = pixel < 127 ? 0 : 255;
        debt = pixel - newpixel;
        next_debt = debt_array[col+2] + SIXTEENTHS(7, debt);
        debt_array[col] += SIXTEENTHS(3, debt);
        debt_array[col+1] += SIXTEENTHS(5, debt);
        debt_array[col+2] = SIXTEENTHS(1, debt);
        if(newpixel == 0 && Params.black)
        {
          tmp[line*paperSizeWInBytes+col/8] |= (0x80 >> (col % 8));
        }
        else if(newpixel == 0)
        {
          tmp[line*paperSizeWInBytes+col/8] &= ~(0x80 >> (col % 8));
        }
      }
    }
    delete[] debt_array;
  }
  else if(Params.colors == 1 && Params.bitsPerColor == 8)
  {
    for(size_t i=0; i < size; i++)
    {
      tmp[i] = Params.black ? (255 - RGB32_GRAY(dat[i])) : RGB32_GRAY(dat[i]);
    }
  }
  else if(Params.colors == 3)
  {
    for(size_t i=0; i < size; i++)
    {
      tmp[i*3] = RGB32_R(dat[i]);
      tmp[i*3+1] = RGB32_G(dat[i]);
      tmp[i*3+2] = RGB32_B(dat[i]);
    }
  }
  else if(Params.colors == 4)
  {
    for(size_t i=0; i < size; i++)
    {
      uint32_t white = MAX3(RGB32_R(dat[i]), RGB32_G(dat[i]), RGB32_B(dat[i]));
      tmp[i*4] = (white - RGB32_R(dat[i]));
      tmp[i*4+1] = (white - RGB32_G(dat[i]));
      tmp[i*4+2] = (white - RGB32_B(dat[i]));
      tmp[i*4+3] = 255-white;
    }
  }
}


void fixup_scale(double& x_scale, double& y_scale, double& x_offset, double& y_offset,
                 bool& rotate, double& w_in, double& h_in, const PrintParameters& Params)
{
  // If the page is landscape, contunue as if it is not, but remember to rotate
  if(w_in > h_in)
  {
    std::swap(w_in, h_in);
    rotate = true;
  }

  // Scale to fit as if we had a symmetric resolution
  // ...this makes determining fitment easier
  PrintParameters tmp = Params;
  tmp.hwResW = std::min(Params.hwResW, Params.hwResH);
  tmp.hwResH = std::min(Params.hwResW, Params.hwResH);

  bool raster = Params.format == PrintParameters::PWG ||
                Params.format == PrintParameters::URF;

  size_t h_out = raster ? tmp.getPaperSizeHInPixels()
                        : tmp.getPaperSizeHInPoints();
  size_t w_out = raster ? tmp.getPaperSizeWInPixels()
                        : tmp.getPaperSizeWInPoints();
  double scale = round2(std::min(w_out/w_in, h_out/h_in));
  x_offset = roundf((w_out-(w_in*scale))/2);
  y_offset = roundf((h_out-(h_in*scale))/2);

  x_scale = scale;
  y_scale = scale;

  // Finally, if we have an asymmetric resolution
  // and are not working in absolute dimensions (points), compensate for it.
  if(raster)
  { // URF will/should not end up here, but still...
    if(Params.hwResW > Params.hwResH)
    {
      x_scale *= (Params.hwResW/Params.hwResH);
      x_offset *= (Params.hwResW/Params.hwResH);
    }
    else if(Params.hwResH > Params.hwResW)
    {
      y_scale *= (Params.hwResH/Params.hwResW);
      y_offset *= (Params.hwResH/Params.hwResW);
    }
  }
}
