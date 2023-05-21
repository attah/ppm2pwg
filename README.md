# ppm2pwg - misc printing format conversion utilities
(should really be renamed)

[![C/C++ CI](https://github.com/attah/ppm2pwg/workflows/C%2FC%2B%2B%20CI/badge.svg)](https://github.com/attah/ppm2pwg/actions/workflows/ccpp.yml)
[![CodeQL](https://github.com/attah/ppm2pwg/workflows/CodeQL/badge.svg)](https://github.com/attah/ppm2pwg/actions/workflows/codeql-analysis.yml)

Available as rudimentary standalone applications, but mainly made for use in [SeaPrint](https://github.com/attah/harbour-seaprint).

## ppm2pwg
Takes a pbm, pgm or ppm (P4, P5 or P6 "raw") Netpbm bitmap image and converts to PWG or URF printer raster format.

...or a raw bitmap in the c++ api.

## pwg2ppm
For debugging. Similar to [rasterview](https://github.com/michaelrsweet/rasterview), but without a GUI. Takes a PWG or URF printer raster and outputs a series of P4, P5 or P6 pbm/pgm/ppm images.

## pdf2printable
Takes a PDF document and makes it suitable for printing, by:
- rotate and scale to fit as needed to a desired page size
- convert to PDF 1.5, Postscript 2 or PWG/URF raster

## baselinify
Takes a JPEG and losslessly repacks it to the baseline ecoding profile, keeping only JFIF and Exif headers.
Sort of like jpegtran without any arguments, but reusable in C++.

IPP-printers are only required to support baseline-encoded jpeg according to PWG5100.14.

Despite working with in-memory data, it only requires the libjpeg 62.2.0 and not 62.3.0/7.3+ API, so it works on conservative distros.

## Building

Install dependencies:

`sudo apt install libpoppler-dev libpoppler-glib-dev libcairo2-dev libglib2.0-dev libjpeg-dev`

Build:

`make`
