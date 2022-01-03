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

#define DPMM(dpi) dpi/25.4

#ifndef PDF_CREATOR
#define PDF_CREATOR "pdf2printable"
#endif

#define CHECK(call) if(!(call)) {res = 1; goto error;}

void fixup_scale(double& x_scale, double& y_scale, double& x_offset, double& y_offset,
                 double w_in, double h_in, double w_out, double h_out,
                 size_t HwResX, size_t HwResY);

cairo_status_t bytestream_writer(void* bts, const unsigned char* data, unsigned int length)
{
  ((Bytestream*)bts)->putBytes(data, length);
  return CAIRO_STATUS_SUCCESS;
}

double round2(double d)
{
  return round(d*100)/100;
}

int pdf_to_printable(std::string Infile, write_fun WriteFun, size_t Colors, size_t Quality,
                     std::string PaperSizeName, float PaperSizeX, float PaperSizeY,
                     size_t HwResX, size_t HwResY, Format TargetFormat,
                     bool Duplex, bool Tumble, bool BackHFlip, bool BackVFlip,
                     size_t FromPage, size_t ToPage, progress_fun ProgressFun, bool Verbose)
{
  int res = 0;

  if(TargetFormat == PDF || TargetFormat == Postscript)
  { // Dimensions will be in points
    HwResX = 72;
    HwResY = 72;
  }

  double w = round2(PaperSizeX*DPMM(HwResX));
  double h = round2(PaperSizeY*DPMM(HwResY));
  size_t w_px = w;
  size_t h_px = h;
  bool raster = TargetFormat == PWG || TargetFormat == URF;

  Bytestream bmp_bts;
  Bytestream OutBts;

  #if MADNESS
  #include "libfuncs"
  #endif

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
  size_t total_pages = ToPage - (FromPage-1);

  if(raster)
  {
    surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, w_px, h_px);
    Bytestream FileHdr;
    if(TargetFormat == URF)
    {
      FileHdr = make_urf_file_hdr(total_pages);
    }
    else
    {
      FileHdr = make_pwg_file_hdr();
    }
    CHECK(WriteFun(FileHdr.raw(), FileHdr.size()));
  }
  else if(TargetFormat == PDF)
  {
    surface = cairo_pdf_surface_create_for_stream(bytestream_writer, &OutBts, w, h);
    cairo_pdf_surface_set_metadata(surface, CAIRO_PDF_METADATA_CREATOR, PDF_CREATOR);
    cairo_pdf_surface_set_size(surface, w, h);
  }
  else if(TargetFormat == Postscript)
  {
    surface = cairo_ps_surface_create_for_stream(bytestream_writer, &OutBts, w, h);
    cairo_ps_surface_restrict_to_level(surface, CAIRO_PS_LEVEL_2);
    if(Duplex)
    {
      cairo_ps_surface_dsc_comment(surface, "%%Requirements: duplex");
      cairo_ps_surface_dsc_begin_setup(surface);
      cairo_ps_surface_dsc_comment(surface, "%%IncludeFeature: *Duplex DuplexNoTumble");
    }
    cairo_ps_surface_dsc_begin_page_setup(surface);
    cairo_ps_surface_set_size(surface, w, h);
  }
  else
  {
    g_object_unref(doc);
    return 1;
  }

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
        cairo_matrix_init(&m, 0, -1, 1, 0, 0, h);
        cairo_transform(cr, &m);
        std::swap(page_width, page_height);
    }

    double x_scale = 0;
    double y_scale = 0;
    double x_offset = 0;
    double y_offset = 0;
    fixup_scale(x_scale, y_scale, x_offset, y_offset,
                page_width, page_height, PaperSizeX, PaperSizeY, HwResX, HwResY);
    cairo_translate(cr, x_offset, y_offset);
    cairo_scale(cr, x_scale, y_scale);

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
                 BackHFlip, BackVFlip, Verbose);
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

void fixup_scale(double& x_scale, double& y_scale, double& x_offset, double& y_offset,
                 double w_in, double h_in, double w_out, double h_out,
                 size_t HwResX, size_t HwResY)
{
  // First, scale to fit as if we had a symmetric resolution
  // ...this makes determining fitment easier
  size_t min_res = std::min(HwResX, HwResY);
  h_out *= DPMM(min_res);
  w_out *= DPMM(min_res);
  double scale = round2(std::min(w_out/w_in, h_out/h_in));
  x_offset = roundf((w_out-(w_in*scale))/2);
  y_offset = roundf((h_out-(h_in*scale))/2);

  x_scale = scale;
  y_scale = scale;

  // Second, if we have an asymmetric resolution, compensate for it
  if(HwResX > HwResY)
  {
    x_scale *= (HwResX/HwResY);
    x_offset *= (HwResX/HwResY);
  }
  else if(HwResY > HwResX)
  {
    y_scale *= (HwResY/HwResX);
    y_offset *= (HwResY/HwResX);
  }
}
