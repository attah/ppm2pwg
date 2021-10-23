#include <poppler/glib/poppler.h>
#include <poppler/glib/poppler-document.h>
#include <cairo.h>
#include <cairo-ps.h>
#include <cairo-pdf.h>

#include <iostream>

int main(int argc, char** argv)
{
  if(argc != 3)
  {
    std::cerr << "Usage: pdf2printable <PDF-file> <outfile>" << std::endl;
    return 1;
  }

  std::cout << argv[1] << std::endl;

  std::string url("file://");
  url.append(argv[1]);
  GError* error = nullptr;
  PopplerDocument* doc = poppler_document_new_from_file(url.c_str(), nullptr, &error);

  if(!doc)
  {
    std::cerr << "Failed to open PDF: " << error->message << std::endl;
    return 1;
  }

  double w = 4960;
  double h = 7016;
  int dpi = 600;
  double w_pts = (w/dpi)*72.0;
  double h_pts = (h/dpi)*72.0;

  cairo_surface_t *surface =
    cairo_pdf_surface_create(argv[2], w_pts, h_pts);

  int pages = poppler_document_get_n_pages(doc);
  for(int i = 0; i < pages; i++)
  {
    std::cout << "Page " << i << std::endl;
    PopplerPage* page = poppler_document_get_page(doc, i);
    double page_width, page_height;

    poppler_page_get_size(page, &page_width, &page_height);
    std::cout << page_width << "x" << page_height << std::endl;

    cairo_pdf_surface_set_size(surface, w_pts, h_pts);

    cairo_t *cr;
    cairo_status_t status;
    cairo_matrix_t m;

    cr = cairo_create(surface);

    if (page_width > page_height) {
        // rotate 90 deg for landscape
        cairo_translate(cr, 0, h_pts);
        cairo_matrix_init(&m, 0, -1, 1, 0, 0, 0);
        cairo_transform(cr, &m);
        std::swap(page_width, page_height);
    }

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
}
