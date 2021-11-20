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
#define RGB32_R(Color) ((dat[i]>>16)&0xff)
#define RGB32_G(Color) ((dat[i]>>8)&0xff)
#define RGB32_B(Color) (dat[i]&0xff)

#define CHECK(call) if(!(call)) {res = 1; goto error;}

void fixup_scale(double& scale, double& x_offset, double& y_offset,
                 double w_in, double h_in, double w_out, double h_out);

cairo_status_t bytestream_writer(void* bts, const unsigned char* data, unsigned int length)
{
  ((Bytestream*)bts)->putBytes(data, length);
  return CAIRO_STATUS_SUCCESS;
}

double round2(double d)
{
  return roundf(d*100)/100;
}

int pdf_to_printable(std::string Infile, write_fun WriteFun, size_t Colors, size_t Quality,
                     std::string PaperSizeName, float PaperSizeX, float PaperSizeY,
                     size_t HwResX, size_t HwResY, Format TargetFormat,
                     bool Duplex, bool Tumble, bool BackHFlip, bool BackVFlip,
                     size_t FromPage, size_t ToPage, progress_fun ProgressFun)
{
  int res = 0;
  double w_pts = PaperSizeX/25.4*72.0;
  double h_pts = PaperSizeY/25.4*72.0;
  size_t w_px = PaperSizeX/25.4*HwResX;
  size_t h_px = PaperSizeY/25.4*HwResY;
  bool raster = TargetFormat == PWG || TargetFormat == URF;

  Bytestream bmp_bts;
  Bytestream OutBts;

  LIB(poppler, "libpoppler-glib.so.8");
  FUNC(poppler, PopplerDocument*, poppler_document_new_from_file, const char*, const char*, GError**);
  FUNC(poppler, int, poppler_document_get_n_pages, PopplerDocument*);
  FUNC(poppler, PopplerPage*, poppler_document_get_page, PopplerDocument*, int);
  FUNC(poppler, void, poppler_page_get_size, PopplerPage*, double*, double*);
  FUNC(poppler, void, poppler_page_render_for_printing, PopplerPage*, cairo_t*);

  LIB(cairo, "libcairo.so.2");
  FUNC(cairo, cairo_surface_t*, cairo_ps_surface_create_for_stream, cairo_write_func_t, void*, double, double);
  FUNC(cairo, void, cairo_ps_surface_restrict_to_level, cairo_surface_t*, cairo_ps_level_t);
  FUNC(cairo, void, cairo_ps_surface_dsc_comment, cairo_surface_t*, const char*);
  FUNC(cairo, void, cairo_ps_surface_dsc_begin_setup, cairo_surface_t*);
  FUNC(cairo, void, cairo_ps_surface_dsc_begin_page_setup, cairo_surface_t*);
  FUNC(cairo, void, cairo_ps_surface_set_size, cairo_surface_t*, double, double);
  FUNC(cairo, cairo_surface_t*, cairo_image_surface_create, cairo_format_t, int, int);
  FUNC(cairo, cairo_surface_t*, cairo_pdf_surface_create_for_stream, cairo_write_func_t, void*, double, double);
  FUNC(cairo, void, cairo_pdf_surface_set_size, cairo_surface_t*, double, double);
  FUNC(cairo, void, cairo_translate, cairo_t*, double, double);
  FUNC(cairo, void, cairo_scale, cairo_t*, double, double);
  FUNC(cairo, cairo_status_t, cairo_status, cairo_t*);
  FUNC(cairo, const char*, cairo_status_to_string, cairo_status_t);
  FUNC(cairo, void, cairo_destroy, cairo_t*);
  FUNC(cairo, void, cairo_surface_show_page, cairo_surface_t*);
  FUNC(cairo, cairo_t*, cairo_create, cairo_surface_t*);
  FUNC(cairo, void, cairo_matrix_init, cairo_matrix_t*, double, double, double, double, double, double);
  FUNC(cairo, void, cairo_transform, cairo_t*, const cairo_matrix_t*);
  FUNC(cairo, void, cairo_save, cairo_t*);
  FUNC(cairo, void, cairo_set_source_rgb, cairo_t*, double, double, double);
  FUNC(cairo, void, cairo_paint, cairo_t*);
  FUNC(cairo, void, cairo_restore, cairo_t*);
  FUNC(cairo, void, cairo_surface_flush, cairo_surface_t*);
  FUNC(cairo, unsigned char*, cairo_image_surface_get_data, cairo_surface_t*);
  FUNC(cairo, void, cairo_surface_finish, cairo_surface_t*);
  FUNC(cairo, void, cairo_surface_destroy, cairo_surface_t*);

  cairo_surface_t* surface;
  std::string url("file://");
  url.append(Infile);
  GError* error = nullptr;
  PopplerDocument* doc = poppler_document_new_from_file(url.c_str(), nullptr, &error);

  if(doc == NULL)
  {
    std::cerr << "Failed to open PDF: " << error->message << std::endl;
    return 1;
  }

  size_t pages = poppler_document_get_n_pages(doc);
  if(FromPage == 0)
  {
    FromPage = 1;
  }
  if(ToPage == 0 || ToPage > pages)
  {
    ToPage = pages;
  }

  size_t out_page_no = 0;
  size_t total_pages = 0;

  if(raster)
  {
    surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, w_px, h_px);
    Bytestream FileHdr;
    if(TargetFormat==URF)
    {
      FileHdr = make_urf_file_hdr(pages);
    }
    else
    {
      FileHdr = make_pwg_file_hdr();
    }
    CHECK(WriteFun(FileHdr.raw(), FileHdr.size()));
  }
  else if(TargetFormat == PDF)
  {
    surface = cairo_pdf_surface_create_for_stream(bytestream_writer, &OutBts, w_pts, h_pts);
    cairo_pdf_surface_set_size(surface, w_pts, h_pts);
  }
  else if(TargetFormat == Postscript)
  {
    surface = cairo_ps_surface_create_for_stream(bytestream_writer, &OutBts, w_pts, h_pts);
    cairo_ps_surface_restrict_to_level(surface, CAIRO_PS_LEVEL_2);
    if(Duplex)
    {
      cairo_ps_surface_dsc_comment(surface, "%%Requirements: duplex");
      cairo_ps_surface_dsc_begin_setup(surface);
      cairo_ps_surface_dsc_comment(surface, "%%IncludeFeature: *Duplex DuplexNoTumble");
    }
    cairo_ps_surface_dsc_begin_page_setup(surface);
    cairo_ps_surface_set_size(surface, w_pts, h_pts);
  }
  else
  {
    g_object_unref(doc);
    return 1;
  }

  total_pages = ToPage - (FromPage-1);
  for(size_t page_index = 0; page_index < pages; page_index++)
  {
    if((page_index+1) < FromPage || (page_index+1) > ToPage)
    {
      continue;
    }
    out_page_no++;

    PopplerPage* page = poppler_document_get_page(doc, page_index);
    double page_width, page_height;
    poppler_page_get_size(page, &page_width, &page_height);

    cairo_t* cr;
    cairo_status_t status;
    cairo_matrix_t m;

    cr = cairo_create(surface);

    if(raster)
    {
      cairo_save(cr);
      cairo_set_source_rgb(cr, 1, 1, 1);
      cairo_paint(cr);
      cairo_restore(cr);
    }

    if (page_width > page_height) {
        // Fix landscape pages
        cairo_translate(cr, 0, raster ? h_px : h_pts);
        cairo_matrix_init(&m, 0, -1, 1, 0, 0, 0);
        cairo_transform(cr, &m);
        std::swap(page_width, page_height);
    }

    double scale = 0;
    double x_offset = 0;
    double y_offset = 0;
    if(raster)
    { // Scale to a pixel size
      fixup_scale(scale, x_offset, y_offset, page_width, page_height, w_px, h_px);
    }
    else
    { // Scale to a poins size
      fixup_scale(scale, x_offset, y_offset, page_width, page_height, w_pts, h_pts);
    }
    cairo_translate(cr, x_offset, y_offset);
    cairo_scale(cr, scale, scale);

    poppler_page_render_for_printing(page, cr);
    g_object_unref(page);

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
      if(bmp_bts.size() != (w_px*h_px*Colors))
      {
        bmp_bts = Bytestream(w_px*h_px*Colors);
      }
      uint8_t* tmp = bmp_bts.raw();

      if(Colors == 1)
      {
        for(size_t i=0; i<(w_px*h_px); i++)
        {
          tmp[i] = (RGB32_R(dat[i])*R_RELATIVE_LUMINOSITY)
                 + (RGB32_G(dat[i])*G_RELATIVE_LUMINOSITY)
                 + (RGB32_B(dat[i])*B_RELATIVE_LUMINOSITY);
        }
      }
      else if(Colors == 3)
      {
        for(size_t i=0; i<(w_px*h_px); i++)
        {
          tmp[i*3] = RGB32_R(dat[i]);
          tmp[i*3+1] = RGB32_G(dat[i]);
          tmp[i*3+2] = RGB32_B(dat[i]);
        }
      }

      bmp_to_pwg(bmp_bts, OutBts, TargetFormat==URF,
                 out_page_no, Colors, Quality,
                 HwResX, HwResY, w_px, h_px,
                 Duplex, Tumble, PaperSizeName,
                 BackHFlip, BackVFlip);
    }

    CHECK(WriteFun(OutBts.raw(), OutBts.size()));
    OutBts.reset();

    if(ProgressFun != nullptr)
    {
      ProgressFun(out_page_no, total_pages);
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

void fixup_scale(double& scale, double& x_offset, double& y_offset,
                 double w_in, double h_in, double w_out, double h_out)
{
  scale = round2(std::min(w_out/w_in, h_out/h_in));
  x_offset = roundf((w_out-(w_in*scale))/2);
  y_offset = roundf((h_out-(h_in*scale))/2);
}
