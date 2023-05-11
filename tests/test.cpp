#include "bytestream.h"
#include "test.h"
#include "subprocess.hpp"
#include "PwgPgHdr.h"
#include "pwg2ppm.h"
#include "printparameters.h"
#include "argget.h"
#include <cstring>
using namespace std;

#define REPEAT(N) (uint8_t)(N-1)
#define VERBATIM(N) (uint8_t)(257-N)

Bytestream W {(uint8_t)255, (uint8_t)255, (uint8_t)255};
Bytestream R {(uint8_t)255, (uint8_t)0,   (uint8_t)0};
Bytestream G {(uint8_t)0,   (uint8_t)255, (uint8_t)0};
Bytestream B {(uint8_t)0,   (uint8_t)0,   (uint8_t)255};
Bytestream Y {(uint8_t)255, (uint8_t)255, (uint8_t)0};

Bytestream PacmanPpm()
{
  Bytestream ppm {string("P6\n8 8\n255\n")};
  ppm << W << Y << Y << Y << W << W << W << W
      << Y << B << Y << W << W << W << G << W
      << Y << Y << W << W << W << G << G << G
      << Y << Y << Y << W << W << W << G << W
      << W << Y << Y << Y << W << W << W << W
      << W << W << W << W << W << W << W << W
      << R << R << R << R << R << R << R << R
      << R << R << R << R << R << R << R << R;
  return ppm;
}

Bytestream RightSideUp()
{
  Bytestream enc;
  enc << (uint8_t)0 << REPEAT(1) << W << REPEAT(3) << Y << REPEAT(4) << W
      << (uint8_t)0 << VERBATIM(3) << Y << B << Y << REPEAT(3) << W << VERBATIM(2) << G << W
      << (uint8_t)0 << REPEAT(2) << Y << REPEAT(3) << W << REPEAT(3) << G
      << (uint8_t)0 << REPEAT(3) << Y << REPEAT(3) << W << VERBATIM(2) << G << W
      << (uint8_t)0 << REPEAT(1) << W << REPEAT(3) << Y << REPEAT(4) << W
      << (uint8_t)0 << REPEAT(8) << W
      << (uint8_t)1 << REPEAT(8) << R;
      return enc;
}

Bytestream UpsideDown()
{
  Bytestream enc;
  enc << (uint8_t)1 << REPEAT(8) << R
      << (uint8_t)0 << REPEAT(8) << W
      << (uint8_t)0 << REPEAT(1) << W << REPEAT(3) << Y << REPEAT(4) << W
      << (uint8_t)0 << REPEAT(3) << Y << REPEAT(3) << W << VERBATIM(2) << G << W
      << (uint8_t)0 << REPEAT(2) << Y << REPEAT(3) << W << REPEAT(3) << G
      << (uint8_t)0 << VERBATIM(3) << Y << B << Y << REPEAT(3) << W << VERBATIM(2) << G << W
      << (uint8_t)0 << REPEAT(1) << W << REPEAT(3) << Y << REPEAT(4) << W;
      return enc;
}

Bytestream Flipped()
{
  Bytestream enc;
  enc << (uint8_t)0 << REPEAT(4) << W << REPEAT(3) << Y << REPEAT(1) << W
      << (uint8_t)0 << VERBATIM(2) << W << G << REPEAT(3) << W << VERBATIM(3) << Y << B << Y
      << (uint8_t)0 << REPEAT(3) << G << REPEAT(3) << W << REPEAT(2) << Y
      << (uint8_t)0 << VERBATIM(2) << W << G << REPEAT(3) << W << REPEAT(3) << Y
      << (uint8_t)0 << REPEAT(4) << W << REPEAT(3) << Y << REPEAT(1) << W
      << (uint8_t)0 << REPEAT(8) << W
      << (uint8_t)1 << REPEAT(8) << R;
      return enc;
}

Bytestream Rotated()
{
  Bytestream enc;
  enc << (uint8_t)1 << REPEAT(8) << R
      << (uint8_t)0 << REPEAT(8) << W
      << (uint8_t)0 << REPEAT(4) << W << REPEAT(3) << Y << REPEAT(1) << W
      << (uint8_t)0 << VERBATIM(2) << W << G << REPEAT(3) << W << REPEAT(3) << Y
      << (uint8_t)0 << REPEAT(3) << G << REPEAT(3) << W << REPEAT(2) << Y
      << (uint8_t)0 << VERBATIM(2) << W << G << REPEAT(3) << W << VERBATIM(3) << Y << B << Y
      << (uint8_t)0 << REPEAT(4) << W << REPEAT(3) << Y << REPEAT(1) << W;
  return enc;
}

Bytestream P4_0101()
{
  Bytestream ppm {string("P4\n24 8\n")};
  ppm << (uint8_t)0x8f << (uint8_t)0x78 << (uint8_t)0xf7
      << (uint8_t)0x76 << (uint8_t)0x77 << (uint8_t)0x67
      << (uint8_t)0x77 << (uint8_t)0x77 << (uint8_t)0x77
      << (uint8_t)0x77 << (uint8_t)0x77 << (uint8_t)0x77
      << (uint8_t)0x77 << (uint8_t)0x77 << (uint8_t)0x77
      << (uint8_t)0x77 << (uint8_t)0x77 << (uint8_t)0x77
      << (uint8_t)0x8e << (uint8_t)0x38 << (uint8_t)0xe3
      << (uint8_t)0xff << (uint8_t)0xff << (uint8_t)0xff;
  return ppm;
}

Bytestream BilevelPwg0101()
{
  Bytestream enc;
  enc << (uint8_t)0 << VERBATIM(3) << (uint8_t)0x8f << (uint8_t)0x78 << (uint8_t)0xf7
      << (uint8_t)0 << VERBATIM(3) << (uint8_t)0x76 << (uint8_t)0x77 << (uint8_t)0x67
      << (uint8_t)3 << REPEAT(3) << (uint8_t)0x77
      << (uint8_t)0 << VERBATIM(3) << (uint8_t)0x8e << (uint8_t)0x38 << (uint8_t)0xe3
      << (uint8_t)0 << REPEAT(3) << (uint8_t)0xff;
  return enc;
}

Bytestream BilevelPwg0101_UpsideDown()
{
  Bytestream enc;
  enc << (uint8_t)0 << REPEAT(3) << (uint8_t)0xff
      << (uint8_t)0 << VERBATIM(3) << (uint8_t)0x8e << (uint8_t)0x38 << (uint8_t)0xe3
      << (uint8_t)3 << REPEAT(3) << (uint8_t)0x77
      << (uint8_t)0 << VERBATIM(3) << (uint8_t)0x76 << (uint8_t)0x77 << (uint8_t)0x67
      << (uint8_t)0 << VERBATIM(3) << (uint8_t)0x8f << (uint8_t)0x78 << (uint8_t)0xf7;
  return enc;
}

Bytestream BilevelPwg0101_Flipped()
{
  Bytestream enc;
  enc << (uint8_t)0 << VERBATIM(3) << (uint8_t)0xef << (uint8_t)0x1e << (uint8_t)0xf1
      << (uint8_t)0 << VERBATIM(3) << (uint8_t)0xe6 << (uint8_t)0xee << (uint8_t)0x6e
      << (uint8_t)3 << REPEAT(3) << (uint8_t)0xee
      << (uint8_t)0 << VERBATIM(3) << (uint8_t)0xc7 << (uint8_t)0x1c << (uint8_t)0x71
      << (uint8_t)0 << REPEAT(3) << (uint8_t)0xff;
  return enc;
}

Bytestream BilevelPwg0101_Rotated()
{
  Bytestream enc;
  enc << (uint8_t)0 << REPEAT(3) << (uint8_t)0xff
      << (uint8_t)0 << VERBATIM(3) << (uint8_t)0xc7 << (uint8_t)0x1c << (uint8_t)0x71
      << (uint8_t)3 << REPEAT(3) << (uint8_t)0xee
      << (uint8_t)0 << VERBATIM(3) << (uint8_t)0xe6 << (uint8_t)0xee << (uint8_t)0x6e
      << (uint8_t)0 << VERBATIM(3) << (uint8_t)0xef << (uint8_t)0x1e << (uint8_t)0xf1;
  return enc;
}

TEST(ppm2pwg)
{
  Bytestream ppm = PacmanPpm();

  // For sanity, make sure the one on disk is the same as created above
  std::ifstream ppm_ifs("pacman.ppm");
  Bytestream expected_ppm(ppm_ifs);
  ASSERT(ppm == expected_ppm);

  subprocess::popen ppm2pwg("../ppm2pwg", {"pacman.ppm", "-"});
  ppm2pwg.close();

  ASSERT(ppm2pwg.wait() == 0);

  Bytestream pwg(ppm2pwg.stdout());
  PwgPgHdr hdr;

  ASSERT(pwg >>= "RaS2");
  hdr.decodeFrom(pwg);

  Bytestream enc = RightSideUp();

  ASSERT(pwg >>= enc);
  ASSERT(pwg.atEnd());

  pwg.setPos(0);

  std::ifstream ifs("pacman.pwg");
  Bytestream expected_pwg(ifs);

  ASSERT(pwg == expected_pwg);
}

TEST(duplex_normal)
{
  Bytestream twoSided;
  twoSided << PacmanPpm() << PacmanPpm();

  setenv("DUPLEX", "true", true);
  subprocess::popen ppm2pwg("../ppm2pwg", {"-", "-"});
  ppm2pwg.stdin() << twoSided;
  ppm2pwg.close();

  ASSERT(ppm2pwg.wait() == 0);

  Bytestream pwg(ppm2pwg.stdout());
  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decodeFrom(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  ASSERT(pwg >>= RightSideUp());
  hdr2.decodeFrom(pwg);
  ASSERT(hdr2.CrossFeedTransform == 1);
  ASSERT(hdr2.FeedTransform == 1);
  ASSERT(pwg >>= RightSideUp());
  ASSERT(pwg.atEnd());
}

TEST(duplex_vflip)
{
  Bytestream twoSided;
  twoSided << PacmanPpm() << PacmanPpm();

  subprocess::popen ppm2pwg("../ppm2pwg", {"-d", "--vflip", "-", "-"});
  ppm2pwg.stdin() << twoSided;
  ppm2pwg.close();

  ASSERT(ppm2pwg.wait() == 0);

  Bytestream pwg(ppm2pwg.stdout());
  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decodeFrom(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  ASSERT(pwg >>= RightSideUp());
  hdr2.decodeFrom(pwg);
  ASSERT(hdr2.CrossFeedTransform == 1);
  ASSERT(hdr2.FeedTransform == -1);
  ASSERT(pwg >>= UpsideDown());
  ASSERT(pwg.atEnd());
}

TEST(duplex_hflip)
{
  Bytestream twoSided;
  twoSided << PacmanPpm() << PacmanPpm();

  subprocess::popen ppm2pwg("../ppm2pwg", {"-d", "--hflip", "-", "-"});
  ppm2pwg.stdin() << twoSided;
  ppm2pwg.close();

  ASSERT(ppm2pwg.wait() == 0);

  Bytestream pwg(ppm2pwg.stdout());
  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decodeFrom(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  ASSERT(pwg >>= RightSideUp());
  hdr2.decodeFrom(pwg);
  ASSERT(hdr2.CrossFeedTransform == -1);
  ASSERT(hdr2.FeedTransform == 1);
  ASSERT(pwg >>= Flipped());
  ASSERT(pwg.atEnd());
}

TEST(duplex_rotated)
{
  Bytestream twoSided;
  twoSided << PacmanPpm() << PacmanPpm();

  subprocess::popen ppm2pwg("../ppm2pwg", {"-d", "--hflip", "--vflip", "-", "-"});
  ppm2pwg.stdin() << twoSided;
  ppm2pwg.close();

  ASSERT(ppm2pwg.wait() == 0);

  Bytestream pwg(ppm2pwg.stdout());
  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decodeFrom(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  ASSERT(pwg >>= RightSideUp());
  hdr2.decodeFrom(pwg);
  ASSERT(hdr2.CrossFeedTransform == -1);
  ASSERT(hdr2.FeedTransform == -1);
  ASSERT(pwg >>= Rotated());
  ASSERT(pwg.atEnd());
}

TEST(two_pages_no_duplex)
{
  Bytestream twoSided;
  twoSided << PacmanPpm() << PacmanPpm();

  subprocess::popen ppm2pwg("../ppm2pwg", {"--hflip", "--vflip", "-", "-"});
  ppm2pwg.stdin() << twoSided;
  ppm2pwg.close();

  ASSERT(ppm2pwg.wait() == 0);

  Bytestream pwg(ppm2pwg.stdout());
  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decodeFrom(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  ASSERT(pwg >>= RightSideUp());
  hdr2.decodeFrom(pwg);
  ASSERT(hdr2.CrossFeedTransform == 1);
  ASSERT(hdr2.FeedTransform == 1);
  ASSERT(pwg >>= RightSideUp());
  ASSERT(pwg.atEnd());
}

TEST(bilevel)
{
  Bytestream P4 = P4_0101();

  subprocess::popen ppm2pwg("../ppm2pwg", {"-", "-"});
  ppm2pwg.stdin() << P4;
  ppm2pwg.close();

  ASSERT(ppm2pwg.wait() == 0);

  Bytestream pwg(ppm2pwg.stdout());

  PwgPgHdr hdr1;
  ASSERT(pwg >>= "RaS2");
  hdr1.decodeFrom(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  ASSERT(pwg >>= BilevelPwg0101());
}

TEST(bilevel_vflip)
{
  Bytestream twoSided;
  twoSided << P4_0101() << P4_0101();

  subprocess::popen ppm2pwg("../ppm2pwg", {"-d", "--vflip", "-", "-"});
  ppm2pwg.stdin() << twoSided;
  ppm2pwg.close();

  ASSERT(ppm2pwg.wait() == 0);

  Bytestream pwg(ppm2pwg.stdout());
  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decodeFrom(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  ASSERT(pwg >>= BilevelPwg0101());
  hdr2.decodeFrom(pwg);
  ASSERT(hdr2.CrossFeedTransform == 1);
  ASSERT(hdr2.FeedTransform == -1);
  ASSERT(pwg >>= BilevelPwg0101_UpsideDown());
  ASSERT(pwg.atEnd());
}

TEST(bilevel_hflip)
{
  Bytestream twoSided;
  twoSided << P4_0101() << P4_0101();

  subprocess::popen ppm2pwg("../ppm2pwg", {"-d", "--hflip", "-", "-"});
  ppm2pwg.stdin() << twoSided;
  ppm2pwg.close();

  ASSERT(ppm2pwg.wait() == 0);

  Bytestream pwg(ppm2pwg.stdout());
  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decodeFrom(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  ASSERT(pwg >>= BilevelPwg0101());
  hdr2.decodeFrom(pwg);
  ASSERT(hdr2.CrossFeedTransform == -1);
  ASSERT(hdr2.FeedTransform == 1);

  ASSERT(pwg >>= BilevelPwg0101_Flipped());
  ASSERT(pwg.atEnd());
}

TEST(bilevel_rotated)
{
  Bytestream twoSided;
  twoSided << P4_0101() << P4_0101();

  subprocess::popen ppm2pwg("../ppm2pwg", {"-d", "--hflip", "--vflip", "-", "-"});
  ppm2pwg.stdin() << twoSided;
  ppm2pwg.close();

  ASSERT(ppm2pwg.wait() == 0);

  Bytestream pwg(ppm2pwg.stdout());
  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decodeFrom(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  ASSERT(pwg >>= BilevelPwg0101());
  hdr2.decodeFrom(pwg);
  ASSERT(hdr2.CrossFeedTransform == -1);
  ASSERT(hdr2.FeedTransform == -1);

  ASSERT(pwg >>= BilevelPwg0101_Rotated());
  ASSERT(pwg.atEnd());
}

bool close_enough(size_t a, size_t b, size_t precision)
{
  size_t lower = b == 0 ? 0 : b-precision;
  size_t upper = b == 0 ? 0 : b+precision;
  return (a <= upper) && (a >= lower);
}

void do_test_centering(const char* test_name, std::string filename, bool asymmetric)
{
  subprocess::popen pdf2printable("../pdf2printable",
                                  {"--format", "pwg",
                                   "-rx", "300",
                                   "-ry", asymmetric ? "600" : "300",
                                   filename, test_name});
  pdf2printable.close();
  ASSERT(pdf2printable.wait() == 0);

  std::ifstream ifs(test_name, std::ios::in | std::ios::binary);
  Bytestream pwg(ifs);

  ASSERT(pwg.size() != 0);

  ASSERT(pwg >>= "RaS2");
  PwgPgHdr PwgHdr;
  PwgHdr.decodeFrom(pwg);

  size_t width = PwgHdr.Width;
  size_t height = PwgHdr.Height;
  size_t colors = PwgHdr.NumColors;

  ASSERT(close_enough(width, 2480, 1));
  ASSERT(close_enough(height, asymmetric ? 7015 : 3507, 1));

  Bytestream bmp;
  raster_to_bmp(bmp, pwg, width*3, height, colors, false);

  size_t left_margin, right_margin, top_margin, bottom_margin;

  left_margin = 0;
  right_margin = 0;
  top_margin = 0;
  bottom_margin = 0;

  Bytestream white_line(0xff, width*colors);

  while(bmp.nextBytestream(white_line))
  {
    top_margin++;
  }
  while(bmp.nextBytestream(white_line, false))
  {
  }
  while(bmp.nextBytestream(white_line))
  {
    bottom_margin++;
  }

  Bytestream middle_line;
  bmp.setPos((height/2)*(width*colors));
  bmp/(width*colors) >> middle_line;

  Bytestream white_pixel(0xff, colors);

  while(middle_line.nextBytestream(white_pixel))
  {
    left_margin++;
  }
  while(middle_line.nextBytestream(white_pixel, false))
  {
  }
  while(middle_line.nextBytestream(white_pixel))
  {
    right_margin++;
  }

  ASSERT(close_enough(left_margin, right_margin, 1));
  ASSERT(close_enough(top_margin, bottom_margin, 1));

}

// More rectangular than A4
TEST(pdf2printable_16x9_portrait)
{
  do_test_centering(__func__, "portrait_16x9.pdf", false);
}

TEST(pdf2printable_16x9_landscape)
{
  do_test_centering(__func__, "landscape_16x9.pdf", false);
}

TEST(pdf2printable_16x9_portrait_asymmetric)
{
  do_test_centering(__func__, "portrait_16x9.pdf", true);
}

TEST(pdf2printable_16x9_landscape_asymmetric)
{
  do_test_centering(__func__, "landscape_16x9.pdf", true);
}

// More square than A4
TEST(pdf2printable_4x3_portrait)
{
  do_test_centering(__func__, "portrait_4x3.pdf", false);
}

TEST(pdf2printable_4x3_landscape)
{
  do_test_centering(__func__, "landscape_4x3.pdf", false);
}

TEST(pdf2printable_4x3_portrait_asymmetric)
{
  do_test_centering(__func__, "portrait_4x3.pdf", true);
}

TEST(pdf2printable_4x3_landscape_asymmetric)
{
  do_test_centering(__func__, "landscape_4x3.pdf", true);
}

TEST(printparameters)
{
  PrintParameters A4;
  A4.paperSizeUnits = PrintParameters::Millimeters;
  A4.paperSizeW = 210;
  A4.paperSizeH = 297;
  A4.hwResW = 300;
  A4.hwResH = 300;

  PrintParameters A4Px;
  A4Px.paperSizeUnits = PrintParameters::Pixels;
  A4Px.paperSizeW = (float)A4.getPaperSizeWInPixels();
  A4Px.paperSizeH = (float)A4.getPaperSizeHInPixels();
  A4Px.hwResW = 300;
  A4Px.hwResH = 300;

  ASSERT(A4.getPaperSizeWInPixels() == 2480);
  ASSERT(A4.getPaperSizeHInPixels() == 3508);

  ASSERT(A4.getPaperSizeWInBytes() == 2480*3);
  ASSERT(A4.getPaperSizeInPixels() == 2480*3508);
  ASSERT(A4.getPaperSizeInBytes() == 2480*3508*3);

  ASSERT(A4.getPaperSizeWInMillimeters() == 210);
  ASSERT(A4.getPaperSizeHInMillimeters() == 297);

  ASSERT(round(A4.getPaperSizeWInPoints()) == 595);
  ASSERT(round(A4.getPaperSizeHInPoints()) == 842);

  ASSERT(round(A4Px.getPaperSizeWInMillimeters()) == 210);
  ASSERT(round(A4Px.getPaperSizeHInMillimeters()) == 297);

  ASSERT(round(A4Px.getPaperSizeWInPoints()) == 595);
  ASSERT(round(A4Px.getPaperSizeHInPoints()) == 842);

  A4.hwResW = 600;
  A4.hwResH = 600;
  A4Px.paperSizeW = (float)A4.getPaperSizeWInPixels();
  A4Px.paperSizeH = (float)A4.getPaperSizeHInPixels();
  A4Px.hwResW = 600;
  A4Px.hwResH = 600;

  ASSERT(A4.getPaperSizeWInPixels() == 4961);
  ASSERT(A4.getPaperSizeHInPixels() == 7016);

  ASSERT(A4.getPaperSizeWInBytes() == 4961*3);
  ASSERT(A4.getPaperSizeInPixels() == 4961*7016);
  ASSERT(A4.getPaperSizeInBytes() == 4961*7016*3);

  ASSERT(A4.getPaperSizeWInMillimeters() == 210);
  ASSERT(A4.getPaperSizeHInMillimeters() == 297);

  ASSERT(round(A4.getPaperSizeWInPoints()) == 595);
  ASSERT(round(A4.getPaperSizeHInPoints()) == 842);

  ASSERT(round(A4Px.getPaperSizeWInMillimeters()) == 210);
  ASSERT(round(A4Px.getPaperSizeHInMillimeters()) == 297);

  ASSERT(round(A4Px.getPaperSizeWInPoints()) == 595);
  ASSERT(round(A4Px.getPaperSizeHInPoints()) == 842);

  PrintParameters Letter;
  Letter.paperSizeUnits = PrintParameters::Inches;
  Letter.paperSizeW = 8.5;
  Letter.paperSizeH = 11;
  Letter.hwResW = 300;
  Letter.hwResH = 300;

  PrintParameters LetterPx;
  LetterPx.paperSizeUnits = PrintParameters::Pixels;
  LetterPx.paperSizeW = (float)Letter.getPaperSizeWInPixels();
  LetterPx.paperSizeH = (float)Letter.getPaperSizeHInPixels();
  LetterPx.hwResW = 300;
  LetterPx.hwResH = 300;

  ASSERT(Letter.getPaperSizeWInPixels() == 2550);
  ASSERT(Letter.getPaperSizeHInPixels() == 3300);

  ASSERT(Letter.getPaperSizeWInBytes() == 2550*3);
  ASSERT(Letter.getPaperSizeInPixels() == 2550*3300);
  ASSERT(Letter.getPaperSizeInBytes() == 2550*3300*3);

  ASSERT(Letter.getPaperSizeWInMillimeters() == 215.9f);
  ASSERT(Letter.getPaperSizeHInMillimeters() == 279.4f);

  ASSERT(Letter.getPaperSizeWInPoints() == 612);
  ASSERT(Letter.getPaperSizeHInPoints() == 792);

  ASSERT(LetterPx.getPaperSizeWInMillimeters() == 215.9f);
  ASSERT(LetterPx.getPaperSizeHInMillimeters() == 279.4f);

  ASSERT(LetterPx.getPaperSizeWInPoints() == 612);
  ASSERT(LetterPx.getPaperSizeHInPoints() == 792);

  Letter.hwResW = 600;
  Letter.hwResH = 600;
  LetterPx.paperSizeW = (float)Letter.getPaperSizeWInPixels();
  LetterPx.paperSizeH = (float)Letter.getPaperSizeHInPixels();
  LetterPx.hwResW = 600;
  LetterPx.hwResH = 600;

  ASSERT(Letter.getPaperSizeWInPixels() == 5100);
  ASSERT(Letter.getPaperSizeHInPixels() == 6600);

  ASSERT(Letter.getPaperSizeWInBytes() == 5100*3);
  ASSERT(Letter.getPaperSizeInPixels() == 5100*6600);
  ASSERT(Letter.getPaperSizeInBytes() == 5100*6600*3);

  ASSERT(Letter.getPaperSizeWInMillimeters() == 215.9f);
  ASSERT(Letter.getPaperSizeHInMillimeters() == 279.4f);

  ASSERT(Letter.getPaperSizeWInPoints() == 612);
  ASSERT(Letter.getPaperSizeHInPoints() == 792);

  ASSERT(LetterPx.getPaperSizeWInMillimeters() == 215.9f);
  ASSERT(LetterPx.getPaperSizeHInMillimeters() == 279.4f);

  ASSERT(LetterPx.getPaperSizeWInPoints() == 612);
  ASSERT(LetterPx.getPaperSizeHInPoints() == 792);

  A4.hwResH = 1200;
  A4Px.hwResH = 1200;
  A4Px.paperSizeH = (float)A4.getPaperSizeHInPixels();

  Letter.hwResH = 1200;
  LetterPx.hwResH = 1200;
  LetterPx.paperSizeH = (float)Letter.getPaperSizeHInPixels();

  ASSERT(A4.getPaperSizeWInPixels() == 4961);
  ASSERT(A4.getPaperSizeHInPixels() == 14031);
  ASSERT(A4Px.getPaperSizeWInPixels() == 4961);
  ASSERT(A4Px.getPaperSizeHInPixels() == 14031);

  ASSERT(A4.getPaperSizeWInBytes() == 4961*3);
  ASSERT(A4.getPaperSizeInPixels() == 4961*14031);
  ASSERT(A4.getPaperSizeInBytes() == 4961*14031*3);

  ASSERT(A4Px.getPaperSizeWInBytes() == 4961*3);
  ASSERT(A4Px.getPaperSizeInPixels() == 4961*14031);
  ASSERT(A4Px.getPaperSizeInBytes() == 4961*14031*3);

  // Asymmetric resolution does not affect dimension in mm
  ASSERT(A4.getPaperSizeWInMillimeters() == 210);
  ASSERT(A4.getPaperSizeHInMillimeters() == 297);
  ASSERT(round(A4Px.getPaperSizeWInMillimeters()) == 210);
  ASSERT(round(A4Px.getPaperSizeHInMillimeters()) == 297);

  // Asymmetric resolution does not affect dimension in points
  ASSERT(round(A4.getPaperSizeWInPoints()) == 595);
  ASSERT(round(A4.getPaperSizeHInPoints()) == 842);
  ASSERT(round(A4Px.getPaperSizeWInPoints()) == 595);
  ASSERT(round(A4Px.getPaperSizeHInPoints()) == 842);

  ASSERT(Letter.getPaperSizeWInPixels() == 5100);
  ASSERT(Letter.getPaperSizeHInPixels() == 13200);
  ASSERT(LetterPx.getPaperSizeWInPixels() == 5100);
  ASSERT(LetterPx.getPaperSizeHInPixels() == 13200);

  // Asymmetric resolution does not affect dimension in mm
  ASSERT(Letter.getPaperSizeWInMillimeters() == 215.9f);
  ASSERT(Letter.getPaperSizeHInMillimeters() == 279.4f);
  ASSERT(LetterPx.getPaperSizeWInMillimeters() == 215.9f);
  ASSERT(LetterPx.getPaperSizeHInMillimeters() == 279.4f);

  // Asymmetric resolution does not affect dimension in points
  ASSERT(LetterPx.getPaperSizeWInPoints() == 612);
  ASSERT(LetterPx.getPaperSizeHInPoints() == 792);
  ASSERT(LetterPx.getPaperSizeWInPoints() == 612);
  ASSERT(LetterPx.getPaperSizeHInPoints() == 792);

}

TEST(pagerange)
{
  PrintParameters params;
  PageSequence seq;
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 100);

  params.pageRangeList = {{4, 7}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 4);
  ASSERT(seq == PageSequence({4, 5, 6, 7}));

  params.documentCopies = 2;
  params.format = PrintParameters::PWG;
  params.pageRangeList = {{4, 7}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 8);
  ASSERT(seq == PageSequence({4, 5, 6, 7,
                              4, 5, 6, 7}));

  params.duplex = false;
  params.documentCopies = 2;
  params.format = PrintParameters::PWG;
  params.pageRangeList = {{4, 6}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 6);
  ASSERT(seq == PageSequence({4, 5, 6,
                              4, 5, 6}));

  params.duplex = false;
  params.documentCopies = 3;
  params.pageRangeList = {{4, 6}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 9);
  ASSERT(seq == PageSequence({4, 5, 6,
                              4, 5, 6,
                              4, 5, 6}));

  params.duplex = true;
  params.documentCopies = 2;
  params.format = PrintParameters::PWG;
  params.pageRangeList = {{4, 6}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 8);
  ASSERT(seq == PageSequence({4, 5, 6, INVALID_PAGE,
                              4, 5, 6, INVALID_PAGE}));

  params.duplex = true;
  params.documentCopies = 3;
  params.pageRangeList = {{4, 6}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 12);
  ASSERT(seq == PageSequence({4, 5, 6, INVALID_PAGE,
                              4, 5, 6, INVALID_PAGE,
                              4, 5, 6, INVALID_PAGE}));

  params.duplex = false;
  params.documentCopies = 1;
  params.pageCopies = 2;
  params.pageRangeList = {{4, 7}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 8);
  ASSERT(seq == PageSequence({4, 4,
                              5, 5,
                              6, 6,
                              7, 7}));

  params.duplex = true;
  params.documentCopies = 1;
  params.pageCopies = 2;
  params.pageRangeList = {{4, 7}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 8);
  ASSERT(seq == PageSequence({4, 5,
                              4, 5,
                              6, 7,
                              6, 7}));

  params.duplex = false;
  params.documentCopies = 3;
  params.pageCopies = 1;
  params.pageRangeList = {{1, 2}, {5, 7}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 15);
  ASSERT(seq == PageSequence({1, 2, 5, 6, 7,
                              1, 2, 5, 6, 7,
                              1, 2, 5, 6, 7}));

  params.duplex = true;
  params.documentCopies = 3;
  params.pageCopies = 1;
  params.pageRangeList = {{1, 2}, {5, 7}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 18);
  ASSERT(seq == PageSequence({1, 2, 5, 6, 7, INVALID_PAGE,
                              1, 2, 5, 6, 7, INVALID_PAGE,
                              1, 2, 5, 6, 7, INVALID_PAGE}));

  params.duplex = true;
  params.documentCopies = 3;
  params.pageCopies = 1;
  params.pageRangeList = {{1, 2}, {6, 7}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 12);
  ASSERT(seq == PageSequence({1, 2, 6, 7,
                              1, 2, 6, 7,
                              1, 2, 6, 7}));

  params.duplex = false;
  params.documentCopies = 1;
  params.pageCopies = 2;
  params.pageRangeList = {{1, 2}, {5, 7}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 10);
  ASSERT(seq == PageSequence({1, 1,
                              2, 2,
                              5, 5,
                              6, 6,
                              7, 7}));

  params.duplex = true;
  params.documentCopies = 1;
  params.pageCopies = 2;
  params.pageRangeList = {{1, 2}, {5, 7}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 12);
  ASSERT(seq == PageSequence({1, 2,
                              1, 2,
                              5, 6,
                              5, 6,
                              7, INVALID_PAGE,
                              7, INVALID_PAGE}));

  params.duplex = false;
  params.documentCopies = 1;
  params.pageCopies = 1;
  params.pageRangeList = {};
  seq = params.getPageSequence(3);
  ASSERT(seq.size() == 3);
  ASSERT(seq == PageSequence({1, 2, 3}));

  params.duplex = false;
  params.documentCopies = 2;
  params.pageCopies = 1;
  params.pageRangeList = {};
  seq = params.getPageSequence(3);
  ASSERT(seq.size() == 6);
  ASSERT(seq == PageSequence({1, 2, 3,
                              1, 2, 3}));

  params.duplex = false;
  params.documentCopies = 1;
  params.pageCopies = 2;
  params.pageRangeList = {};
  seq = params.getPageSequence(3);
  ASSERT(seq.size() == 6);
  ASSERT(seq == PageSequence({1, 1,
                              2, 2,
                              3, 3}));

  params.duplex = true;
  params.documentCopies = 2;
  params.pageCopies = 1;
  params.pageRangeList = {};
  seq = params.getPageSequence(3);
  ASSERT(seq.size() == 8);
  ASSERT(seq == PageSequence({1, 2, 3, INVALID_PAGE,
                              1, 2, 3, INVALID_PAGE}));

  params.duplex = true;
  params.documentCopies = 1;
  params.pageCopies = 2;
  params.pageRangeList = {};
  seq = params.getPageSequence(3);
  ASSERT(seq.size() == 8);
  ASSERT(seq == PageSequence({1, 2,
                              1, 2,
                              3, INVALID_PAGE,
                              3, INVALID_PAGE}));

  params.duplex = false;
  params.documentCopies = 1;
  params.pageCopies = 1;
  params.pageRangeList = {{17,42}};
  seq = params.getPageSequence(42);
  ASSERT(seq.size() == 26);
  seq = params.getPageSequence(32);
  ASSERT(seq.size() == 16);

  params.pageRangeList = {{1,2},{17,42}};
  seq = params.getPageSequence(42);
  ASSERT(seq.size() == 28);
  seq = params.getPageSequence(32);
  ASSERT(seq.size() == 18);

}

TEST(parse_pagerange)
{
  PrintParameters params;
  ASSERT(params.setPageRange("1"));
  ASSERT(params.pageRangeList == PageRangeList({{1,1}}));
  ASSERT(params.setPageRange("1-1"));
  ASSERT(params.pageRangeList == PageRangeList({{1,1}}));
  ASSERT(params.setPageRange("1,2,3"));
  ASSERT(params.pageRangeList == PageRangeList({{1,1},{2,2},{3,3}}));
  ASSERT(params.setPageRange("1-3,5"));
  ASSERT(params.pageRangeList == PageRangeList({{1,3},{5,5}}));
  ASSERT(params.setPageRange("1-3,5,6,17-42"));
  ASSERT(params.pageRangeList == PageRangeList({{1,3},{5,5},{6,6},{17,42}}));

  params.pageRangeList = {};
  ASSERT_FALSE(params.setPageRange("1,"));
  ASSERT(params.pageRangeList.empty());
  ASSERT_FALSE(params.setPageRange(""));
  ASSERT(params.pageRangeList.empty());
  ASSERT_FALSE(params.setPageRange(","));
  ASSERT(params.pageRangeList.empty());
  ASSERT_FALSE(params.setPageRange("-,-"));
  ASSERT(params.pageRangeList.empty());
  ASSERT_FALSE(params.setPageRange("fail"));
  ASSERT(params.pageRangeList.empty());
  ASSERT_FALSE(params.setPageRange("fail1"));
  ASSERT(params.pageRangeList.empty());
  ASSERT_FALSE(params.setPageRange("fail,1-2"));
  ASSERT(params.pageRangeList.empty());
  ASSERT_FALSE(params.setPageRange("1-2,fail"));
  ASSERT(params.pageRangeList.empty());
  ASSERT_FALSE(params.setPageRange("1-0"));
  ASSERT(params.pageRangeList.empty());
  ASSERT_FALSE(params.setPageRange("1,2-"));
  ASSERT(params.pageRangeList.empty());
}

TEST(parse_page_size)
{
  PrintParameters params;
  ASSERT(params.setPaperSize("iso_a4_210x297mm"));
  ASSERT(params.paperSizeW == 210);
  ASSERT(params.paperSizeH == 297);
  ASSERT(params.paperSizeUnits == PrintParameters::Millimeters);

  ASSERT(params.setPaperSize("na_letter_8.5x11in"));
  ASSERT(params.paperSizeName == "na_letter_8.5x11in");
  ASSERT(params.paperSizeW == 8.5f);
  ASSERT(params.paperSizeH == 11);
  ASSERT(params.paperSizeUnits == PrintParameters::Inches);

  ASSERT(params.setPaperSize("jpn_chou2_111.1x146mm"));
  ASSERT(params.paperSizeName == "jpn_chou2_111.1x146mm");
  ASSERT(params.paperSizeW == 111.1f);
  ASSERT(params.paperSizeH == 146);
  ASSERT(params.paperSizeUnits == PrintParameters::Millimeters);

  ASSERT_FALSE(params.setPaperSize(""));
  ASSERT_FALSE(params.setPaperSize("_1x1mm"));
  ASSERT(params.paperSizeName == "jpn_chou2_111.1x146mm");
  ASSERT(params.setPaperSize("a_1x1mm"));
  ASSERT(params.paperSizeName == "a_1x1mm");
  ASSERT(params.paperSizeW == 1);
  ASSERT(params.paperSizeH == 1);
  ASSERT(params.paperSizeUnits == PrintParameters::Millimeters);
  ASSERT_FALSE(params.setPaperSize("a_1x1km"));
  ASSERT_FALSE(params.setPaperSize("a_11mm"));
  ASSERT_FALSE(params.setPaperSize("a_a1x1mm"));
  ASSERT_FALSE(params.setPaperSize("1.1x1.2mm"));
  ASSERT(params.paperSizeName == "a_1x1mm");
  ASSERT(params.paperSizeW == 1);
  ASSERT(params.paperSizeH == 1);
  ASSERT(params.paperSizeUnits == PrintParameters::Millimeters);
}

TEST(silly_locale_parse_page_size)
{
  const std::string locale = std::setlocale(LC_ALL, nullptr);
  std::setlocale(LC_ALL, "sv_SE.utf8");

  PrintParameters params;

  ASSERT(params.setPaperSize("fuuuuu_13.37x42.69mm"));
  ASSERT(params.paperSizeName == "fuuuuu_13.37x42.69mm");
  ASSERT(params.paperSizeW == 13.37f);
  ASSERT(params.paperSizeH == 42.69f);
  ASSERT(params.paperSizeUnits == PrintParameters::Millimeters);

  std::setlocale(LC_ALL, locale.c_str());

}

TEST(color_mode)
{
  PrintParameters params;
  params.colorMode = PrintParameters::sRGB24;
  ASSERT(params.getNumberOfColors() == 3);
  ASSERT(params.getBitsPerColor() == 8);
  ASSERT(params.isBlack() == false);
  params = PrintParameters();
  params.colorMode = PrintParameters::CMYK32;
  ASSERT(params.getNumberOfColors() == 4);
  ASSERT(params.getBitsPerColor() == 8);
  ASSERT(params.isBlack() == false);
  params = PrintParameters();
  params.colorMode = PrintParameters::Gray8;
  ASSERT(params.getNumberOfColors() == 1);
  ASSERT(params.getBitsPerColor() == 8);
  ASSERT(params.isBlack() == false);
  params = PrintParameters();
  params.colorMode = PrintParameters::Black8;
  ASSERT(params.getNumberOfColors() == 1);
  ASSERT(params.getBitsPerColor() == 8);
  ASSERT(params.isBlack() == true);
  params = PrintParameters();
  params.colorMode = PrintParameters::Gray1;
  ASSERT(params.getNumberOfColors() == 1);
  ASSERT(params.getBitsPerColor() == 1);
  ASSERT(params.isBlack() == false);
  params = PrintParameters();
  params.colorMode = PrintParameters::Black1;
  ASSERT(params.getNumberOfColors() == 1);
  ASSERT(params.getBitsPerColor() == 1);
  ASSERT(params.isBlack() == true);
}

TEST(argget)
{
  bool b = false;
  SwitchArg<bool> boolopt(b, {"-b", "--bool"}, "A bool option");
  ASSERT_FALSE(boolopt.isSet());
  std::list<std::string> list1 {"-b"};
  ASSERT(boolopt.parse(list1));
  ASSERT(boolopt.isSet());
  ASSERT(b == true);

  std::string s;
  SwitchArg<string> stringopt(s, {"-s", "--string"}, "A string option");
  std::list<std::string> list2 {"-s", "mystring"};
  ASSERT(stringopt.parse(list2));
  ASSERT(stringopt.isSet());
  ASSERT(s == "mystring");

  int i = 0;
  SwitchArg<int> intopt(i, {"-i", "--integer"}, "An integer option");
  std::list<std::string> list3 {"-i", "123"};
  ASSERT(intopt.parse(list3));
  ASSERT(intopt.isSet());
  ASSERT(i == 123);

  enum MyEnum
  {
    apa,
    bpa,
    cpa
  };

  std::map<std::string, MyEnum> enumMap = {{"apa", apa}, {"bpa", bpa}, {"cpa", cpa}};
  MyEnum e = apa;

  EnumSwitchArg<MyEnum> enumopt(e, enumMap, {"-e", "--enum"}, "An enum option");
  std::list<std::string> list4 {"-e", "bpa"};
  ASSERT(enumopt.parse(list4));
  ASSERT(enumopt.isSet());
  ASSERT(e == bpa);

  b = false;

  ArgGet get({&boolopt, &stringopt, &intopt, &enumopt});
  char* argv[8] = {(char*)"myprog",
                   (char*)"-b",
                   (char*)"-s", (char*)"something",
                   (char*)"-i", (char*)"456",
                   (char*)"-e", (char*)"cpa"};
  ASSERT(get.get_args(8, argv));
  ASSERT(b == true);
  ASSERT(s == "something");
  ASSERT(i == 456);
  ASSERT(e == cpa);

  char* argv1[3] = {(char*)"myprog",
                    (char*)"-i", (char*)"NaN"};
  ASSERT_FALSE(get.get_args(3, argv1));
  ASSERT(get.errmsg().find("Bad value") != std::string::npos);
  ASSERT(get.errmsg().find("-i, --integer") != std::string::npos);
  ASSERT(get.errmsg().find("NaN") != std::string::npos);

  char* argv2[3] = {(char*)"myprog",
                    (char*)"-e", (char*)"dpa"};
  ASSERT_FALSE(get.get_args(3, argv2));
  ASSERT(get.errmsg().find("Bad value") != std::string::npos);
  ASSERT(get.errmsg().find("-e, --enum") != std::string::npos);
  ASSERT(get.errmsg().find("dpa") != std::string::npos);

  EnumSwitchArg<MyEnum> enumopt2(e, enumMap, {"-e", "--enum"}, "An enum option", "Does not map to enum");
  ArgGet get1({&boolopt, &stringopt, &intopt, &enumopt2});

  ASSERT_FALSE(get1.get_args(3, argv2));
  ASSERT(get1.errmsg().find("Does not map to enum") != std::string::npos);
  ASSERT(get1.errmsg().find("dpa") != std::string::npos);

  char* argv3[5] = {(char*)"myprog",
                    (char*)"-i", (char*)"666",
                    (char*)"-n", (char*)"777"};
  ASSERT_FALSE(get.get_args(5, argv3));

  std::string a1;
  std::string a2;
  PosArg arg1(a1, "arg1");
  PosArg arg2(a2, "arg2", true);

  ArgGet get2({&boolopt, &stringopt, &intopt}, {&arg1, &arg2});

  b = false;

  char* argv4[8] = {(char*)"myprog",
                    (char*)"-b",
                    (char*)"-s", (char*)"something_else",
                    (char*)"-i", (char*)"789",
                    (char*)"posarg1", (char*)"posarg2"};

  ASSERT(get2.get_args(8, argv4));
  ASSERT(b == true);
  ASSERT(s == "something_else");
  ASSERT(i == 789);
  ASSERT(arg1.isSet());
  ASSERT(a1 == "posarg1");
  ASSERT(arg1.isSet());
  ASSERT(a2 == "posarg2");

  // It still succeeds even without the last optional argument
  ASSERT(get2.get_args(7, argv4));
  ASSERT(get2.errmsg() == "");
  // But fails with missing mandatory argument
  ASSERT_FALSE(get2.get_args(6, argv4));
  ASSERT(get2.errmsg().find("Missing positional argument") != std::string::npos);
  ASSERT(get2.errmsg().find("arg1") != std::string::npos);

  char* argv5[4] = {(char*)"myprog",
                    (char*)"-bool", // Malformed switch
                    (char*)"posarg1", (char*)"posarg2"};

  ASSERT_FALSE(get2.get_args(4, argv5));
  ASSERT(get2.errmsg().find("Unknown argument") != std::string::npos);

  ASSERT(get2.errmsg().find("-bool") != std::string::npos);

}
