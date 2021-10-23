#include <poppler/glib/poppler.h>
#include <poppler/glib/poppler-document.h>
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-page-renderer.h>
#include <poppler/cpp/poppler-image.h>

#include <cairo.h>
#include <cairo-ps.h>
#include <cairo-pdf.h>

#include <iostream>
#include <fstream>

using namespace poppler;

int to_raster(std::string infile, std::string outfile,
              double w, double h, int dpi, bool urf, bool duplex, int colors);
int to_pdf_or_ps(std::string infile, std::string outfile,
                 double w, double h, int dpi, bool ps, bool duplex);

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

  bool ps = true;
  bool duplex = true;

  bool pwg = true;
  bool urf = false;

  if(pwg || urf)
  {
    char tmpfile[] = "/tmp/pdf2printable_XXXXXX";
    int fd = mkstemp(tmpfile);
    close(fd);
    int res = to_pdf_or_ps(infile, tmpfile, w, h, dpi, false, duplex);
    if(res)
    {
      remove(tmpfile);
      return res;
    }
    res = to_raster(tmpfile, outfile, w, h, dpi, urf, duplex, 3);
    remove(tmpfile);
    return res;
  }
  else
  {
    return to_pdf_or_ps(infile, outfile, w, h, dpi, ps, duplex);
  }
}

int to_raster(std::string infile, std::string outfile,
              double w, double h, int dpi, bool urf, bool duplex, int colors)
{
  document* doc  = document::load_from_file(infile);

  if(doc == NULL)
  {
    std::cerr << "Failed to open PDF" << std::endl;
    return 1;
  }

  int pages = doc->pages();
  page_renderer renderer;
  renderer.set_image_format(colors == 3 ? image::format_rgb24 : image::format_gray8);

  std::ofstream of(outfile, std::ofstream::out);
  of << (colors==3 ? "P6" : "P5") << '\n' << w << ' ' << h << '\n' << 255 << '\n';

  for(int i = 0; i < pages; i++)
  {
    std::cout << "Page " << i << std::endl;
    page* p = doc->create_page(i);
    image img = renderer.render_page(p, dpi, dpi, -1, -1, w, h);

    std::cout << w << "x" << h << std::endl;

    of.write(img.data(), img.height()*img.bytes_per_row());

    delete p;
  }

  delete doc;
  return 0;
}

int to_pdf_or_ps(std::string infile, std::string outfile,
                 double w, double h, int dpi, bool ps, bool duplex)
{
  double w_pts = (w/dpi)*72.0;
  double h_pts = (h/dpi)*72.0;
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

  if(ps)
  {
    surface = cairo_ps_surface_create(outfile.c_str(), w, h);
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

    if(ps)
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

    if (page_width > page_height) {
        // Fix landscape pages
        cairo_translate(cr, 0, h_pts);
        cairo_matrix_init(&m, 0, -1, 1, 0, 0, 0);
        cairo_transform(cr, &m);
        std::swap(page_width, page_height);
    }

    // TODO: find minimum scale and center the other axis
    double h_scale = h_pts/page_height;
    double w_scale = w_pts/page_width;

    cairo_scale(cr, w_scale, h_scale);

    poppler_page_render_for_printing(page, cr);

    status = cairo_status(cr);
    if (status)
    {
      std::cerr << "cairo error: " << cairo_status_to_string(status) << std::endl;
    }
    cairo_destroy(cr);
    // g_free(page);
    cairo_surface_show_page(surface);
  }

  cairo_surface_finish(surface);
  cairo_surface_destroy(surface);
  g_free(doc);
  return 0;
}
