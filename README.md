# ppm2pwg - misc printing format conversion utilities
(should really be renamed)

Available as rudimentary standalone applications, but mainly made for use in [SeaPrint](https://github.com/attah/harbour-seaprint).

## ppm2pwg
Takes a pgm or ppm (P5 or P6 "raw") bitmap image and converts to PWG or URF printer raster format.

...or a completely raw bitmap in the c++ api.

## pwg2ppm
For debugging. Similar to [rasterview](https://github.com/michaelrsweet/rasterview), but without a GUI. Takes a PWG or URF printer raster and outputs a P5 or P6 pgm/ppm image.

## pdf2printable
Takes a pdf document and makes it suitable for printing, by:
- rotate and scale to fit as needed to a desired page size
- convert to PDF 1.5, Postscript 2 or PWG/URF raster

## baselinify
Takes a JPEG and losslessly repacks it to the baseline ecoding profile. Like jpegtran without any arguments, but reusable in C++.

IPP-printers are only required to support baseline-encoded jpeg according to PWG5100.14.

Despite working with in-memory data, it only requires the libjpeg 62.2.0 and not 62.3.0/7.3+ API, so it works on conservative distros.