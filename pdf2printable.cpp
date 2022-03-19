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

#ifndef PDF_CREATOR
#define PDF_CREATOR "pdf2printable"
#endif

#define CHECK(call) if(!(call)) {res = 1; goto error;}

void fixup_scale(double& x_scale, double& y_scale, double& x_offset, double& y_offset,
                 bool& rotate, double& w_in, double& h_in, PrintParameters Params);

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

int pdf_to_printable(std::string Infile, write_fun WriteFun, PrintParameters Params,
                     progress_fun ProgressFun, bool Verbose)
{
  int res = 0;

  bool raster = Params.format == PrintParameters::PWG ||
                Params.format == PrintParameters::URF;

  Bytestream bmp_bts;
  Bytestream OutBts;

  #if MADNESS
  #include "libfuncs"
  #endif

  cairo_surface_t* surface;

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

  size_t out_page_no = 0;
  size_t total_pages = Params.getToPage(pages) - Params.getFromPage() + 1;

  if(raster)
  {
    surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
                                         Params.getPaperSizeWInPixels(),
                                         Params.getPaperSizeHInPixels());
    Bytestream FileHdr;
    if(Params.format == PrintParameters::URF)
    {
      FileHdr = make_urf_file_hdr(total_pages);
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

  for(size_t page_index = 0; page_index < pages; page_index++)
  {
    if((page_index+1) < Params.getFromPage() ||
       (page_index+1) > Params.getToPage(pages))
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

    double x_scale = 0;
    double y_scale = 0;
    double x_offset = 0;
    double y_offset = 0;
    bool rotate = false;
    fixup_scale(x_scale, y_scale, x_offset, y_offset, rotate,
                page_width, page_height, Params);

    cairo_translate(cr, x_offset, y_offset);
    cairo_scale(cr, x_scale, y_scale);

    if (rotate)
    { // Rotate to portrait
      cairo_matrix_init(&m, 0, -1, 1, 0, 0, page_height);
      cairo_transform(cr, &m);
    }

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
      if(bmp_bts.size() != (Params.getPaperSizeWInPixels() *
                            Params.getPaperSizeHInPixels() *
                            Params.colors))
      {
        bmp_bts = Bytestream(Params.getPaperSizeWInPixels() *
                             Params.getPaperSizeHInPixels() *
                             Params.colors);
      }
      uint8_t* tmp = bmp_bts.raw();

      if(Params.colors == 1)
      {
        for(size_t i=0; i<(Params.getPaperSizeWInPixels() *
                           Params.getPaperSizeHInPixels()); i++)
        {
          tmp[i] = (RGB32_R(dat[i])*R_RELATIVE_LUMINOSITY)
                 + (RGB32_G(dat[i])*G_RELATIVE_LUMINOSITY)
                 + (RGB32_B(dat[i])*B_RELATIVE_LUMINOSITY);
        }
      }
      else if(Params.colors == 3)
      {
        for(size_t i=0; i<(Params.getPaperSizeWInPixels() *
                           Params.getPaperSizeHInPixels()); i++)
        {
          tmp[i*3] = RGB32_R(dat[i]);
          tmp[i*3+1] = RGB32_G(dat[i]);
          tmp[i*3+2] = RGB32_B(dat[i]);
        }
      }

      bmp_to_pwg(bmp_bts, OutBts, out_page_no, Params, Verbose);
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
                 bool& rotate, double& w_in, double& h_in, PrintParameters Params)
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

  bool pointsBased = Params.format == PrintParameters::PDF ||
                     Params.format == PrintParameters::Postscript;

  size_t h_out = pointsBased ? tmp.getPaperSizeHInPoints()
                             : tmp.getPaperSizeHInPixels();
  size_t w_out = pointsBased ? tmp.getPaperSizeWInPoints()
                             : tmp.getPaperSizeWInPixels();
  double scale = round2(std::min(w_out/w_in, h_out/h_in));
  x_offset = roundf((w_out-(w_in*scale))/2);
  y_offset = roundf((h_out-(h_in*scale))/2);

  x_scale = scale;
  y_scale = scale;

  // Finally, if we have an asymmetric resolution, compensate for it
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
