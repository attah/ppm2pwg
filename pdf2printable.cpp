#include <poppler/glib/poppler.h>
#include <poppler/glib/poppler-document.h>

#include <cairo.h>
#include <cairo-ps.h>
#include <cairo-pdf.h>

#include <iostream>
#include <fstream>

int to_pdf_or_ps(std::string infile, std::string outfile, int colors,
                 double w, double h, int dpi, bool ps, bool pwg, bool urf,
                 bool duplex);

int main(int argc, char** argv)
{
  if(argc != 3)
  {
    std::cerr << "Usage: pdf2printable <PDF-file> <outfile>" << std::endl;
    return 1;
  }

  std::cout << argv[1] << std::endl;

  std::string infile(argv[1]);
  std::string outfile(argv[2]);

  double w = 4960;
  double h = 7016;
  int dpi = 600;

  bool ps = false;
  bool duplex = true;

  bool pwg = false;
  bool urf = false;

  int colors = 3;

  return to_pdf_or_ps(infile, outfile, colors, w, h, dpi, ps, pwg, urf, duplex);
}

int to_pdf_or_ps(std::string infile, std::string outfile, int colors,
                 double w, double h, int dpi, bool ps, bool pwg, bool urf,
                 bool duplex)
{
  double w_pts = (w/dpi)*72.0;
  double h_pts = (h/dpi)*72.0;
  bool raster = pwg | urf;
  cairo_surface_t *surface;

  std::string url("file://");
  url.append(infile);
  GError* error = nullptr;
  PopplerDocument* doc = poppler_document_new_from_file(url.c_str(), nullptr, &error);

  if(doc == NULL)
  {
    std::cerr << "Failed to open PDF: " << error->message << std::endl;
    return 1;
  }

  if(raster)
  {
    surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, w, h);
  }
  else if(ps)
  {
    surface = cairo_ps_surface_create(outfile.c_str(), w_pts, h_pts);
    cairo_ps_surface_restrict_to_level(surface, CAIRO_PS_LEVEL_2);
    if(duplex)
    {
      cairo_ps_surface_dsc_comment(surface, "%%Requirements: duplex");
      cairo_ps_surface_dsc_begin_setup(surface);
      cairo_ps_surface_dsc_comment(surface, "%%IncludeFeature: *Duplex DuplexNoTumble");
    }
    cairo_ps_surface_dsc_begin_page_setup(surface);
  }
  else
  {
    surface = cairo_pdf_surface_create(outfile.c_str(), w_pts, h_pts);
  }

  int pages = poppler_document_get_n_pages(doc);
  for(int i = 0; i < pages; i++)
  {
    std::cout << "Page " << i << std::endl;
    PopplerPage* page = poppler_document_get_page(doc, i);
    double page_width, page_height;

    poppler_page_get_size(page, &page_width, &page_height);
    std::cout << page_width << "x" << page_height << std::endl;

    if(raster)
    {
      //ok
    }
    else if(ps)
    {
      cairo_ps_surface_set_size(surface, w_pts, h_pts);
    }
    else
    {
      cairo_pdf_surface_set_size(surface, w_pts, h_pts);
    }

    cairo_t *cr;
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
        cairo_translate(cr, 0, raster ? h : h_pts);
        cairo_matrix_init(&m, 0, -1, 1, 0, 0, 0);
        cairo_transform(cr, &m);
        std::swap(page_width, page_height);
    }

    if(raster)
    { // Scale to a pixel size
      // TODO: find minimum scale and center the other axis
      double h_scale = h/page_height;
      double w_scale = w/page_width;

      cairo_scale(cr, h_scale, w_scale);
    }
    else
    { // Scale to a poins size
      // TODO: find minimum scale and center the other axis
      double h_scale = h_pts/page_height;
      double w_scale = w_pts/page_width;

      cairo_scale(cr, w_scale, h_scale);
    }

    poppler_page_render_for_printing(page, cr);
    //g_object_unref(page);

    status = cairo_status(cr);
    if (status)
    {
      std::cerr << "cairo error: " << cairo_status_to_string(status) << std::endl;
    }
    cairo_destroy(cr);
    // g_free(page);
    cairo_surface_show_page(surface);

    if(raster)
    {
      int h = cairo_image_surface_get_height(surface);
      int w = cairo_image_surface_get_width(surface);

      cairo_surface_flush(surface);
      std::ofstream of(outfile, std::ofstream::out);
      of << (colors==3 ? "P6" : "P5") << '\n' << w << ' ' << h << '\n' << 255 << '\n';

      char* dat = (char*)cairo_image_surface_get_data(surface);
      char* tmp = new char[w*h*colors];

      for(int i=0; i<(w*h); i++)
      {
        memcpy(&tmp[i*3], &dat[i*4], 3);
      }
      of.write(tmp, w*h*colors);
      delete tmp;
    }

  }

  cairo_surface_finish(surface);
  cairo_surface_destroy(surface);
  g_free(doc);
  return 0;
}
