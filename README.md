# ppm2pwg - misc printing utilities
(should really be renamed)

[![C/C++ CI](https://github.com/attah/ppm2pwg/workflows/C%2FC%2B%2B%20CI/badge.svg)](https://github.com/attah/ppm2pwg/actions/workflows/ccpp.yml)
[![CodeQL](https://github.com/attah/ppm2pwg/workflows/CodeQL/badge.svg)](https://github.com/attah/ppm2pwg/actions/workflows/codeql-analysis.yml)
[![IRC-ppm2pwg@OFTC](https://img.shields.io/badge/IRC-%23ppm2pwg@OFTC-blue)](https://webchat.oftc.net/?channels=ppm2pwg)

Available as rudimentary standalone applications, but mainly made for use in [SeaPrint](https://github.com/attah/harbour-seaprint).

## ppm2pwg
Takes a pbm, pgm or ppm (P4, P5 or P6 "raw") Netpbm bitmap image and converts to PWG or URF printer raster format. Supports 1, 8 and **16** bits per color.

## pwg2ppm
For debugging. Similar to [rasterview](https://github.com/michaelrsweet/rasterview), but without a GUI. Takes a PWG or URF printer raster and outputs a series of P4, P5 or P6 pbm/pgm/ppm images.

## pdf2printable
Takes a PDF document and makes it suitable for printing, by:
- rotate and scale to fit as needed to a desired page size
- convert to PDF 1.7&sup1;, Postscript level 2 or PWG/URF raster

1. PDF 1.5 if using Cairo < 1.17.6.

## baselinify
Takes a JPEG and losslessly repacks it to the baseline ecoding profile, keeping only JFIF and Exif headers.
Sort of like jpegtran without any arguments.

IPP-printers are only required to support baseline-encoded jpeg according to PWG5100.14.

Despite working with in-memory data, it only requires the libjpeg 62.2.0 and not 62.3.0/7.3+ API, so it works on conservative distros.

## ippclient
An IPP client that harnesses the above tools for converting files to be printed.
This is a port/rewrite/clean-up of the core parts of SeaPrint in regular (non-Qt) C++.
The plan is to swap over to using this once fature parity is achieved.

## ippdiscover

A DNS-SD/mDNS "simple resolver" which looks for ipp and ipps printers on the local network.

## Building

Install dependencies:

`sudo apt install libpoppler-dev libpoppler-glib-dev libcairo2-dev libglib2.0-dev libjpeg-dev libcurl4-openssl-dev zlib1g-dev`

These are the aggregate dependencies; **ippclient** needs all of them and e.g. **ppm2pwg** needs none of them.

Build:

`make -j$(nproc)`

## pdf2printable vs the competition

(As of 2025-01-04)

A bit of friendly comparison helps make sure the featureset is well-rounded and performance is on par.

### Basics
|                                                         | PDF renderer          | Language    | License            |
| ------------------------------------------------------- | --------------------- | ----------- | ------------------ |
| pdf2printable                                           | Poppler&sup1;         | C++         | GPL3               |
| [ipptransform](https://github.com/OpenPrinting/libcups) | XPDF or Poppler&sup1; | C           | Apache 2.0         |
| [mutool](https://mupdf.com/)                            | MuPDF                 | C           | AGPL or commercial |
| [jrender](https://github.com/HPInc/jipp)                | Apache PDFBox         | Java/Kotlin | MIT                |

Not in the running: cups-filters (can't get them to run outside CUPS), Android/Apple built-ins and Google Cloud Print (not available standalone).

1. pdf2printable uses the Cairo backend in Poppler and ipptransform uses the Splash backend inhertited from XPDF. Cairo generally produces higher quality rasterization, but is slightly more prone to misrendering with unusual PDFs.

### Format support

|               | PDF | Postscript | PWG | URF | PCLm&sup1; | PCL&sup2; |
| ------------- | --- | ---------- | --- | --- | ---------- | --------- |
| pdf2printable | ✔   | ✔          | ✔   | ✔   | ✘          | ✘         |
| ipptransform  | ✔   | ✔          | ✔   | ✔   | ✘          | ✔&sup3;   |
| mutool        | ✔   | ✘          | ✔   | ✘   | ✔          | ✔         |
| jrender       | ✘   | ✘          | ✔   | ✘   | ✔          | ✘         |

Good printers should support PDF or PWG. After that, URF is the biggest enabler.

1. I have not yet seen a printer support PCLm and none of the other formats that pdf2printable supports.
2. PCL comes in many different dialects (even beyond the versions) so it might not work across all printers.
3. Pre-rasterized compatibility version.

### Features

|               | back-xform&sup1; | color modes &sup2;| rotate-to-fit | page selection | stdout    |
| ------------- | ---------------- | ----------------- | ------------- | -------------- | --------- |
| pdf2printable | ✔                | ✔(6)              | ✔             | ✔              | ✔(+stdin) |
| ipptransform  | ✔                | ✔(5)              | ✔             | ✔              | ✔         |
| mutool        | ✘                | ✔(3?)             | ✘             | ✔              | ✘         |
| jrender       | ✘                | ✘(1)&sup3;        | ✘             | ✘              | ✘         |

1. PWG, URF and PCLm printers may require the client to help transform backside pages for duplex printing, or they will come out incorrectly.
2. Two color modes (sRGB24 and sGray8) is enough for basically anything.
3. Jrender can be patched to do gray or bi-level, but the output is broken.

### Performance
Measured with a representative 90-page document for PWG-raster at 600 DPI on a AMD 3950X.

|                        | Speed (RGB) | Speed (Gray) | Size (RGB)   | Size (Gray) |
| ---------------------- | ----------- | -------------| ------------ | ----------- |
| pdf2printable          | 500 PPM     | 529 PPM      | 152 MB       | 76 MB       |
| ipptransform           | 204 PPM     | 194 PPM      | 159 MB       | 76 MB       |
| mutool (AA off)        | 260 PPM     | 184 PPM      | 152 MB       | 76 MB       |
| jrender (600dpi patch) | 171 PPM     | N/A          | 334 MB&sup1; | N/A         |

1. Antialiasing seems to be enabled and would account for the size difference. However, at these resolutions that doesn't really provide much benefit. For pdf2printable and mutool it can be optionally enabled/disabled.
