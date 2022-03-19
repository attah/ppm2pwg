#include "bytestream.h"
#include "test.h"
#include "subprocess.hpp"
#include "PwgPgHdr.h"
#include "pwg2ppm.h"
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

TEST(ppm2pwg)
{
  Bytestream ppm = PacmanPpm();

  // For sanity, make sure the one on disk is the same as created above
  std::ifstream ppm_ifs("pacman.ppm");
  Bytestream expected_ppm(ppm_ifs);
  ASSERT(ppm == expected_ppm);

  subprocess::popen ppm2pwg("../ppm2pwg", {});
  ppm2pwg.stdin().write((char*)ppm.raw(), ppm.size());
  ppm2pwg.close();

  ASSERT(ppm2pwg.wait() == 0);

  Bytestream pwg(ppm2pwg.stdout());
  PwgPgHdr hdr;

  ASSERT(pwg >>= "RaS2");
  hdr.decode_from(pwg);

  Bytestream enc = RightSideUp();

  ASSERT(pwg >>= enc);
  ASSERT(pwg.atEnd());

  std::ifstream ifs("pacman.pwg");
  Bytestream expected_pwg(ifs);

  ASSERT(pwg == expected_pwg);

}

TEST(duplex_normal)
{
  Bytestream twoSided;
  twoSided << PacmanPpm() << PacmanPpm();

  subprocess::popen ppm2pwg("../ppm2pwg", {});
  ppm2pwg.stdin().write((char*)twoSided.raw(), twoSided.size());
  ppm2pwg.close();

  ASSERT(ppm2pwg.wait() == 0);

  Bytestream pwg(ppm2pwg.stdout());
  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decode_from(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  ASSERT(pwg >>= RightSideUp());
  hdr2.decode_from(pwg);
  ASSERT(hdr2.CrossFeedTransform == 1);
  ASSERT(hdr2.FeedTransform == 1);
  ASSERT(pwg >>= RightSideUp());
  ASSERT(pwg.atEnd());
}

TEST(duplex_vflip)
{
  Bytestream twoSided;
  twoSided << PacmanPpm() << PacmanPpm();

  setenv("BACK_VFLIP", "true", true);
  setenv("BACK_HFLIP", "false", true);
  subprocess::popen ppm2pwg("../ppm2pwg", {});
  ppm2pwg.stdin().write((char*)twoSided.raw(), twoSided.size());
  ppm2pwg.close();

  ASSERT(ppm2pwg.wait() == 0);

  Bytestream pwg(ppm2pwg.stdout());
  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decode_from(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  ASSERT(pwg >>= RightSideUp());
  hdr2.decode_from(pwg);
  ASSERT(hdr2.CrossFeedTransform == 1);
  ASSERT(hdr2.FeedTransform == -1);
  ASSERT(pwg >>= UpsideDown());
  ASSERT(pwg.atEnd());
}

TEST(duplex_hflip)
{
  Bytestream twoSided;
  twoSided << PacmanPpm() << PacmanPpm();

  setenv("BACK_VFLIP", "false", true);
  setenv("BACK_HFLIP", "true", true);
  subprocess::popen ppm2pwg("../ppm2pwg", {});
  ppm2pwg.stdin().write((char*)twoSided.raw(), twoSided.size());
  ppm2pwg.close();

  ASSERT(ppm2pwg.wait() == 0);

  Bytestream pwg(ppm2pwg.stdout());
  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decode_from(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  ASSERT(pwg >>= RightSideUp());
  hdr2.decode_from(pwg);
  ASSERT(hdr2.CrossFeedTransform == -1);
  ASSERT(hdr2.FeedTransform == 1);
  ASSERT(pwg >>= Flipped());
  ASSERT(pwg.atEnd());
}

TEST(duplex_rotated)
{
  Bytestream twoSided;
  twoSided << PacmanPpm() << PacmanPpm();

  setenv("BACK_VFLIP", "true", true);
  setenv("BACK_HFLIP", "true", true);
  subprocess::popen ppm2pwg("../ppm2pwg", {});
  ppm2pwg.stdin().write((char*)twoSided.raw(), twoSided.size());
  ppm2pwg.close();

  ASSERT(ppm2pwg.wait() == 0);

  Bytestream pwg(ppm2pwg.stdout());
  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decode_from(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  ASSERT(pwg >>= RightSideUp());
  hdr2.decode_from(pwg);
  ASSERT(hdr2.CrossFeedTransform == -1);
  ASSERT(hdr2.FeedTransform == -1);
  ASSERT(pwg >>= Rotated());
  ASSERT(pwg.atEnd());
}

bool close_enough(size_t a, size_t b, size_t precision)
{
  size_t lower = b == 0 ? 0 : b-precision;
  size_t upper = b == 0 ? 0 : b+precision;
  return (a <= upper) && (a >= lower);
}

void do_test_16x9(const char* test_name, std::string filename, bool asymmetric)
{
  setenv("FORMAT", "pwg", true);
  setenv("HWRES_X", "300", true);
  setenv("HWRES_Y", asymmetric ? "600" : "300", true);

  subprocess::popen pdf2printable("../pdf2printable", {filename, test_name});
  pdf2printable.close();
  ASSERT(pdf2printable.wait() == 0);

  std::ifstream ifs(test_name, std::ios::in | std::ios::binary);
  Bytestream pwg(ifs);

  ASSERT(pwg >>= "RaS2");
  PwgPgHdr PwgHdr;
  PwgHdr.decode_from(pwg);

  size_t width = PwgHdr.Width;
  size_t height = PwgHdr.Height;
  size_t colors = PwgHdr.NumColors;

  ASSERT(close_enough(width, 2480, 1));
  ASSERT(close_enough(height, asymmetric ? 7015 : 3507, 1));

  Bytestream bmp;
  raster_to_bmp(bmp, pwg, width, height, colors, false);

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
  // ASSERT(close_enough(image_width, height*ratio, 2));

}

// More rectangular than A4
TEST(pdf2printable_16x9_portrait)
{
  do_test_16x9(__func__, "portrait_16x9.pdf", false);
}

TEST(pdf2printable_16x9_landscape)
{
  do_test_16x9(__func__, "landscape_16x9.pdf", false);
}

TEST(pdf2printable_16x9_portrait_asymmetric)
{
  do_test_16x9(__func__, "portrait_16x9.pdf", true);
}

TEST(pdf2printable_16x9_landscape_asymmetric)
{
  do_test_16x9(__func__, "landscape_16x9.pdf", true);
}

// More square than A4
TEST(pdf2printable_4x3_portrait)
{
  do_test_16x9(__func__, "portrait_4x3.pdf", false);
}

TEST(pdf2printable_4x3_landscape)
{
  do_test_16x9(__func__, "landscape_4x3.pdf", false);
}

TEST(pdf2printable_4x3_portrait_asymmetric)
{
  do_test_16x9(__func__, "portrait_4x3.pdf", true);
}

TEST(pdf2printable_4x3_landscape_asymmetric)
{
  do_test_16x9(__func__, "landscape_4x3.pdf", true);
}
