#include "bytestream.h"
#include "test.h"
#include "pwgpghdr.h"
#include "pwg2ppm.h"
#include "printparameters.h"
#include "argget.h"
#include "lthread.h"
#include "ippmsg.h"
#include "ippprinter.h"
#include "ippprintjob.h"
#include "json11.hpp"
#include <cstring>
#include <filesystem>
using namespace std;
using namespace json11;

#define REPEAT(N) (uint8_t)(N-1)
#define VERBATIM(N) (uint8_t)(257-N)

template <typename T>
Bytestream PacmanPpm()
{
  T max = std::numeric_limits<T>::max();
  Bytestream W {(T)max, (T)max, (T)max};
  Bytestream R {(T)max, (T)0,   (T)0};
  Bytestream G {(T)0,   (T)max, (T)0};
  Bytestream B {(T)0,   (T)0,   (T)max};
  Bytestream Y {(T)max, (T)max, (T)0};
  Bytestream ppm {"P6\n8 8\n", to_string(max), "\n"};
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

template <typename T>
Bytestream RightSideUp()
{
  T max = std::numeric_limits<T>::max();
  Bytestream W {(T)max, (T)max, (T)max};
  Bytestream R {(T)max, (T)0,   (T)0};
  Bytestream G {(T)0,   (T)max, (T)0};
  Bytestream B {(T)0,   (T)0,   (T)max};
  Bytestream Y {(T)max, (T)max, (T)0};
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

template <typename T>
Bytestream UpsideDown()
{
  T max = std::numeric_limits<T>::max();
  Bytestream W {(T)max, (T)max, (T)max};
  Bytestream R {(T)max, (T)0,   (T)0};
  Bytestream G {(T)0,   (T)max, (T)0};
  Bytestream B {(T)0,   (T)0,   (T)max};
  Bytestream Y {(T)max, (T)max, (T)0};
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

template <typename T>
Bytestream Flipped()
{
  T max = std::numeric_limits<T>::max();
  Bytestream W {(T)max, (T)max, (T)max};
  Bytestream R {(T)max, (T)0,   (T)0};
  Bytestream G {(T)0,   (T)max, (T)0};
  Bytestream B {(T)0,   (T)0,   (T)max};
  Bytestream Y {(T)max, (T)max, (T)0};
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

template <typename T>
Bytestream Rotated()
{
  T max = std::numeric_limits<T>::max();
  Bytestream W {(T)max, (T)max, (T)max};
  Bytestream R {(T)max, (T)0,   (T)0};
  Bytestream G {(T)0,   (T)max, (T)0};
  Bytestream B {(T)0,   (T)0,   (T)max};
  Bytestream Y {(T)max, (T)max, (T)0};
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

Bytestream run_ppm2pwg(List<std::string> args, Bytestream input, std::string tmpfile)
{
  std::filesystem::remove(tmpfile);
  args.push_front("../ppm2pwg");
  args += {"-", tmpfile};
  FILE* file = popen(join_string(args, " ").c_str(), "w");
  fwrite(input.raw(), input.size(), 1, file);
  pclose(file);

  std::ifstream pwg_ifs(tmpfile);
  return Bytestream(pwg_ifs);
}

Bytestream run_pdf2printable(List<std::string> args, std::string infile, std::string outfile)
{
  args.push_front("../pdf2printable");
  args += {infile, outfile};
  FILE* file = popen(join_string(args, " ").c_str(), "w");
  pclose(file);
  std::ifstream pwg_ifs(outfile);
  return Bytestream(pwg_ifs);
}

TEST(ppm2pwg)
{
  Bytestream ppm = PacmanPpm<uint8_t>();

  // For sanity, make sure the one on disk is the same as created above
  std::ifstream ppm_ifs("pacman.ppm");
  Bytestream expected_ppm(ppm_ifs);
  ASSERT(ppm == expected_ppm);

  Bytestream pwg = run_ppm2pwg({}, ppm, __func__);

  PwgPgHdr hdr;

  ASSERT(pwg >>= "RaS2");
  hdr.decodeFrom(pwg);

  Bytestream enc = RightSideUp<uint8_t>();

  ASSERT(pwg >>= enc);
  ASSERT(pwg.atEnd());

  pwg.setPos(0);

  std::ifstream ifs("pacman.pwg");
  Bytestream expected_pwg(ifs);

  ASSERT(pwg == expected_pwg);
}

TEST(ppm2pwg_16bit)
{
  Bytestream ppm = PacmanPpm<uint16_t>();
  Bytestream pwg = run_ppm2pwg({}, ppm, __func__);

  PwgPgHdr hdr;

  ASSERT(pwg >>= "RaS2");
  hdr.decodeFrom(pwg);

  Bytestream enc = RightSideUp<uint16_t>();

  ASSERT(pwg >>= enc);
  ASSERT(pwg.atEnd());
}

template <typename T>
void basic_pacman_asserts(const PwgPgHdr& hdr)
{
  ASSERT(hdr.Width == 8);
  ASSERT(hdr.Height == 8);
  ASSERT(hdr.BitsPerColor == 8 * sizeof(T));
  ASSERT(hdr.BitsPerPixel == 3 * 8 * sizeof(T));
  ASSERT(hdr.BytesPerLine == 3 * 8 * sizeof(T));
  ASSERT(hdr.ColorSpace == PwgPgHdr::sRGB);
  ASSERT(hdr.NumColors == 3);
}

TEST(duplex_normal)
{
  Bytestream twoSided;
  twoSided << PacmanPpm<uint8_t>() << PacmanPpm<uint8_t>();

  Bytestream pwg = run_ppm2pwg({"-d"}, twoSided, __func__);

  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decodeFrom(pwg);
  ASSERT(hdr1.Duplex == true);
  ASSERT(hdr1.Tumble == false);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  basic_pacman_asserts<uint8_t>(hdr1);
  ASSERT(pwg >>= RightSideUp<uint8_t>());
  hdr2.decodeFrom(pwg);
  ASSERT(hdr2.CrossFeedTransform == 1);
  ASSERT(hdr2.FeedTransform == 1);
  basic_pacman_asserts<uint8_t>(hdr2);
  ASSERT(pwg >>= RightSideUp<uint8_t>());
  ASSERT(pwg.atEnd());
}

TEST(duplex_normal_16bit)
{
  Bytestream twoSided;
  twoSided << PacmanPpm<uint16_t>() << PacmanPpm<uint16_t>();

  Bytestream pwg = run_ppm2pwg({"-d"}, twoSided, __func__);

  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decodeFrom(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  basic_pacman_asserts<uint16_t>(hdr1);
  ASSERT(pwg >>= RightSideUp<uint16_t>());
  hdr2.decodeFrom(pwg);
  ASSERT(hdr2.CrossFeedTransform == 1);
  ASSERT(hdr2.FeedTransform == 1);
  basic_pacman_asserts<uint16_t>(hdr2);
  ASSERT(pwg >>= RightSideUp<uint16_t>());
  ASSERT(pwg.atEnd());
}

TEST(duplex_vflip)
{
  Bytestream twoSided;
  twoSided << PacmanPpm<uint8_t>() << PacmanPpm<uint8_t>();

  Bytestream pwg = run_ppm2pwg({"-d", "--back-xform", "flip"}, twoSided, __func__);

  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decodeFrom(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  basic_pacman_asserts<uint8_t>(hdr1);
  ASSERT(pwg >>= RightSideUp<uint8_t>());
  hdr2.decodeFrom(pwg);
  ASSERT(hdr2.CrossFeedTransform == 1);
  ASSERT(hdr2.FeedTransform == -1);
  basic_pacman_asserts<uint8_t>(hdr2);
  ASSERT(pwg >>= UpsideDown<uint8_t>());
  ASSERT(pwg.atEnd());
}

TEST(duplex_vflip_16bit)
{
  Bytestream twoSided;
  twoSided << PacmanPpm<uint16_t>() << PacmanPpm<uint16_t>();

  Bytestream pwg = run_ppm2pwg({"-d", "--back-xform", "flip"}, twoSided, __func__);

  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decodeFrom(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  basic_pacman_asserts<uint16_t>(hdr1);
  ASSERT(pwg >>= RightSideUp<uint16_t>());
  hdr2.decodeFrom(pwg);
  ASSERT(hdr2.CrossFeedTransform == 1);
  ASSERT(hdr2.FeedTransform == -1);
  basic_pacman_asserts<uint16_t>(hdr2);
  ASSERT(pwg >>= UpsideDown<uint16_t>());
  ASSERT(pwg.atEnd());
}

TEST(duplex_hflip)
{
  Bytestream twoSided;
  twoSided << PacmanPpm<uint8_t>() << PacmanPpm<uint8_t>();

  Bytestream pwg = run_ppm2pwg({"-d", "-t", "--back-xform", "flip"}, twoSided, __func__);

  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decodeFrom(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  basic_pacman_asserts<uint8_t>(hdr1);
  ASSERT(pwg >>= RightSideUp<uint8_t>());
  hdr2.decodeFrom(pwg);
  ASSERT(hdr2.CrossFeedTransform == -1);
  ASSERT(hdr2.FeedTransform == 1);
  basic_pacman_asserts<uint8_t>(hdr2);
  ASSERT(pwg >>= Flipped<uint8_t>());
  ASSERT(pwg.atEnd());
}

TEST(duplex_hflip_16bit)
{
  Bytestream twoSided;
  twoSided << PacmanPpm<uint16_t>() << PacmanPpm<uint16_t>();

  Bytestream pwg = run_ppm2pwg({"-d", "-t", "--back-xform", "flip"}, twoSided, __func__);

  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decodeFrom(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  basic_pacman_asserts<uint16_t>(hdr1);
  ASSERT(pwg >>= RightSideUp<uint16_t>());
  hdr2.decodeFrom(pwg);
  ASSERT(hdr2.CrossFeedTransform == -1);
  ASSERT(hdr2.FeedTransform == 1);
  basic_pacman_asserts<uint16_t>(hdr2);
  ASSERT(pwg >>= Flipped<uint16_t>());
  ASSERT(pwg.atEnd());
}

TEST(duplex_rotated)
{
  Bytestream twoSided;
  twoSided << PacmanPpm<uint8_t>() << PacmanPpm<uint8_t>();

  Bytestream pwg = run_ppm2pwg({"-d", "--back-xform", "rotate"}, twoSided, __func__);

  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decodeFrom(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  basic_pacman_asserts<uint8_t>(hdr1);
  ASSERT(pwg >>= RightSideUp<uint8_t>());
  hdr2.decodeFrom(pwg);
  ASSERT(hdr2.CrossFeedTransform == -1);
  ASSERT(hdr2.FeedTransform == -1);
  basic_pacman_asserts<uint8_t>(hdr2);
  ASSERT(pwg >>= Rotated<uint8_t>());
  ASSERT(pwg.atEnd());
}

TEST(duplex_rotated_16bit)
{
  Bytestream twoSided;
  twoSided << PacmanPpm<uint16_t>() << PacmanPpm<uint16_t>();

  Bytestream pwg = run_ppm2pwg({"-d", "--back-xform", "rotate"}, twoSided, __func__);

  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decodeFrom(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  basic_pacman_asserts<uint16_t>(hdr1);
  ASSERT(pwg >>= RightSideUp<uint16_t>());
  hdr2.decodeFrom(pwg);
  ASSERT(hdr2.CrossFeedTransform == -1);
  ASSERT(hdr2.FeedTransform == -1);
  basic_pacman_asserts<uint16_t>(hdr2);
  ASSERT(pwg >>= Rotated<uint16_t>());
  ASSERT(pwg.atEnd());
}

TEST(two_pages_no_duplex)
{
  Bytestream twoSided;
  twoSided << PacmanPpm<uint8_t>() << PacmanPpm<uint8_t>();

  Bytestream pwg = run_ppm2pwg({"--back-xform", "flip"}, twoSided, __func__);

  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decodeFrom(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  basic_pacman_asserts<uint8_t>(hdr1);
  ASSERT(pwg >>= RightSideUp<uint8_t>());
  hdr2.decodeFrom(pwg);
  ASSERT(hdr2.CrossFeedTransform == 1);
  ASSERT(hdr2.FeedTransform == 1);
  basic_pacman_asserts<uint8_t>(hdr2);
  ASSERT(pwg >>= RightSideUp<uint8_t>());
  ASSERT(pwg.atEnd());
}

TEST(two_pages_no_duplex_16bit)
{
  Bytestream twoSided;
  twoSided << PacmanPpm<uint16_t>() << PacmanPpm<uint16_t>();

  Bytestream pwg = run_ppm2pwg({"--back-xform", "flip"}, twoSided, __func__);

  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decodeFrom(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  basic_pacman_asserts<uint16_t>(hdr1);
  ASSERT(pwg >>= RightSideUp<uint16_t>());
  hdr2.decodeFrom(pwg);
  ASSERT(hdr2.CrossFeedTransform == 1);
  ASSERT(hdr2.FeedTransform == 1);
  basic_pacman_asserts<uint16_t>(hdr2);
  ASSERT(pwg >>= RightSideUp<uint16_t>());
  ASSERT(pwg.atEnd());
}

void basic_bilevel_asserts(const PwgPgHdr& hdr)
{
  ASSERT(hdr.Width == 24);
  ASSERT(hdr.Height == 8);
  ASSERT(hdr.BitsPerColor == 1);
  ASSERT(hdr.BitsPerPixel == 1);
  ASSERT(hdr.BytesPerLine == 3);
  ASSERT(hdr.ColorSpace == PwgPgHdr::Black);
  ASSERT(hdr.NumColors == 1);
}

TEST(bilevel)
{
  Bytestream P4 = P4_0101();

  Bytestream pwg = run_ppm2pwg({}, P4, __func__);

  PwgPgHdr hdr1;
  ASSERT(pwg >>= "RaS2");
  hdr1.decodeFrom(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  basic_bilevel_asserts(hdr1);
  ASSERT(pwg >>= BilevelPwg0101());
}

TEST(bilevel_vflip)
{
  Bytestream twoSided;
  twoSided << P4_0101() << P4_0101();

  Bytestream pwg = run_ppm2pwg({"-d", "--back-xform", "flip"}, twoSided, __func__);

  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decodeFrom(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  basic_bilevel_asserts(hdr1);
  ASSERT(pwg >>= BilevelPwg0101());
  hdr2.decodeFrom(pwg);
  ASSERT(hdr2.CrossFeedTransform == 1);
  ASSERT(hdr2.FeedTransform == -1);
  basic_bilevel_asserts(hdr2);
  ASSERT(pwg >>= BilevelPwg0101_UpsideDown());
  ASSERT(pwg.atEnd());
}

TEST(bilevel_hflip)
{
  Bytestream twoSided;
  twoSided << P4_0101() << P4_0101();

  Bytestream pwg = run_ppm2pwg({"-d", "-t", "--back-xform", "flip"}, twoSided, __func__);

  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decodeFrom(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  basic_bilevel_asserts(hdr1);
  ASSERT(pwg >>= BilevelPwg0101());
  hdr2.decodeFrom(pwg);
  ASSERT(hdr2.CrossFeedTransform == -1);
  ASSERT(hdr2.FeedTransform == 1);
  basic_bilevel_asserts(hdr2);
  ASSERT(pwg >>= BilevelPwg0101_Flipped());
  ASSERT(pwg.atEnd());
}

TEST(bilevel_rotated)
{
  Bytestream twoSided;
  twoSided << P4_0101() << P4_0101();

  Bytestream pwg = run_ppm2pwg({"-d", "--back-xform", "rotate"}, twoSided, __func__);

  PwgPgHdr hdr1, hdr2;

  ASSERT(pwg >>= "RaS2");
  hdr1.decodeFrom(pwg);
  ASSERT(hdr1.CrossFeedTransform == 1);
  ASSERT(hdr1.FeedTransform == 1);
  basic_bilevel_asserts(hdr1);
  ASSERT(pwg >>= BilevelPwg0101());
  hdr2.decodeFrom(pwg);
  ASSERT(hdr2.CrossFeedTransform == -1);
  ASSERT(hdr2.FeedTransform == -1);
  basic_bilevel_asserts(hdr2);
  ASSERT(pwg >>= BilevelPwg0101_Rotated());
  ASSERT(pwg.atEnd());
}

bool close_enough(int a, int b, unsigned int precision)
{
  int lower = b == 0 ? 0 : b-precision;
  int upper = b == 0 ? 0 : b+precision;
  return (a <= upper) && (a >= lower);
}

void do_test_centering(const char* test_name, std::string filename, bool asymmetric)
{

  Bytestream pwg = run_pdf2printable({"--format", "pwg", "-rx", "300", "-ry", asymmetric ? "600" : "300"},
                                     filename, test_name);
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
  raster_to_bmp(bmp, pwg, width*colors, height, colors, false);

  size_t left_margin, right_margin, top_margin, bottom_margin;

  left_margin = 0;
  right_margin = 0;
  top_margin = 0;
  bottom_margin = 0;

  Bytestream white_line(width*colors, 0xff);

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
  middle_line = bmp.getBytestream(width*colors);

  Bytestream white_pixel(colors, 0xff);

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
  ASSERT(close_enough(top_margin, bottom_margin, asymmetric ? 2 : 1));

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
  A4Px.paperSizeW = A4.getPaperSizeWInPixels();
  A4Px.paperSizeH = A4.getPaperSizeHInPixels();
  A4Px.hwResW = 300;
  A4Px.hwResH = 300;

  ASSERT(A4.getPaperSizeWInPixels() == 2480);
  ASSERT(A4.getPaperSizeHInPixels() == 3507);

  ASSERT(A4.getPaperSizeWInBytes() == 2480*3);
  ASSERT(A4.getPaperSizeInPixels() == 2480*3507);
  ASSERT(A4.getPaperSizeInBytes() == 2480*3507*3);

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
  A4Px.paperSizeW = A4.getPaperSizeWInPixels();
  A4Px.paperSizeH = A4.getPaperSizeHInPixels();
  A4Px.hwResW = 600;
  A4Px.hwResH = 600;

  ASSERT(A4.getPaperSizeWInPixels() == 4960);
  ASSERT(A4.getPaperSizeHInPixels() == 7015);

  ASSERT(A4.getPaperSizeWInBytes() == 4960*3);
  ASSERT(A4.getPaperSizeInPixels() == 4960*7015);
  ASSERT(A4.getPaperSizeInBytes() == 4960*7015*3);

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
  LetterPx.paperSizeW = Letter.getPaperSizeWInPixels();
  LetterPx.paperSizeH = Letter.getPaperSizeHInPixels();
  LetterPx.hwResW = 300;
  LetterPx.hwResH = 300;

  ASSERT(Letter.getPaperSizeWInPixels() == 2550);
  ASSERT(Letter.getPaperSizeHInPixels() == 3300);

  ASSERT(Letter.getPaperSizeWInBytes() == 2550*3);
  ASSERT(Letter.getPaperSizeInPixels() == 2550*3300);
  ASSERT(Letter.getPaperSizeInBytes() == 2550*3300*3);

  ASSERT(Letter.getPaperSizeWInMillimeters() == 215.9);
  ASSERT(Letter.getPaperSizeHInMillimeters() == 279.4);

  ASSERT(Letter.getPaperSizeWInPoints() == 612);
  ASSERT(Letter.getPaperSizeHInPoints() == 792);

  ASSERT(LetterPx.getPaperSizeWInMillimeters() == 215.9);
  ASSERT(LetterPx.getPaperSizeHInMillimeters() == 279.4);

  ASSERT(LetterPx.getPaperSizeWInPoints() == 612);
  ASSERT(LetterPx.getPaperSizeHInPoints() == 792);

  Letter.hwResW = 600;
  Letter.hwResH = 600;
  LetterPx.paperSizeW = Letter.getPaperSizeWInPixels();
  LetterPx.paperSizeH = Letter.getPaperSizeHInPixels();
  LetterPx.hwResW = 600;
  LetterPx.hwResH = 600;

  ASSERT(Letter.getPaperSizeWInPixels() == 5100);
  ASSERT(Letter.getPaperSizeHInPixels() == 6600);

  ASSERT(Letter.getPaperSizeWInBytes() == 5100*3);
  ASSERT(Letter.getPaperSizeInPixels() == 5100*6600);
  ASSERT(Letter.getPaperSizeInBytes() == 5100*6600*3);

  ASSERT(Letter.getPaperSizeWInMillimeters() == 215.9);
  ASSERT(Letter.getPaperSizeHInMillimeters() == 279.4);

  ASSERT(Letter.getPaperSizeWInPoints() == 612);
  ASSERT(Letter.getPaperSizeHInPoints() == 792);

  ASSERT(LetterPx.getPaperSizeWInMillimeters() == 215.9);
  ASSERT(LetterPx.getPaperSizeHInMillimeters() == 279.4);

  ASSERT(LetterPx.getPaperSizeWInPoints() == 612);
  ASSERT(LetterPx.getPaperSizeHInPoints() == 792);

  A4.hwResH = 1200;
  A4Px.hwResH = 1200;
  A4Px.paperSizeH = A4.getPaperSizeHInPixels();

  Letter.hwResH = 1200;
  LetterPx.hwResH = 1200;
  LetterPx.paperSizeH = Letter.getPaperSizeHInPixels();

  ASSERT(A4.getPaperSizeWInPixels() == 4960);
  ASSERT(A4.getPaperSizeHInPixels() == 14031);
  ASSERT(A4Px.getPaperSizeWInPixels() == 4960);
  ASSERT(A4Px.getPaperSizeHInPixels() == 14031);

  ASSERT(A4.getPaperSizeWInBytes() == 4960*3);
  ASSERT(A4.getPaperSizeInPixels() == 4960*14031);
  ASSERT(A4.getPaperSizeInBytes() == 4960*14031*3);

  ASSERT(A4Px.getPaperSizeWInBytes() == 4960*3);
  ASSERT(A4Px.getPaperSizeInPixels() == 4960*14031);
  ASSERT(A4Px.getPaperSizeInBytes() == 4960*14031*3);

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
  ASSERT(Letter.getPaperSizeWInMillimeters() == 215.9);
  ASSERT(Letter.getPaperSizeHInMillimeters() == 279.4);
  ASSERT(LetterPx.getPaperSizeWInMillimeters() == 215.9);
  ASSERT(LetterPx.getPaperSizeHInMillimeters() == 279.4);

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

  params.copies = 2;
  params.collatedCopies = true;
  params.format = PrintParameters::PWG;
  params.pageRangeList = {{4, 7}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 8);
  ASSERT(seq == PageSequence({4, 5, 6, 7,
                              4, 5, 6, 7}));

  params.duplexMode = PrintParameters::OneSided;
  params.copies = 2;
  params.collatedCopies = true;
  params.format = PrintParameters::PWG;
  params.pageRangeList = {{4, 6}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 6);
  ASSERT(seq == PageSequence({4, 5, 6,
                              4, 5, 6}));

  params.duplexMode = PrintParameters::OneSided;
  params.copies = 3;
  params.collatedCopies = true;
  params.pageRangeList = {{4, 6}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 9);
  ASSERT(seq == PageSequence({4, 5, 6,
                              4, 5, 6,
                              4, 5, 6}));

  params.duplexMode = PrintParameters::TwoSidedLongEdge;
  params.copies = 2;
  params.collatedCopies = true;
  params.format = PrintParameters::PWG;
  params.pageRangeList = {{4, 6}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 8);
  ASSERT(seq == PageSequence({4, 5, 6, INVALID_PAGE,
                              4, 5, 6, INVALID_PAGE}));

  params.duplexMode = PrintParameters::TwoSidedLongEdge;
  params.copies = 3;
  params.pageRangeList = {{4, 6}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 12);
  ASSERT(seq == PageSequence({4, 5, 6, INVALID_PAGE,
                              4, 5, 6, INVALID_PAGE,
                              4, 5, 6, INVALID_PAGE}));

  params.duplexMode = PrintParameters::OneSided;
  params.copies = 2;
  params.collatedCopies = false;
  params.pageRangeList = {{4, 7}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 8);
  ASSERT(seq == PageSequence({4, 4,
                              5, 5,
                              6, 6,
                              7, 7}));

  params.duplexMode = PrintParameters::TwoSidedLongEdge;
  params.copies = 2;
  params.collatedCopies = false;
  params.pageRangeList = {{4, 7}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 8);
  ASSERT(seq == PageSequence({4, 5,
                              4, 5,
                              6, 7,
                              6, 7}));

  params.duplexMode = PrintParameters::OneSided;
  params.copies = 3;
  params.collatedCopies = true;
  params.pageRangeList = {{1, 2}, {5, 7}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 15);
  ASSERT(seq == PageSequence({1, 2, 5, 6, 7,
                              1, 2, 5, 6, 7,
                              1, 2, 5, 6, 7}));

  params.duplexMode = PrintParameters::TwoSidedLongEdge;
  params.copies = 3;
  params.collatedCopies = true;
  params.pageRangeList = {{1, 2}, {5, 7}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 18);
  ASSERT(seq == PageSequence({1, 2, 5, 6, 7, INVALID_PAGE,
                              1, 2, 5, 6, 7, INVALID_PAGE,
                              1, 2, 5, 6, 7, INVALID_PAGE}));

  params.duplexMode = PrintParameters::TwoSidedLongEdge;
  params.copies = 3;
  params.collatedCopies = true;
  params.pageRangeList = {{1, 2}, {6, 7}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 12);
  ASSERT(seq == PageSequence({1, 2, 6, 7,
                              1, 2, 6, 7,
                              1, 2, 6, 7}));

  params.duplexMode = PrintParameters::OneSided;
  params.copies = 2;
  params.collatedCopies = false;
  params.pageRangeList = {{1, 2}, {5, 7}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 10);
  ASSERT(seq == PageSequence({1, 1,
                              2, 2,
                              5, 5,
                              6, 6,
                              7, 7}));

  params.duplexMode = PrintParameters::TwoSidedLongEdge;
  params.copies = 2;
  params.collatedCopies = false;
  params.pageRangeList = {{1, 2}, {5, 7}};
  seq = params.getPageSequence(100);
  ASSERT(seq.size() == 12);
  ASSERT(seq == PageSequence({1, 2,
                              1, 2,
                              5, 6,
                              5, 6,
                              7, INVALID_PAGE,
                              7, INVALID_PAGE}));

  params.duplexMode = PrintParameters::OneSided;
  params.copies = 1;
  params.pageRangeList = {};
  seq = params.getPageSequence(3);
  ASSERT(seq.size() == 3);
  ASSERT(seq == PageSequence({1, 2, 3}));

  params.duplexMode = PrintParameters::OneSided;
  params.copies = 2;
  params.collatedCopies = true;
  params.pageRangeList = {};
  seq = params.getPageSequence(3);
  ASSERT(seq.size() == 6);
  ASSERT(seq == PageSequence({1, 2, 3,
                              1, 2, 3}));

  params.duplexMode = PrintParameters::OneSided;
  params.copies = 2;
  params.collatedCopies = false;
  params.pageRangeList = {};
  seq = params.getPageSequence(3);
  ASSERT(seq.size() == 6);
  ASSERT(seq == PageSequence({1, 1,
                              2, 2,
                              3, 3}));

  params.duplexMode = PrintParameters::TwoSidedLongEdge;
  params.copies = 2;
  params.collatedCopies = true;
  params.pageRangeList = {};
  seq = params.getPageSequence(3);
  ASSERT(seq.size() == 8);
  ASSERT(seq == PageSequence({1, 2, 3, INVALID_PAGE,
                              1, 2, 3, INVALID_PAGE}));

  params.duplexMode = PrintParameters::TwoSidedLongEdge;
  params.copies = 2;
  params.collatedCopies = false;
  params.pageRangeList = {};
  seq = params.getPageSequence(3);
  ASSERT(seq.size() == 8);
  ASSERT(seq == PageSequence({1, 2,
                              1, 2,
                              3, INVALID_PAGE,
                              3, INVALID_PAGE}));

  params.duplexMode = PrintParameters::OneSided;
  params.copies = 1;
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

  ASSERT_FALSE(params.setPageRange("0-1"));
  ASSERT(params.pageRangeList.empty());
  ASSERT_FALSE(params.setPageRange("1-2,2-3"));
  ASSERT(params.pageRangeList.empty());
  ASSERT_FALSE(params.setPageRange("1-,2-"));
  ASSERT(params.pageRangeList.empty());

  ASSERT(params.setPageRange("2-"));
  ASSERT(params.getPageSequence(3) == PageSequence({2,3}));
  ASSERT(params.getPageSequence(4) == PageSequence({2,3,4}));
  ASSERT(params.setPageRange("1,2-"));
  ASSERT(params.getPageSequence(3) == PageSequence({1,2,3}));
  ASSERT(params.getPageSequence(4) == PageSequence({1,2,3,4}));
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
  ASSERT(params.paperSizeW == 8.5);
  ASSERT(params.paperSizeH == 11);
  ASSERT(params.paperSizeUnits == PrintParameters::Inches);

  ASSERT(params.setPaperSize("jpn_chou2_111.1x146mm"));
  ASSERT(params.paperSizeName == "jpn_chou2_111.1x146mm");
  ASSERT(params.paperSizeW == 111.1);
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
  ASSERT(params.paperSizeW == 13.37);
  ASSERT(params.paperSizeH == 42.69);
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
  params.colorMode = PrintParameters::CMYK32;
  ASSERT(params.getNumberOfColors() == 4);
  ASSERT(params.getBitsPerColor() == 8);
  ASSERT(params.isBlack() == false);
  params.colorMode = PrintParameters::Gray8;
  ASSERT(params.getNumberOfColors() == 1);
  ASSERT(params.getBitsPerColor() == 8);
  ASSERT(params.isBlack() == false);
  params.colorMode = PrintParameters::Black8;
  ASSERT(params.getNumberOfColors() == 1);
  ASSERT(params.getBitsPerColor() == 8);
  ASSERT(params.isBlack() == true);
  params.colorMode = PrintParameters::Gray1;
  ASSERT(params.getNumberOfColors() == 1);
  ASSERT(params.getBitsPerColor() == 1);
  ASSERT(params.isBlack() == false);
  params.colorMode = PrintParameters::Black1;
  ASSERT(params.getNumberOfColors() == 1);
  ASSERT(params.getBitsPerColor() == 1);
  ASSERT(params.isBlack() == true);
  params.colorMode = PrintParameters::sRGB48;
  ASSERT(params.getNumberOfColors() == 3);
  ASSERT(params.getBitsPerColor() == 16);
  ASSERT(params.isBlack() == false);
  params.colorMode = PrintParameters::Gray16;
  ASSERT(params.getNumberOfColors() == 1);
  ASSERT(params.getBitsPerColor() == 16);
  ASSERT(params.isBlack() == false);
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

TEST(argget_subcommand)
{
  bool b = false;
  SwitchArg<bool> boolopt(b, {"-b", "--bool"}, "A bool option");

  std::string s;
  SwitchArg<string> stringopt(s, {"-s", "--string"}, "A string option");

  std::string a1;
  PosArg arg1(a1, "arg1");

  SubArgGet get({{"boolify", {{&boolopt}, {}}},
                 {"stringify", {{&stringopt}, {}}},
                 {"posify", {{}, {&arg1}}}});
  char* argv[3] = {(char*)"myprog",
                   (char*)"boolify",
                   (char*)"-b"};

  ASSERT(get.get_args(3, argv));
  ASSERT(boolopt.isSet());
  ASSERT(b);

  char* argv1[4] = {(char*)"myprog",
                    (char*)"stringify",
                    (char*)"--string",
                    (char*)"mystring"};

  ASSERT(get.get_args(4, argv1));
  ASSERT(stringopt.isSet());
  ASSERT(s=="mystring");

  char* argv2[3] = {(char*)"myprog",
                    (char*)"posify",
                    (char*)"myposarg"};

  ASSERT(get.get_args(3, argv2));
  ASSERT(arg1.isSet());
  ASSERT(a1=="myposarg");

  char* argv3[4] = {(char*)"myprog",
                    (char*)"stringify",
                    (char*)"-b"};

  ASSERT_FALSE(get.get_args(3, argv3));

}

TEST(flip_logic)
{
  PrintParameters params;
  ASSERT(params.backXformMode == PrintParameters::Normal);
  ASSERT(params.duplexMode == PrintParameters::OneSided);
  ASSERT_FALSE(params.isTwoSided());

  params.backXformMode = PrintParameters::Flipped;
  ASSERT(params.getBackHFlip() == false);
  ASSERT(params.getBackVFlip() == false);

  params.backXformMode = PrintParameters::ManualTumble;
  ASSERT(params.getBackHFlip() == false);
  ASSERT(params.getBackVFlip() == false);

  params.backXformMode = PrintParameters::Normal;
  ASSERT(params.getBackHFlip() == false);
  ASSERT(params.getBackVFlip() == false);

  params.backXformMode = PrintParameters::Rotated;
  ASSERT(params.getBackHFlip() == false);
  ASSERT(params.getBackVFlip() == false);

  params.duplexMode = PrintParameters::TwoSidedLongEdge;
  ASSERT(params.isTwoSided());

  params.backXformMode = PrintParameters::Flipped;
  ASSERT(params.getBackHFlip() == false);
  ASSERT(params.getBackVFlip() == true);

  params.backXformMode = PrintParameters::ManualTumble;
  ASSERT(params.getBackHFlip() == false);
  ASSERT(params.getBackVFlip() == false);

  params.backXformMode = PrintParameters::Normal;
  ASSERT(params.getBackHFlip() == false);
  ASSERT(params.getBackVFlip() == false);

  params.backXformMode = PrintParameters::Rotated;
  ASSERT(params.getBackHFlip() == true);
  ASSERT(params.getBackVFlip() == true);

  params.duplexMode = PrintParameters::TwoSidedShortEdge;
  ASSERT(params.isTwoSided()); // Still two-sided

  params.backXformMode = PrintParameters::Flipped;
  ASSERT(params.getBackHFlip() == true);
  ASSERT(params.getBackVFlip() == false);

  params.backXformMode = PrintParameters::ManualTumble;
  ASSERT(params.getBackHFlip() == true);
  ASSERT(params.getBackVFlip() == true);

  params.backXformMode = PrintParameters::Normal;
  ASSERT(params.getBackHFlip() == false);
  ASSERT(params.getBackVFlip() == false);

  params.backXformMode = PrintParameters::Rotated;
  ASSERT(params.getBackHFlip() == false);
  ASSERT(params.getBackVFlip() == false);
}

TEST(lthread)
{
  LThread ltr;
  ltr.run([](){});
  ltr.await();

  bool started = false;
  bool proceed = false;

  LThread::runnable fun1 = [&started, &proceed]()
                           {
                             started = true;
                             while(!proceed);
                           };
  ltr.run(fun1);
  while(!started);
  ASSERT(ltr.isRunning());
  proceed = true;
  ltr.await();
}

TEST(ippattr)
{

  IppAttr attr1(IppTag::BeginCollection, IppCollection{{"key", IppAttr(IppTag::Integer, 42)}});
  IppAttr attr2(IppTag::Keyword, IppOneSetOf {IppValue {42}});
  ASSERT(attr2.isList());
  ASSERT(attr2.get<IppOneSetOf>().front().is<int>());
  ASSERT(attr2.asList().front().is<int>());
  ASSERT(attr2.asList().front().get<int>() == 42);

  IppAttr attr3(IppTag::Enum, 42);
  ASSERT_FALSE(attr3.isList());
  ASSERT(attr3.get<int>() == 42);
  ASSERT(attr3.asList().front().is<int>());
  ASSERT(attr3.asList().front().get<int>() == 42);

}

TEST(ippprintjob_support)
{
  IppAttrs printerAttrs =
    {{"sides-default", IppAttr(IppTag::Keyword, "one-sided")},
     {"sides-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"one-sided",
                                                               "two-sided-long-edge",
                                                               "two-sided-short-edge"})},
     {"media-default", IppAttr(IppTag::Keyword, "iso_a4_210x297mm")},
     {"media-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"iso_a4_210x297mm",
                                                               "na_letter_8.5x11in"})},
     {"media-ready", IppAttr(IppTag::Keyword, "iso_a4_210x297mm")},
     {"copies-default", IppAttr(IppTag::Integer, 1)},
     {"copies-supported", IppAttr(IppTag::IntegerRange, IppIntRange {1, 999})},
     {"multiple-document-handling-default", IppAttr(IppTag::Keyword, "separate-documents-uncollated-copies")},
     {"multiple-document-handling-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"single-document",
                                                                                    "separate-documents-uncollated-copies",
                                                                                    "separate-documents-collated-copies"})},
     {"page-ranges-supported", IppAttr(IppTag::Boolean, false)},
     {"number-up-default", IppAttr(IppTag::Integer, 1)},
     {"number-up-supported", IppAttr(IppTag::Integer, IppOneSetOf{1, 2, 4})},
     {"print-color-mode-default", IppAttr(IppTag::Keyword, "auto")},
     {"print-color-mode-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"auto", "color", "monochrome"})},
     {"print-quality-default", IppAttr(IppTag::Enum, 4)},
     {"print-quality-supported", IppAttr(IppTag::Enum, IppOneSetOf {3, 4, 5})},
     {"printer-resolution-default", IppAttr(IppTag::Resolution, IppResolution {600, 600, 3})},
     {"printer-resolution-supported", IppAttr(IppTag::Resolution, IppOneSetOf {IppResolution {600, 600, 3},
                                                                               IppResolution {600, 1200, 3}})},
     {"document-format-default", IppAttr(IppTag::Keyword, "application/octet-stream")},
     {"document-format-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"application/octet-stream",
                                                                         "image/urf",
                                                                         "image/pwg-raster",
                                                                         "application/pdf"})},
     {"print-scaling-default", IppAttr(IppTag::Keyword, "auto")},
     {"print-scaling-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"auto", "fill", "fit"})},
     {"media-type-default", IppAttr(IppTag::Keyword, "stationery")},
     {"media-type-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"stationery", "cardstock", "labels"})},
     {"media-source-default", IppAttr(IppTag::Keyword, "auto")},
     {"media-source-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"auto", "envelope", "manual", "tray-1"})},
     {"output-bin-default", IppAttr(IppTag::Keyword, "face-down")},
     {"output-bin-supported", IppAttr(IppTag::Keyword, "face-down")},

     {"media-top-margin-default", IppAttr(IppTag::Integer, 1)},
     {"media-top-margin-supported", IppAttr(IppTag::Integer, IppOneSetOf {1, 2, 3, 4})},
     {"media-bottom-margin-default", IppAttr(IppTag::Integer, 2)},
     {"media-bottom-margin-supported", IppAttr(IppTag::Integer, IppOneSetOf {2, 3, 4})},
     {"media-left-margin-default", IppAttr(IppTag::Integer, 3)},
     {"media-left-margin-supported", IppAttr(IppTag::Integer, IppOneSetOf {3, 4})},
     {"media-right-margin-default", IppAttr(IppTag::Integer, 4)},
     {"media-right-margin-supported", IppAttr(IppTag::Integer, 4)},
    };

  IppMsg getAttrsMsg(0, IppAttrs(), IppAttrs(), printerAttrs);
  Bytestream encoded = getAttrsMsg.encode();
  IppMsg msg2(encoded);

  ASSERT(printerAttrs == msg2.getPrinterAttrs());

  encoded -= encoded.size();
  IppMsg::setReqId(1); // Revert automatic global incrementation of ReqId.
  Bytestream encoded2 = msg2.encode();
  ASSERT(encoded2 == encoded);

  IppPrintJob ip(printerAttrs);

  ASSERT(ip.sides.isSupported());
  ASSERT(ip.sides.getDefault() == "one-sided");
  ASSERT(ip.sides.getSupported() == List<std::string>({"one-sided",
                                                       "two-sided-long-edge",
                                                       "two-sided-short-edge"}));

  ASSERT(ip.media.isSupported());
  ASSERT(ip.media.getDefault() == "iso_a4_210x297mm");
  ASSERT(ip.media.getSupported() == List<std::string>({"iso_a4_210x297mm", "na_letter_8.5x11in"}));
  // A single choice was not wrapped in OneSetOf, but still returned as a list.
  ASSERT(ip.media.getPreferred() == List<std::string>({"iso_a4_210x297mm"}));

  ASSERT(ip.copies.isSupported());
  ASSERT(ip.copies.getDefault() == 1);
  ASSERT(ip.copies.getSupportedMin() == 1);
  ASSERT(ip.copies.getSupportedMax() == 999);

  ASSERT(ip.multipleDocumentHandling.isSupported());
  ASSERT(ip.multipleDocumentHandling.getDefault() == "separate-documents-uncollated-copies");
  ASSERT(ip.multipleDocumentHandling.getSupported() == List<std::string>({"single-document",
                                                                          "separate-documents-uncollated-copies",
                                                                          "separate-documents-collated-copies"}));

  ASSERT(ip.pageRanges.isSupported());
  ASSERT(ip.pageRanges.getSupported() == false);

  ASSERT(ip.numberUp.isSupported());
  ASSERT(ip.numberUp.getDefault() == 1);
  ASSERT(ip.numberUp.getSupported() == List<int>({1, 2, 4}));

  // number-up-supported may be a range too...
  printerAttrs.set("number-up-supported", IppAttr(IppTag::IntegerRange, IppIntRange {1, 4}));

  ip = IppPrintJob(printerAttrs);

  ASSERT(ip.numberUp.isSupported());
  ASSERT(ip.numberUp.getDefault() == 1);
  ASSERT(ip.numberUp.getSupported() == List<int>({1, 2, 3, 4}));

  ASSERT(ip.colorMode.isSupported());
  ASSERT(ip.colorMode.getDefault() == "auto");
  ASSERT(ip.colorMode.getSupported() == List<std::string>({"auto", "color", "monochrome"}));

  ASSERT(ip.printQuality.isSupported());
  ASSERT(ip.printQuality.getDefault() == 4);
  ASSERT(ip.printQuality.getSupported() == List<int>({3, 4, 5}));

  ASSERT(ip.resolution.isSupported());
  ASSERT(ip.resolution.getDefault() == IppResolution({600, 600, 3}));
  ASSERT(ip.resolution.getSupported() == List<IppResolution>({{600, 600, 3}, {600, 1200, 3}}));

  ASSERT(ip.scaling.isSupported());
  ASSERT(ip.scaling.getDefault() == "auto");
  ASSERT(ip.scaling.getSupported() == List<std::string>({"auto", "fill", "fit"}));

  ASSERT(ip.documentFormat.isSupported());
  ASSERT(ip.documentFormat.getDefault() == "application/octet-stream");
  ASSERT(ip.documentFormat.getSupported() == List<std::string>({"application/octet-stream",
                                                                "image/urf",
                                                                "image/pwg-raster",
                                                                "application/pdf"}));

  ASSERT(ip.mediaType.isSupported());
  ASSERT(ip.mediaType.getDefault() == "stationery");
  ASSERT(ip.mediaType.getSupported() == List<std::string>({"stationery", "cardstock", "labels"}));

  ASSERT(ip.mediaSource.isSupported());
  ASSERT(ip.mediaSource.getDefault() == "auto");
  ASSERT(ip.mediaSource.getSupported() == List<std::string>({"auto", "envelope", "manual", "tray-1"}));

  ASSERT(ip.outputBin.isSupported());
  ASSERT(ip.outputBin.getDefault() == "face-down");
  ASSERT(ip.outputBin.getSupported() == List<std::string>({"face-down"}));

  ASSERT(ip.topMargin.isSupported());
  ASSERT(ip.topMargin.getDefault() == 1);
  ASSERT(ip.topMargin.getSupported() == List<int>({1, 2, 3, 4}));

  ASSERT(ip.bottomMargin.isSupported());
  ASSERT(ip.bottomMargin.getDefault() == 2);
  ASSERT(ip.bottomMargin.getSupported() == List<int>({2, 3, 4}));

  ASSERT(ip.leftMargin.isSupported());
  ASSERT(ip.leftMargin.getDefault() == 3);
  ASSERT(ip.leftMargin.getSupported() == List<int>({3, 4}));

  ASSERT(ip.rightMargin.isSupported());
  ASSERT(ip.rightMargin.getDefault() == 4);
  ASSERT(ip.rightMargin.getSupported() == List<int>({4}));

}

TEST(ippprintjob_empty)
{
  IppAttrs printerAttrs;
  IppPrintJob ip(printerAttrs);

  ASSERT_FALSE(ip.sides.isSupported());
  ASSERT(ip.sides.getDefault() == "");
  ASSERT(ip.sides.getSupported() == List<std::string>());

  ASSERT_FALSE(ip.media.isSupported());
  ASSERT(ip.media.getDefault() == "");
  ASSERT(ip.media.getSupported() == List<std::string>());
  ASSERT(ip.media.getPreferred() == List<std::string>());

  ASSERT_FALSE(ip.copies.isSupported());
  ASSERT(ip.copies.getDefault() == 0);
  ASSERT(ip.copies.getSupportedMin() == 0);
  ASSERT(ip.copies.getSupportedMax() == 0);

  ASSERT_FALSE(ip.multipleDocumentHandling.isSupported());
  ASSERT(ip.multipleDocumentHandling.getDefault() == "");
  ASSERT(ip.multipleDocumentHandling.getSupported() == List<std::string>());

  ASSERT_FALSE(ip.pageRanges.isSupported());
  ASSERT(ip.pageRanges.getSupported() == false);

  ASSERT_FALSE(ip.numberUp.isSupported());
  ASSERT(ip.numberUp.getDefault() == 0);
  ASSERT(ip.numberUp.getSupported() == List<int>());

  ASSERT_FALSE(ip.colorMode.isSupported());
  ASSERT(ip.colorMode.getDefault() == "");
  ASSERT(ip.colorMode.getSupported() == List<std::string>());

  ASSERT_FALSE(ip.printQuality.isSupported());
  ASSERT(ip.printQuality.getDefault() == 0);
  ASSERT(ip.printQuality.getSupported() == List<int>());

  ASSERT_FALSE(ip.resolution.isSupported());
  ASSERT(ip.resolution.getDefault() == IppResolution());
  ASSERT(ip.resolution.getSupported() == List<IppResolution>());

  ASSERT_FALSE(ip.scaling.isSupported());
  ASSERT(ip.scaling.getDefault() == "");
  ASSERT(ip.scaling.getSupported() == List<std::string>());

  ASSERT_FALSE(ip.documentFormat.isSupported());
  ASSERT(ip.documentFormat.getDefault() == "");
  ASSERT(ip.documentFormat.getSupported() == List<std::string>());

  ASSERT_FALSE(ip.mediaType.isSupported());
  ASSERT(ip.mediaType.getDefault() == "");
  ASSERT(ip.mediaType.getSupported() == List<std::string>());

  ASSERT_FALSE(ip.mediaSource.isSupported());
  ASSERT(ip.mediaSource.getDefault() == "");
  ASSERT(ip.mediaSource.getSupported() == List<std::string>());

  ASSERT_FALSE(ip.outputBin.isSupported());
  ASSERT(ip.outputBin.getDefault() == "");
  ASSERT(ip.outputBin.getSupported() == List<std::string>());

  ASSERT_FALSE(ip.topMargin.isSupported());
  ASSERT(ip.topMargin.getDefault() == 0);
  ASSERT(ip.topMargin.getSupported() == List<int>());

  ASSERT_FALSE(ip.bottomMargin.isSupported());
  ASSERT(ip.bottomMargin.getDefault() == 0);
  ASSERT(ip.bottomMargin.getSupported() == List<int>());

  ASSERT_FALSE(ip.leftMargin.isSupported());
  ASSERT(ip.leftMargin.getDefault() == 0);
  ASSERT(ip.leftMargin.getSupported() == List<int>());

  ASSERT_FALSE(ip.rightMargin.isSupported());
  ASSERT(ip.rightMargin.getDefault() == 0);
  ASSERT(ip.rightMargin.getSupported() == List<int>());

}

TEST(ippprintjob_set)
{
  IppAttrs printerAttrs;
  IppPrintJob ip(printerAttrs);

  IppAttrs jobAttrs;

  ip.sides.set("two-sided");
  ASSERT(ip.sides.isSet());
  ASSERT(ip.sides.get() == "two-sided");
  jobAttrs.set("sides", IppAttr(IppTag::Keyword, "two-sided"));
  ASSERT(ip.jobAttrs == jobAttrs);

  ip.media.set("iso_a4_210x297mm");
  ASSERT(ip.media.isSet());
  ASSERT(ip.media.get() == "iso_a4_210x297mm");
  jobAttrs.set("media", IppAttr(IppTag::Keyword, "iso_a4_210x297mm"));
  ASSERT(ip.jobAttrs == jobAttrs);

  ip.copies.set(42);
  ASSERT(ip.copies.isSet());
  ASSERT(ip.copies.get() == 42);
  jobAttrs.set("copies", IppAttr(IppTag::Integer, 42));
  ASSERT(ip.jobAttrs == jobAttrs);

  ip.multipleDocumentHandling.set("separate-documents-collated-copies");
  ASSERT(ip.multipleDocumentHandling.isSet());
  ASSERT(ip.multipleDocumentHandling.get() == "separate-documents-collated-copies");
  jobAttrs.set("multiple-document-handling", IppAttr(IppTag::Keyword, "separate-documents-collated-copies"));
  ASSERT(ip.jobAttrs == jobAttrs);

  ip.pageRanges.set({IppIntRange {17, 42}});
  ASSERT(ip.pageRanges.isSet());
  ASSERT((ip.pageRanges.get() == IppOneSetOf {IppIntRange {17, 42}}));
  jobAttrs.set("page-ranges", IppAttr(IppTag::IntegerRange, IppOneSetOf {IppIntRange {17, 42}}));
  ASSERT(ip.jobAttrs == jobAttrs);

  ip.pageRanges.set({{IppIntRange {1, 2}}, {IppIntRange {17, 42}}});
  ASSERT(ip.pageRanges.isSet());
  ASSERT((ip.pageRanges.get() == IppOneSetOf {IppIntRange {1, 2}, IppIntRange {17, 42}}));
  jobAttrs.set("page-ranges", IppAttr(IppTag::IntegerRange, IppOneSetOf {IppIntRange {1, 2}, IppIntRange {17, 42}}));
  ASSERT(ip.jobAttrs == jobAttrs);

  ip.numberUp.set(8);
  ASSERT(ip.numberUp.isSet());
  ASSERT(ip.numberUp.get() == 8);
  jobAttrs.set("number-up", IppAttr(IppTag::Integer, 8));
  ASSERT(ip.jobAttrs == jobAttrs);

  ip.colorMode.set("monochrome");
  ASSERT(ip.colorMode.isSet());
  ASSERT(ip.colorMode.get() == "monochrome");
  jobAttrs.set("print-color-mode", IppAttr(IppTag::Keyword, "monochrome"));
  ASSERT(ip.jobAttrs == jobAttrs);

  ip.printQuality.set(5);
  ASSERT(ip.printQuality.isSet());
  ASSERT(ip.printQuality.get() == 5);
  jobAttrs.set("print-quality", IppAttr(IppTag::Integer, 5));
  ASSERT(ip.jobAttrs == jobAttrs);

  ip.resolution.set(IppResolution {600, 600, 3});
  ASSERT(ip.resolution.isSet());
  ASSERT((ip.resolution.get() == IppResolution {600, 600, 3}));
  jobAttrs.set("printer-resolution", IppAttr(IppTag::Resolution, IppResolution {600, 600, 3}));
  ASSERT(ip.jobAttrs == jobAttrs);

  ip.scaling.set("auto");
  ASSERT(ip.scaling.isSet());
  ASSERT(ip.scaling.get() == "auto");
  jobAttrs.set("print-scaling", IppAttr(IppTag::Keyword, "auto"));
  ASSERT(ip.jobAttrs == jobAttrs);

  ip.documentFormat.set("application/pdf");
  ASSERT(ip.documentFormat.isSet());
  ASSERT(ip.documentFormat.get() == "application/pdf");
  ASSERT((ip.opAttrs == IppAttrs {{"document-format", IppAttr(IppTag::MimeMediaType, "application/pdf")}}));
  ASSERT(ip.jobAttrs == jobAttrs);

  ip.outputBin.set("face-down");
  ASSERT(ip.outputBin.isSet());
  ASSERT(ip.outputBin.get() == "face-down");
  jobAttrs.set("output-bin", IppAttr(IppTag::Keyword, "face-down"));
  ASSERT(ip.jobAttrs == jobAttrs);

  ip.mediaType.set("stationery");
  ASSERT(ip.mediaType.isSet());
  ASSERT(ip.mediaType.get() == "stationery");
  ip.mediaSource.set("main");
  ASSERT(ip.mediaSource.isSet());
  ASSERT(ip.mediaSource.get() == "main");
  ip.topMargin.set(1);
  ASSERT(ip.topMargin.isSet());
  ASSERT(ip.topMargin.get() == 1);
  ip.bottomMargin.set(2);
  ASSERT(ip.bottomMargin.isSet());
  ASSERT(ip.bottomMargin.get() == 2);
  ip.leftMargin.set(3);
  ASSERT(ip.leftMargin.isSet());
  ASSERT(ip.leftMargin.get() == 3);
  ip.rightMargin.set(4);
  ASSERT(ip.rightMargin.isSet());
  ASSERT(ip.rightMargin.get() == 4);

  IppCollection mediaCol {{"media-type", IppAttr(IppTag::Keyword, "stationery")},
                          {"media-source", IppAttr(IppTag::Keyword, "main")},
                          {"media-top-margin", IppAttr(IppTag::Integer, 1)},
                          {"media-bottom-margin", IppAttr(IppTag::Integer, 2)},
                          {"media-left-margin", IppAttr(IppTag::Integer, 3)},
                          {"media-right-margin", IppAttr(IppTag::Integer, 4)}};

  jobAttrs.set("media-col", IppAttr(IppTag::BeginCollection, mediaCol));

  ASSERT(ip.jobAttrs == jobAttrs);

}

TEST(finalize)
{
  IppAttrs printerAttrs =
    {{"sides-default", IppAttr(IppTag::Keyword, "one-sided")},
     {"sides-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"one-sided",
                                                               "two-sided-long-edge",
                                                               "two-sided-short-edge"})},
     {"media-default", IppAttr(IppTag::Keyword, "iso_a4_210x297mm")},
     {"media-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"iso_a4_210x297mm",
                                                               "na_letter_8.5x11in"})},
     {"media-ready", IppAttr(IppTag::Keyword, "iso_a4_210x297mm")},
     {"copies-default", IppAttr(IppTag::Integer, 1)},
     {"copies-supported", IppAttr(IppTag::IntegerRange, IppIntRange {1, 999})},
     {"multiple-document-handling-default", IppAttr(IppTag::Keyword, "separate-documents-uncollated-copies")},
     {"multiple-document-handling-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"single-document",
                                                                                    "separate-documents-uncollated-copies",
                                                                                    "separate-documents-collated-copies"})},
     {"page-ranges-supported", IppAttr(IppTag::Boolean, false)},
     {"number-up-default", IppAttr(IppTag::Integer, 1)},
     {"number-up-supported", IppAttr(IppTag::Integer, IppOneSetOf{1, 2, 4})},
     {"print-color-mode-default", IppAttr(IppTag::Keyword, "auto")},
     {"print-color-mode-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"auto", "color", "monochrome"})},
     {"print-quality-default", IppAttr(IppTag::Enum, 4)},
     {"print-quality-supported", IppAttr(IppTag::Enum, IppOneSetOf {3, 4, 5})},
     {"printer-resolution-default", IppAttr(IppTag::Resolution, IppResolution {600, 600, 3})},
     {"printer-resolution-supported", IppAttr(IppTag::Resolution, IppOneSetOf {IppResolution {600, 600, 3},
                                                                               IppResolution {600, 1200, 3}})},
     {"document-format-default", IppAttr(IppTag::Keyword, "application/octet-stream")},
     {"document-format-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"application/octet-stream",
                                                                         "image/urf",
                                                                         "image/pwg-raster",
                                                                         "application/pdf"})},
     {"print-scaling-default", IppAttr(IppTag::Keyword, "auto")},
     {"print-scaling-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"auto", "fill", "fit"})},
     {"media-type-default", IppAttr(IppTag::Keyword, "stationery")},
     {"media-type-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"stationery", "cardstock", "labels"})},
     {"media-source-default", IppAttr(IppTag::Keyword, "auto")},
     {"media-source-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"auto", "envelope", "manual", "tray-1"})},
     {"output-bin-default", IppAttr(IppTag::Keyword, "face-down")},
     {"output-bin-supported", IppAttr(IppTag::Keyword, "face-down")},

     {"media-top-margin-default", IppAttr(IppTag::Integer, 1)},
     {"media-top-margin-supported", IppAttr(IppTag::Integer, IppOneSetOf {1, 2, 3, 4})},
     {"media-bottom-margin-default", IppAttr(IppTag::Integer, 2)},
     {"media-bottom-margin-supported", IppAttr(IppTag::Integer, IppOneSetOf {2, 3, 4})},
     {"media-left-margin-default", IppAttr(IppTag::Integer, 3)},
     {"media-left-margin-supported", IppAttr(IppTag::Integer, IppOneSetOf {3, 4})},
     {"media-right-margin-default", IppAttr(IppTag::Integer, 4)},
     {"media-right-margin-supported", IppAttr(IppTag::Integer, 4)},

     {"pwg-raster-document-resolution-supported", IppAttr(IppTag::Resolution, IppOneSetOf {IppResolution {300, 300, 3},
                                                                                           IppResolution {600, 600, 3}})},
     {"pwg-raster-document-sheet-back", IppAttr(IppTag::Keyword, "flipped")},
     {"document-format-varying-attributes", IppAttr(IppTag::Keyword, "copies-supported")},
     {"urf-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"RS300-600", "DM3"})},

    };

  IppPrintJob ip(printerAttrs);

  // Margins - to be unset/ignored for PDF derivatives
  ip.topMargin.set(1);
  ip.bottomMargin.set(2);
  ip.leftMargin.set(3);
  ip.rightMargin.set(4);

  // To propagate to printParams
  ip.resolution.set(IppResolution {600, 600, 3});

  // To be kept in media since media-col is emptied when margins are removed
  ip.media.set("na_letter_8.5x11in");

  // To be removed in jobAttrs, but set in jobAttrs
  ip.pageRanges.set({IppIntRange {17, 42}});

  // To be kept and set in printParams
  ip.sides.set("two-sided-long-edge");
  ip.printQuality.set(5);
  ip.colorMode.set("monochrome");

  ip.finalize("application/pdf");

  ASSERT((ip.opAttrs == IppAttrs {{"document-format", IppAttr(IppTag::MimeMediaType, "application/pdf")}}));
  ASSERT((ip.jobAttrs == IppAttrs {{"printer-resolution", IppAttr(IppTag::Resolution, IppResolution {600, 600, 3})},
                                   {"media", IppAttr(IppTag::Keyword, "na_letter_8.5x11in")},
                                   {"sides", IppAttr(IppTag::Keyword, "two-sided-long-edge")},
                                   {"print-quality", IppAttr(IppTag::Enum, 5)},
                                   {"print-color-mode", IppAttr(IppTag::Keyword, "monochrome")}}));
  ASSERT(ip.printParams.format == PrintParameters::PDF);
  ASSERT(ip.printParams.paperSizeName == "na_letter_8.5x11in");
  ASSERT(ip.printParams.hwResW == 600);
  ASSERT(ip.printParams.hwResH == 600);
  ASSERT((ip.printParams.pageRangeList == PageRangeList {{17, 42}}));
  ASSERT(ip.printParams.duplexMode == PrintParameters::TwoSidedLongEdge);
  ASSERT(ip.printParams.quality == PrintParameters::HighQuality);
  ASSERT(ip.printParams.colorMode == PrintParameters::Gray8);
  ASSERT(ip.compression.get() == "");

  // Forget choices
  ip = IppPrintJob(printerAttrs);

  // With some random media-col setting, media-size moves to media-col too
  ip.media.set("iso_a4_210x297mm");
  ip.mediaSource.set("tray-1");
  ip.mediaType.set("cardstock");

  ip.finalize("application/pdf");

  ASSERT((ip.opAttrs == IppAttrs {{"document-format", IppAttr(IppTag::MimeMediaType, "application/pdf")}}));
  IppCollection dimensions {{"x-dimension", IppAttr(IppTag::Integer, 21000)},
                            {"y-dimension", IppAttr(IppTag::Integer, 29700)}};
  IppCollection mediaCol {{"media-type", IppAttr(IppTag::Keyword, "cardstock")},
                          {"media-size", IppAttr(IppTag::BeginCollection, dimensions)},
                          {"media-source", IppAttr(IppTag::Keyword, "tray-1")}};
  ASSERT((ip.jobAttrs == IppAttrs {{"media-col", IppAttr(IppTag::BeginCollection, mediaCol)}}));

  ASSERT(ip.printParams.paperSizeName == "iso_a4_210x297mm");
  ASSERT(ip.printParams.mediaPosition == PrintParameters::Tray1);
  ASSERT(ip.printParams.mediaType == "cardstock");

  // Remove PDF support, forget choices
  printerAttrs.set("document-format-supported",
                   IppAttr(IppTag::Keyword, IppOneSetOf {"application/octet-stream",
                                                         "image/urf",
                                                         "image/pwg-raster"}));
  ip = IppPrintJob(printerAttrs);

  // To be removed and clamped to 600x600 in printParams
  ip.resolution.set(IppResolution {600, 1200, 3});

  ip.copies.set(2);

  ip.finalize("application/pdf");

  ASSERT((ip.opAttrs == IppAttrs {{"document-format", IppAttr(IppTag::MimeMediaType, "image/pwg-raster")}}));
  ASSERT((ip.jobAttrs == IppAttrs {}));
  ASSERT(ip.printParams.format == PrintParameters::PWG);
  ASSERT(ip.printParams.hwResW == 600);
  ASSERT(ip.printParams.hwResH == 600);
  ASSERT(ip.printParams.backXformMode == PrintParameters::Flipped);
  ASSERT(ip.printParams.copies == 2);

  // Forget and do almost the same again, but with URF selected
  ip = IppPrintJob(printerAttrs);
  ip.documentFormat.set("image/urf");

  // To be removed and clamped to 600x600 in printParams
  ip.resolution.set(IppResolution {600, 1200, 3});

  ip.copies.set(2);

  ip.finalize("application/pdf");

  ASSERT((ip.opAttrs == IppAttrs {{"document-format", IppAttr(IppTag::MimeMediaType, "image/urf")}}));
  ASSERT((ip.jobAttrs == IppAttrs {}));
  ASSERT(ip.printParams.format == PrintParameters::URF);
  ASSERT(ip.printParams.hwResW == 600);
  ASSERT(ip.printParams.hwResH == 600);
  ASSERT(ip.printParams.backXformMode == PrintParameters::Rotated);
  ASSERT(ip.printParams.copies == 2);
  ASSERT(ip.compression.get() == "");

  // Add Postscript, forget settings
  printerAttrs.set("document-format-supported",
                   IppAttr(IppTag::Keyword, IppOneSetOf {"application/octet-stream",
                                                         "application/postscript",
                                                         "image/urf",
                                                         "image/pwg-raster"}));
  ip = IppPrintJob(printerAttrs);

  // Margins - to be unset/ignored for Postscript
  ip.topMargin.set(1);
  ip.bottomMargin.set(2);
  ip.leftMargin.set(3);
  ip.rightMargin.set(4);

  ip.resolution.set(IppResolution {600, 600, 3});
  // To be kept for Postscript
  ip.pageRanges.set({IppIntRange {17, 42}});

  ip.finalize("application/postscript");

  ASSERT((ip.opAttrs == IppAttrs {{"document-format", IppAttr(IppTag::MimeMediaType, "application/postscript")}}));
  ASSERT((ip.jobAttrs == IppAttrs {{"printer-resolution", IppAttr(IppTag::Resolution, IppResolution {600, 600, 3})},
                                   {"page-ranges", IppAttr(IppTag::IntegerRange, IppOneSetOf {IppIntRange {17, 42}})}}));
  // PrintParameters only used for PDF input
  ASSERT(ip.printParams.format == PrintParameters::Invalid);

  // Add JPEG, forget settings
  printerAttrs.set("document-format-supported",
                   IppAttr(IppTag::Keyword, IppOneSetOf {"application/octet-stream",
                                                         "image/jpeg",
                                                         "image/urf",
                                                         "image/pwg-raster"}));
  ip = IppPrintJob(printerAttrs);

  // Margins - to be kept for images
  ip.topMargin.set(1);
  ip.bottomMargin.set(2);
  ip.leftMargin.set(3);
  ip.rightMargin.set(4);

  ip.resolution.set(IppResolution {600, 600, 3});

  ip.finalize("image/jpeg");

  ASSERT((ip.opAttrs == IppAttrs {{"document-format", IppAttr(IppTag::MimeMediaType, "image/jpeg")}}));
  IppCollection mediaCol2 {{"media-top-margin", IppAttr(IppTag::Integer, 1)},
                           {"media-bottom-margin", IppAttr(IppTag::Integer, 2)},
                           {"media-left-margin", IppAttr(IppTag::Integer, 3)},
                           {"media-right-margin", IppAttr(IppTag::Integer, 4)}};
  ASSERT((ip.jobAttrs == IppAttrs {{"printer-resolution", IppAttr(IppTag::Resolution, IppResolution {600, 600, 3})},
                                   {"media-col", IppAttr(IppTag::BeginCollection, mediaCol2)}}));
  // PrintParameters only used for PDF input
  ASSERT(ip.printParams.format == PrintParameters::Invalid);

  // Forget choices
  ip = IppPrintJob(printerAttrs);

  // Margins - to be unset/ignored for rasters
  ip.topMargin.set(1);
  ip.bottomMargin.set(2);
  ip.leftMargin.set(3);
  ip.rightMargin.set(4);

  // To be changed for single-page document doing copies
  ip.sides.set("two-sided-long-edge");
  ip.copies.set(2);

  ip.finalize("application/pdf", 1);

  ASSERT((ip.opAttrs == IppAttrs {{"document-format", IppAttr(IppTag::MimeMediaType, "image/pwg-raster")}}));
  ASSERT((ip.jobAttrs == IppAttrs {{"sides", IppAttr(IppTag::Keyword, "one-sided")}}));
  // Not an image taregt format - margins removed
  IppCollection mediaCol3 = ip.jobAttrs.get<IppCollection>("media-col");
  ASSERT_FALSE(mediaCol3.has("media-top-margin"));
  ASSERT_FALSE(mediaCol3.has("media-bottom-margin"));
  ASSERT_FALSE(mediaCol3.has("media-left-margin"));
  ASSERT_FALSE(mediaCol3.has("media-right-margin"));
  ASSERT(ip.margins.top == 1);
  ASSERT(ip.margins.bottom == 2);
  ASSERT(ip.margins.left == 3);
  ASSERT(ip.margins.right == 4);
  ASSERT(ip.printParams.format == PrintParameters::PWG);
  ASSERT(ip.printParams.colorMode == PrintParameters::sRGB24);

  // Forget choices
  ip = IppPrintJob(printerAttrs);

  // To be changed for single-page selection doing copies
  ip.sides.set("two-sided-long-edge");
  ip.copies.set(2);
  ip.pageRanges.set({IppIntRange {17, 17}});

  ip.finalize("application/pdf", 42);
  ASSERT((ip.opAttrs == IppAttrs {{"document-format", IppAttr(IppTag::MimeMediaType, "image/pwg-raster")}}));
  ASSERT((ip.jobAttrs == IppAttrs {{"sides", IppAttr(IppTag::Keyword, "one-sided")}}));

  // Un-support color, forget choices
  printerAttrs.set("print-color-mode-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"auto", "monochrome"}));
  ip = IppPrintJob(printerAttrs);
  ip.finalize("application/pdf", 0);

  ASSERT((ip.opAttrs == IppAttrs {{"document-format", IppAttr(IppTag::MimeMediaType, "image/pwg-raster")}}));
  ASSERT(ip.printParams.format == PrintParameters::PWG);
  ASSERT(ip.printParams.colorMode == PrintParameters::Gray8); // Since we don't support color

  // Support compression, forget choices
  printerAttrs.set("compression-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"none", "deflate", "gzip"}));
  ip = IppPrintJob(printerAttrs);
  ip.finalize("application/pdf", 0);
  // gzip has higher priority
  ASSERT(ip.compression.get() == "gzip");

}

TEST(additional_formats)
{
  IppAttrs printerAttrs;
  IppPrinter ip(printerAttrs);
  ASSERT(ip.additionalDocumentFormats() == List<std::string>());

  printerAttrs.set("printer-device-id", IppAttr(IppTag::TextWithoutLanguage, "MANUFACTURER:Xerox;COMMAND SET:PDF,URF,JPEG;MODEL:WorkCentre 6515;URF:IS4-20,IFU20,OB10,CP255,DM1,MT1,PQ4-5,RS600,W8,SRGB24,V1.4,FN3"));
  ip = IppPrinter(printerAttrs);
  ASSERT(ip.additionalDocumentFormats() == List<std::string>({"application/pdf", "image/urf"}));

  printerAttrs.set("printer-device-id", IppAttr(IppTag::TextWithoutLanguage, "MFG:Canon;CMD:URF;MDL:SELPHY CP1300;CLS:PRINTER;URF:W8,SRGB24,V1.4,RS300,IS7,MT11,PQ4,OB9,IFU0,OFU0,CP99;"));
  ip = IppPrinter(printerAttrs);
  ASSERT(ip.additionalDocumentFormats() == List<std::string>({"image/urf"}));

  printerAttrs.set("printer-device-id", IppAttr(IppTag::TextWithoutLanguage, "MANUFACTURER:Xerox;COMMAND SET:PCL 6 Emulation, PostScript Level 3 Emulation, URF, PWG, NPAP, PJL;MODEL:C230 Color Printer;CLS:PRINTER;DES:Xerox(R) C230 Color Printer;CID:XR_PCL6_XCPT_Color_A4;COMMENT:ECP1.0, LV_0924, LP_9D15, LF_00C7;"));
  ip = IppPrinter(printerAttrs);
  ASSERT(ip.additionalDocumentFormats() == List<std::string>({"application/postscript", "image/pwg-raster", "image/urf"}));

  printerAttrs.set("printer-device-id", IppAttr(IppTag::TextWithoutLanguage, "MFG:EPSON;CMD:ESCPL2,BDC,D4,D4PX,ESCPR7,END4,GENEP,PWGRaster,URF;MDL:SC-P900 Series;CLS:PRINTER;DES:EPSON SC-P900 Series;CID:EpsonRGB;RID:02;DDS:401900;ELG:13F0;SN:583757503030333448;URF:CP1,PQ3-4-5,OB9,OFU0,RS360-720,SRGB24,ADOBERGB24-48,W8,IS18-20-21-22,V1.4,MT1-10-11;"));
  ip = IppPrinter(printerAttrs);
  ASSERT(ip.additionalDocumentFormats() == List<std::string>({"image/pwg-raster", "image/urf"}));

  printerAttrs.set("printer-device-id", IppAttr(IppTag::TextWithoutLanguage, "MFG:HP;CMD:PJL,PML,PWG_RASTER,URP;MDL:HP LaserJet MFP M28-M31;CLS:PRINTER;DES:HP LaserJet MFP M28w;MEM:MEM=279MB;PRN:W2G55A;S:0300000000000000000000000000000000;COMMENT:RES=600x2;LEDMDIS:USB#ff#04#01;CID:HPLJPCLMSMV1;MCT:MF;MCL:FL;MCV:2.0;"));
  ip = IppPrinter(printerAttrs);
  ASSERT(ip.additionalDocumentFormats() == List<std::string>({"image/pwg-raster"}));

  printerAttrs.set("printer-device-id", IppAttr(IppTag::TextWithoutLanguage, "MFG:HP;MDL:DeskJet 3700 series;CMD:PCL3GUI,PJL,Automatic,JPEG,PCLM,AppleRaster,PWGRaster,DW-PCL,802.11,DESKJET,DYN;CLS:PRINTER;DES:T8X04B;CID:HPDeskjet_P976D;LEDMDIS:USB#FF#CC#00,USB#07#01#02,USB#FF#04#01;SN:CN69R3D24J06H3;S:038000C480a00001002c0000000c1400046;Z:05000001000008,12000,17000000000000,181;"));
  ip = IppPrinter(printerAttrs);
  ASSERT(ip.additionalDocumentFormats() == List<std::string>({"image/pwg-raster", "image/urf"}));

  printerAttrs = IppAttrs();
  ip = IppPrinter(printerAttrs);
  ASSERT(ip.additionalDocumentFormats() == List<std::string>());

  printerAttrs.set("document-format-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"application/octet-stream",
                                                                                      "application/postscript",
                                                                                      "image/urf"}));
  printerAttrs.set("printer-device-id", IppAttr(IppTag::TextWithoutLanguage, "MFG:OKI DATA c332.js-CORP;CMD:PCL,XPS,IBMPPR,EPSONFX,POSTSCRIPT,PDF,PCLXL,HIPERMIP,URF;MDL:C332;CLS:PRINTER;DES:OKI c332.js-C332;CID:OK_N_B800;"));
  ip = IppPrinter(printerAttrs);
  ASSERT(ip.additionalDocumentFormats() == List<std::string>({"application/pdf"}));

}

TEST(json)
{
  IppAttrs printerAttrs =
    {{"sides-default", IppAttr(IppTag::Keyword, "one-sided")},
     {"sides-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"one-sided",
                                                               "two-sided-long-edge",
                                                               "two-sided-short-edge"})},
     {"media-default", IppAttr(IppTag::Keyword, "iso_a4_210x297mm")},
     {"media-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"iso_a4_210x297mm",
                                                               "na_letter_8.5x11in"})},
     {"media-ready", IppAttr(IppTag::Keyword, "iso_a4_210x297mm")},
     {"copies-default", IppAttr(IppTag::Integer, 1)},
     {"copies-supported", IppAttr(IppTag::IntegerRange, IppIntRange {1, 999})},
     {"multiple-document-handling-default", IppAttr(IppTag::Keyword, "separate-documents-uncollated-copies")},
     {"multiple-document-handling-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"single-document",
                                                                                    "separate-documents-uncollated-copies",
                                                                                    "separate-documents-collated-copies"})},
     {"page-ranges-supported", IppAttr(IppTag::Boolean, false)},
     {"number-up-default", IppAttr(IppTag::Integer, 1)},
     {"number-up-supported", IppAttr(IppTag::Integer, IppOneSetOf{1, 2, 4})},
     {"print-color-mode-default", IppAttr(IppTag::Keyword, "auto")},
     {"print-color-mode-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"auto", "color", "monochrome"})},
     {"print-quality-default", IppAttr(IppTag::Enum, 4)},
     {"print-quality-supported", IppAttr(IppTag::Enum, IppOneSetOf {3, 4, 5})},
     {"printer-resolution-default", IppAttr(IppTag::Resolution, IppResolution {600, 600, 3})},
     {"printer-resolution-supported", IppAttr(IppTag::Resolution, IppOneSetOf {IppResolution {600, 600, 3},
                                                                               IppResolution {600, 1200, 3}})},
     {"document-format-default", IppAttr(IppTag::Keyword, "application/octet-stream")},
     {"document-format-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"application/octet-stream",
                                                                         "image/urf",
                                                                         "image/pwg-raster",
                                                                         "application/pdf"})},
     {"print-scaling-default", IppAttr(IppTag::Keyword, "auto")},
     {"print-scaling-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"auto", "fill", "fit"})},
     {"media-type-default", IppAttr(IppTag::Keyword, "stationery")},
     {"media-type-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"stationery", "cardstock", "labels"})},
     {"media-source-default", IppAttr(IppTag::Keyword, "auto")},
     {"media-source-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"auto", "envelope", "manual", "tray-1"})},
     {"output-bin-default", IppAttr(IppTag::Keyword, "face-down")},
     {"output-bin-supported", IppAttr(IppTag::Keyword, "face-down")},

     {"media-top-margin-default", IppAttr(IppTag::Integer, 1)},
     {"media-top-margin-supported", IppAttr(IppTag::Integer, IppOneSetOf {1, 2, 3, 4})},
     {"media-bottom-margin-default", IppAttr(IppTag::Integer, 2)},
     {"media-bottom-margin-supported", IppAttr(IppTag::Integer, IppOneSetOf {2, 3, 4})},
     {"media-left-margin-default", IppAttr(IppTag::Integer, 3)},
     {"media-left-margin-supported", IppAttr(IppTag::Integer, IppOneSetOf {3, 4})},
     {"media-right-margin-default", IppAttr(IppTag::Integer, 4)},
     {"media-right-margin-supported", IppAttr(IppTag::Integer, 4)},

     {"pwg-raster-document-resolution-supported", IppAttr(IppTag::Resolution, IppOneSetOf {IppResolution {300, 300, 3},
                                                                                           IppResolution {600, 600, 3}})},
     {"pwg-raster-document-sheet-back", IppAttr(IppTag::Keyword, "flipped")},
     {"document-format-varying-attributes", IppAttr(IppTag::Keyword, "copies-supported")},
     {"urf-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"RS300-600", "DM3"})},

    };
  Json json = printerAttrs.toJSON();
  ASSERT(printerAttrs == IppAttrs::fromJSON(json.object_items()));
}

TEST(attribute_getters)
{
  IppAttrs printerAttrs;
  IppPrinter ip(printerAttrs);

  ASSERT(ip.name() == "");
  ASSERT(ip.uuid() == "");
  ASSERT(ip.makeAndModel() == "");
  ASSERT(ip.location() == "");
  ASSERT(ip.state() == 0);
  ASSERT(ip.stateMessage() == "");
  ASSERT(ip.stateReasons().empty());
  ASSERT(ip.ippVersionsSupported().empty());
  ASSERT(ip.ippFeaturesSupported().empty());
  ASSERT(ip.pagesPerMinute() == 0);
  ASSERT(ip.pagesPerMinuteColor() == 0);
  ASSERT(ip.identifySupported() == false);
  ASSERT(ip.supplies().empty());
  ASSERT(ip.firmware().empty());
  ASSERT(ip.settableAttributes().empty());
  ASSERT(ip.documentFormats().empty());
  ASSERT(ip.supportsPrinterRaster() == false);

  IppPrintJob job = ip.createJob();
  ASSERT(job.canSaveSettings() == false);

  printerAttrs =
    {{"printer-name", IppAttr(IppTag::NameWithLanguage, "Printer Name")},
     {"printer-uuid", IppAttr(IppTag::Uri, "urn:uuid:deadbeef")},
     {"printer-make-and-model", IppAttr(IppTag::TextWithoutLanguage, "Printer Make and Model")},
     {"printer-location", IppAttr(IppTag::TextWithoutLanguage, "Printer Location")},
     {"printer-state", IppAttr(IppTag::Enum, 3)},
     {"printer-state-message", IppAttr(IppTag::TextWithoutLanguage, "State.")},
     {"printer-state-reasons", IppAttr(IppTag::Keyword, "none")},
     {"ipp-versions-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"1.1", "2.0"})},
     {"ipp-features-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"ipp-everywhere"})},
     {"pages-per-minute", IppAttr(IppTag::Integer, 42)},
     {"pages-per-minute-color", IppAttr(IppTag::Integer, 17)},
     {"identify-actions-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"display", "sound"})},

     {"marker-names", IppAttr(IppTag::NameWithoutLanguage, IppOneSetOf {"Supply 1", "Supply 2", "Supply 3"})},
     {"marker-types", IppAttr(IppTag::Keyword, IppOneSetOf {"toner", "opc", "ink-cartridge"})},
     {"marker-colors", IppAttr(IppTag::NameWithoutLanguage, IppOneSetOf {"#FFFFFF", "none", "#00FFFF#FF00FF#FFFF00"})},
     {"marker-levels", IppAttr(IppTag::Integer, IppOneSetOf {17, 42, 69})},
     {"marker-low-levels", IppAttr(IppTag::Integer, IppOneSetOf {20, 20, 10})},
     {"marker-high-levels", IppAttr(IppTag::Integer, IppOneSetOf {100, 100, 100})},

     {"printer-firmware-name", IppAttr(IppTag::NameWithoutLanguage, IppOneSetOf {"Firmware 1", "Firmware 2"})},
     {"printer-firmware-string-version", IppAttr(IppTag::TextWithoutLanguage, IppOneSetOf {"1.0", "2.0"})},
     {"printer-settable-attributes-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"printer-name", "printer-location"})},
     {"document-format-supported", IppAttr(IppTag::Keyword, IppOneSetOf {"application/octet-stream",
                                                                         "image/urf",
                                                                         "image/pwg-raster",
                                                                         "application/pdf"})},
    };

  ip = IppPrinter(printerAttrs);

  ASSERT(ip.name() == "Printer Name");
  ASSERT(ip.uuid() == "urn:uuid:deadbeef");
  ASSERT(ip.makeAndModel() == "Printer Make and Model");
  ASSERT(ip.location() == "Printer Location");
  ASSERT(ip.state() == 3);
  ASSERT(ip.stateMessage() == "State.");
  ASSERT(ip.stateReasons() == List<std::string> {"none"});
  ASSERT_FALSE(ip.isWarningState());
  ASSERT(ip.ippVersionsSupported() == (List<std::string> {"1.1", "2.0"}));
  ASSERT(ip.ippFeaturesSupported() == List<std::string> {"ipp-everywhere"});
  ASSERT(ip.pagesPerMinute() == 42);
  ASSERT(ip.pagesPerMinuteColor() == 17);
  ASSERT(ip.identifySupported());
  ASSERT(ip.supplies().size() == 3);
  ASSERT(ip.firmware() == (List<IppPrinter::Firmware> {{"Firmware 1", "1.0"}, {"Firmware 2", "2.0"}}));
  ASSERT(ip.settableAttributes() == (List<std::string> {"printer-name", "printer-location"}));
  ASSERT(ip.documentFormats() == (List<std::string> {"application/octet-stream",
                                                     "image/urf",
                                                     "image/pwg-raster",
                                                     "application/pdf"}));
  ASSERT(ip.supportsPrinterRaster());

  List<IppPrinter::Supply> Supplies = ip.supplies();
  IppPrinter::Supply Supply1 = Supplies.takeFront();
  IppPrinter::Supply Supply2 = Supplies.takeFront();
  IppPrinter::Supply Supply3 = Supplies.takeFront();

  ASSERT(Supply1.name == "Supply 1");
  ASSERT(Supply1.type == "toner");
  ASSERT(Supply1.colors == List<std::string> {"#FFFFFF"});
  ASSERT(Supply1.level == 17);
  ASSERT(Supply1.lowLevel == 20);
  ASSERT(Supply1.highLevel == 100);
  ASSERT(Supply1.getPercent() == 17);
  ASSERT(Supply1.isLow());

  ASSERT(Supply2.name == "Supply 2");
  ASSERT(Supply2.type == "opc");
  ASSERT(Supply2.colors == List<std::string> {"none"});
  ASSERT(Supply2.level == 42);
  ASSERT(Supply2.lowLevel == 20);
  ASSERT(Supply2.highLevel == 100);
  ASSERT(Supply2.getPercent() == 42);
  ASSERT(Supply2.isLow() == false);

  ASSERT(Supply3.name == "Supply 3");
  ASSERT(Supply3.type == "ink-cartridge");
  ASSERT(Supply3.colors == (List<std::string> {"#00FFFF", "#FF00FF", "#FFFF00"}));
  ASSERT(Supply3.level == 69);
  ASSERT(Supply3.lowLevel == 10);
  ASSERT(Supply3.highLevel == 100);
  ASSERT(Supply3.getPercent() == 69);
  ASSERT(Supply3.isLow() == false);

  job = ip.createJob();
  ASSERT(job.canSaveSettings());
}

TEST(converter)
{
  List<std::string> supportedFormats;
  ASSERT(Converter::instance().possibleInputFormats(supportedFormats) == List<std::string> {});

  // PDF to PDF
  supportedFormats = {"application/pdf"};
  ASSERT(Converter::instance().possibleInputFormats(supportedFormats)
         == List<std::string> {"application/pdf"});
  ASSERT(Converter::instance().getTargetFormat("application/pdf", supportedFormats)
         == "application/pdf");

  // PDF maps to PWG-raster
  supportedFormats = {"image/pwg-raster"};
  ASSERT(Converter::instance().possibleInputFormats(supportedFormats)
         == (List<std::string> {"application/pdf", "image/pwg-raster"}));
  ASSERT(Converter::instance().getTargetFormat("application/pdf", supportedFormats)
         == "image/pwg-raster");

  // ...and URF
  supportedFormats = {"image/urf"};
  ASSERT(Converter::instance().possibleInputFormats(supportedFormats)
         == (List<std::string> {"application/pdf", "image/urf"}));
  ASSERT(Converter::instance().getTargetFormat("application/pdf", supportedFormats)
         == "image/urf");

  // PDF has higher prio than raster
  supportedFormats = {"image/pwg-raster", "application/pdf"};
  ASSERT(Converter::instance().possibleInputFormats(supportedFormats)
         == (List<std::string> {"application/pdf", "image/pwg-raster"}));
  ASSERT(Converter::instance().getTargetFormat("application/pdf", supportedFormats)
         == "application/pdf");

  // Octet Stream, because why not
  supportedFormats = {"application/octet-stream", "image/pwg-raster", "application/pdf"};
  ASSERT(Converter::instance().possibleInputFormats(supportedFormats)
         == (List<std::string> {"application/pdf", "application/octet-stream", "image/pwg-raster"}));
  ASSERT(Converter::instance().getTargetFormat("application/pdf", supportedFormats)
         == "application/pdf");

  // PDF has higher prio than Postscript too
  supportedFormats = {"application/postscript", "application/pdf"};
  ASSERT(Converter::instance().possibleInputFormats(supportedFormats)
         == (List<std::string> {"application/pdf", "application/postscript"}));
  ASSERT(Converter::instance().getTargetFormat("application/pdf", supportedFormats)
         == "application/pdf");

  // Postscript has higher prio than raster
  supportedFormats = {"image/pwg-raster", "application/postscript"};
  ASSERT(Converter::instance().possibleInputFormats(supportedFormats)
         == (List<std::string> {"application/pdf", "image/pwg-raster", "application/postscript"}));
  ASSERT(Converter::instance().getTargetFormat("application/pdf", supportedFormats)
         == "application/postscript");

  // JPEG needs JPEG support
  ASSERT_FALSE(Converter::instance().getTargetFormat("image/jpeg", supportedFormats));
  supportedFormats = {"application/octet-stream", "image/pwg-raster", "application/pdf", "image/jpeg"};
  ASSERT(Converter::instance().getTargetFormat("image/jpeg", supportedFormats) == "image/jpeg");

  // Functions exist. TODO: Check that they are the correct ones.
  ASSERT(Converter::instance().getConvertFun("application/pdf", "application/pdf"));
  ASSERT(Converter::instance().getConvertFun("application/pdf", "image/pwg-raster"));
  ASSERT(Converter::instance().getConvertFun("application/pdf", "image/urf"));
  ASSERT(Converter::instance().getConvertFun("image/jpeg", "image/jpeg"));
  ASSERT(Converter::instance().getConvertFun("text/plain", "text/plain"));
  ASSERT(Converter::instance().getConvertFun("foo", "foo"));

  // Custom function
  ASSERT_FALSE(Converter::instance().getConvertFun("foo", "bar"));
  Converter::ConvertFun FooBar = [](std::string, WriteFun, const IppPrintJob&, ProgressFun){return Error();};
  Converter::instance().Pipelines.push_back({{"foo", "bar"}, FooBar});
  ASSERT(Converter::instance().getConvertFun("foo", "bar"));


  // PDF has higher prio than custom format
  Converter::instance().Pipelines.push_back({{"application/pdf", "application/aaa"}, FooBar});
  supportedFormats = {"image/pwg-raster", "application/pdf", "application/aaa"};
  ASSERT(Converter::instance().possibleInputFormats(supportedFormats)
         == (List<std::string> {"application/pdf", "image/pwg-raster", "application/aaa"}));
  ASSERT(Converter::instance().getTargetFormat("application/pdf", supportedFormats)
         == "application/pdf");

  // ...but will do in a pinch
  supportedFormats = {"application/aaa"};
  ASSERT(Converter::instance().getTargetFormat("application/pdf", supportedFormats)
         == "application/aaa");

  // Higher prio custom convert function
  Converter::instance().Pipelines.push_front({{"application/pdf", "application/bbb"}, FooBar});
  supportedFormats = {"application/aaa", "application/bbb", "application/pdf"};
  ASSERT(Converter::instance().getTargetFormat("application/pdf", supportedFormats)
         == "application/bbb");

}

extern List<std::string> get_addr(Bytestream& bts, std::set<uint16_t> seenReferences={});

TEST(malicious_dns)
{
  Bytestream bts;
  ASSERT_THROW(get_addr(bts), out_of_range);

  // Address with a c-reference loop tr.ol.ol.ol...
  bts << (uint8_t)2 << "tr" << (uint8_t)2 << "ol" << (uint16_t)0xc003;

  ASSERT_THROW(get_addr(bts), logic_error);

}
