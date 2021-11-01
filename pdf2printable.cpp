#include <poppler/glib/poppler.h>
#include <poppler/glib/poppler-document.h>

#include <cairo.h>
#include <cairo-ps.h>
#include <cairo-pdf.h>

#include <iostream>
#include <fstream>
#include <math.h>
#include <functional>

#include <bytestream.h>
#include "pwgpapersizes.h"
#include "ppm2pwg.h"

#define R_RELATIVE_LUMINOSITY 0.299
#define G_RELATIVE_LUMINOSITY 0.587
#define B_RELATIVE_LUMINOSITY 0.114
#define RGB32_R(Color) ((dat[i]>>16)&0xff)
#define RGB32_G(Color) ((dat[i]>>8)&0xff)
#define RGB32_B(Color) (dat[i]&0xff)

bool getenv_bool(std::string VarName);
int getenv_int(std::string VarName, int Default);
std::string getenv_str(std::string VarName, std::string Default);
typedef std::function<bool(unsigned char const*, unsigned int)> write_fun;

enum Format
{
  PDF,
  Postscript,
  PWG,
  URF
};

cairo_status_t lambda_adapter(void* lambda, const unsigned char* data, unsigned int length)
{
  return (*(write_fun*)lambda)(data, length) ? CAIRO_STATUS_SUCCESS : CAIRO_STATUS_WRITE_ERROR;
}

int do_convert(std::string Infile, write_fun WriteFun, size_t Colors, size_t Quality,
               std::string PaperSizeName, float PaperSizeX, float PaperSizeY,
               size_t HwResX, size_t HwResY, Format TargetFormat,
               bool Duplex, bool Tumble, bool BackHFlip, bool BackVFlip,
               size_t FromPage, size_t ToPage);

void fixup_scale(double& scale, double& x_offset, double& y_offset,
                 double w_in, double h_in, double w_out, double h_out);

double round2(double d)
{
  return roundf(d*100)/100;
}

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

  size_t FromPage = getenv_int("FROM_PAGE", 0);
  size_t ToPage = getenv_int("TO_PAGE", 0);

  bool Duplex = true;
  bool Tumble = false;
  bool BackHFlip = false;
  bool BackVFlip = false;
  size_t Colors = getenv_int("COLORS", 3);
  size_t Quality = getenv_int("QUALITY", 4);

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

  std::ofstream of = std::ofstream(Outfile, std::ofstream::out);
  write_fun WriteFun([&of](unsigned char const* buf, unsigned int len) -> bool
            {
              of.write((char*)buf, len);
              return of.exceptions() == std::ostream::goodbit;
            });

  return do_convert(Infile, WriteFun, Colors, Quality,
                    PaperSizeName, PaperSizeX, PaperSizeY,
                    HwResX, HwResY, TargetFormat,
                    Duplex, Tumble, BackHFlip, BackVFlip,
                    FromPage, ToPage);
}

int do_convert(std::string Infile, write_fun WriteFun, size_t Colors, size_t Quality,
               std::string PaperSizeName, float PaperSizeX, float PaperSizeY,
               size_t HwResX, size_t HwResY, Format TargetFormat,
               bool Duplex, bool Tumble, bool BackHFlip, bool BackVFlip,
               size_t FromPage, size_t ToPage)
{
  double w_pts = PaperSizeX/25.4*72.0;
  double h_pts = PaperSizeY/25.4*72.0;
  size_t w_px = PaperSizeX/25.4*HwResX;
  size_t h_px = PaperSizeY/25.4*HwResY;
  bool raster = TargetFormat == PWG || TargetFormat == URF;

  Bytestream bmp_bts;
  Bytestream OutBts;

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
  if(ToPage == 0)
  {
    ToPage = pages;
  }

  std::ofstream of;

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
    WriteFun(FileHdr.raw(), FileHdr.size());
  }
  else if(TargetFormat == PDF)
  {
    surface = cairo_pdf_surface_create_for_stream(lambda_adapter, &WriteFun, w_pts, h_pts);
    cairo_pdf_surface_set_size(surface, w_pts, h_pts);
  }

  else if(TargetFormat == Postscript)
  {
    surface = cairo_ps_surface_create_for_stream(lambda_adapter, &WriteFun, w_pts, h_pts);
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
    g_free(doc);
    return 1;
  }

  size_t page_no;
  for(size_t page_index = 0; page_index < pages; page_index++)
  {
    page_no = page_index+1;
    if(page_no < FromPage || page_no > ToPage)
    {
      continue;
    }

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
      OutBts.reset();
      bmp_to_pwg(bmp_bts, OutBts, TargetFormat==URF,
                 page_no, Colors, Quality,
                 HwResX, HwResY, w_px, h_px,
                 Duplex, Tumble, PaperSizeName,
                 BackHFlip, BackVFlip);

      WriteFun(OutBts.raw(), OutBts.size());
    }
  }

  cairo_surface_finish(surface);
  cairo_surface_destroy(surface);
  g_free(doc);
  return 0;
}

void fixup_scale(double& scale, double& x_offset, double& y_offset,
                 double w_in, double h_in, double w_out, double h_out)
{
  scale = round2(std::min(w_out/w_in, h_out/h_in));
  x_offset = roundf((w_out-(w_in*scale))/2);
  y_offset = roundf((h_out-(h_in*scale))/2);
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
