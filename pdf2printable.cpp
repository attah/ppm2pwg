#include <poppler/glib/poppler.h>
#include <poppler/glib/poppler-document.h>

#include <cairo.h>
#include <cairo-ps.h>
#include <cairo-pdf.h>

#include <iostream>
#include <fstream>

#include <bytestream.h>
#include "pwgpapersizes.h"
#include "ppm2pwg.h"

bool getenv_bool(std::string VarName);
int getenv_int(std::string VarName, int Default);
std::string getenv_str(std::string VarName, std::string Default);

enum Format
{
  PDF,
  Postscript,
  PWG,
  URF
};

int do_convert(std::string Infile, std::string Outfile, int Colors,
               std::string PaperSizeName, float PaperSizeX, float PaperSizeY,
               size_t HwResX, size_t HwResY, Format TargetFormat,
               bool Duplex, bool Tumble, bool BackHFlip, bool BackVFlip);

int main(int argc, char** argv)
{
  if(argc != 3)
  {
    std::cerr << "Usage: pdf2printable <PDF-file> <outfile>" << std::endl;
    return 1;
  }

  std::string Infile(argv[1]);
  std::string Outfile(argv[2]);

  Format TargetFormat = PDF;

  size_t HwResX = getenv_int("HWRES_X", getenv_int("HWRES", 300));
  size_t HwResY = getenv_int("HWRES_Y", getenv_int("HWRES", 300));
  std::string PaperSizeName = getenv_str("PAPER_SIZE", "iso_a4_210x297mm");

  std::pair<float, float> PaperSize = PwgPaperSizes.at(PaperSizeName);
  float PaperSizeX = PaperSize.first;
  float PaperSizeY = PaperSize.second;

  bool Duplex = true;
  bool Tumble = false;
  bool BackHFlip = false;
  bool BackVFlip = false;
  int Colors = getenv_int("COLORS", 3);

  std::string format = getenv_str("FORMAT", "pdf");
  if(format == "ps" || format == "postscript")
  {
    TargetFormat = Postscript;
  }
  else if(format == "pwg")
  {
    TargetFormat = PWG;
  }
  else if(format == "urf")
  {
    TargetFormat = URF;
  }
  else if(format != "pdf")
  {
    return 1;
  }

  return do_convert(Infile, Outfile, Colors,
                    PaperSizeName, PaperSizeX, PaperSizeY,
                    HwResX, HwResY, TargetFormat,
                    Duplex, Tumble, BackHFlip, BackVFlip);
}

int do_convert(std::string Infile, std::string Outfile, int Colors,
               std::string PaperSizeName, float PaperSizeX, float PaperSizeY,
               size_t HwResX, size_t HwResY, Format TargetFormat,
               bool Duplex, bool Tumble, bool BackHFlip, bool BackVFlip)
{
  double w_pts = PaperSizeX/25.4*72.0;
  double h_pts = PaperSizeY/25.4*72.0;
  size_t w_px = PaperSizeX/25.4*HwResX;
  size_t h_px = PaperSizeY/25.4*HwResY;
  bool raster = TargetFormat == PWG || TargetFormat == URF;
  cairo_surface_t *surface;

  std::string url("file://");
  url.append(Infile);
  GError* error = nullptr;
  PopplerDocument* doc = poppler_document_new_from_file(url.c_str(), nullptr, &error);

  if(doc == NULL)
  {
    std::cerr << "Failed to open PDF: " << error->message << std::endl;
    return 1;
  }

  std::ofstream of;

  if(raster)
  {
    surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, w_px, h_px);
    of = std::ofstream(Outfile, std::ofstream::out);

  }
  else if(TargetFormat == Postscript)
  {
    surface = cairo_ps_surface_create(Outfile.c_str(), w_pts, h_pts);
    cairo_ps_surface_restrict_to_level(surface, CAIRO_PS_LEVEL_2);
    if(Duplex)
    {
      cairo_ps_surface_dsc_comment(surface, "%%Requirements: duplex");
      cairo_ps_surface_dsc_begin_setup(surface);
      cairo_ps_surface_dsc_comment(surface, "%%IncludeFeature: *Duplex DuplexNoTumble");
    }
    cairo_ps_surface_dsc_begin_page_setup(surface);
  }
  else
  {
    surface = cairo_pdf_surface_create(Outfile.c_str(), w_pts, h_pts);
  }

  int pages = poppler_document_get_n_pages(doc);
  for(int i = 0; i < pages; i++)
  {
    PopplerPage* page = poppler_document_get_page(doc, i);
    double page_width, page_height;

    poppler_page_get_size(page, &page_width, &page_height);

    if(raster)
    {
      //ok
    }
    else if(TargetFormat == Postscript)
    {
      cairo_ps_surface_set_size(surface, w_pts, h_pts);
    }
    else
    {
      cairo_pdf_surface_set_size(surface, w_pts, h_pts);
    }

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

    if(raster)
    { // Scale to a pixel size
      // TODO: find minimum scale and center the other axis
      double w_scale = w_px/page_width;
      double h_scale = h_px/page_height;

      cairo_scale(cr, h_scale, w_scale);
    }
    else
    { // Scale to a poins size
      // TODO: find minimum scale and center the other axis
      double w_scale = w_pts/page_width;
      double h_scale = h_pts/page_height;

      cairo_scale(cr, w_scale, h_scale);
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
      int h = cairo_image_surface_get_height(surface);
      int w = cairo_image_surface_get_width(surface);

      cairo_surface_flush(surface);
      if(i==0)
      {
        Bytestream FileHdr;
        if(TargetFormat==URF)
        {
          uint32_t pages = getenv_int("PAGES", 1);
          FileHdr = make_urf_file_hdr(pages);
        }
        else
        {
          FileHdr = make_pwg_file_hdr();
        }
        of << FileHdr;
      }

      uint32_t* dat = (uint32_t*)cairo_image_surface_get_data(surface);
      Bytestream bmp_bts(w*h*Colors);
      uint8_t* tmp = bmp_bts.raw();

      if(Colors == 1)
      {
        for(int i=0; i<(w*h); i++)
        {
          tmp[i] = ((dat[i]&0xff)*0.114) // R
                 + (((dat[i]>>8)&0xff)*0.587) // G
                 + (((dat[i]>>16)&0xff)*0.299); // B
        }
      }
      else if(Colors == 3)
      {
        for(int i=0; i<(w*h); i++)
        {
          tmp[i*3] = (dat[i]>>16)&0xff; // R
          tmp[i*3+1] = (dat[i]>>8)&0xff; // G
          tmp[i*3+2] = dat[i]&0xff; // B
        }
      }
      Bytestream OutBts;
      bmp_to_pwg(bmp_bts, OutBts, TargetFormat==URF,
                 i+1, Colors, 4,
                 HwResX, HwResY, w, h,
                 Duplex, Tumble, PaperSizeName,
                 BackHFlip, BackVFlip);

     of << OutBts;
    }
  }

  cairo_surface_finish(surface);
  cairo_surface_destroy(surface);
  g_free(doc);
  return 0;
}

bool getenv_bool(std::string VarName)
{
  char* tmp = getenv(VarName.c_str());
  return (tmp && strcmp(tmp,"0")!=0 && strcmp(tmp,"false")!=0);
}

int getenv_int(std::string VarName, int Default)
{
  char* tmp = getenv(VarName.c_str());
  return tmp ? atoi(tmp) : Default;
}

std::string getenv_str(std::string VarName, std::string Default)
{
  char* tmp = getenv(VarName.c_str());
  return tmp ? tmp : Default;
}
