#include "bytestream.h"
#include "test.h"
#include "subprocess.hpp"
#include "PwgPgHdr.h"
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

  std::ifstream ppm_ifs("pacman.ppm", std::ios::in | std::ios::binary | std::ios::ate);
  std::ifstream::pos_type ppmSize = ppm_ifs.tellg();
  ppm_ifs.seekg(0, std::ios::beg);
  Bytestream expected_ppm(ppmSize);
  ppm_ifs.read((char*)(expected_ppm.raw()), ppmSize);

  // For sanity, make sure the one on disk is the same as created above
  ASSERT(ppm == expected_ppm);

  subprocess::popen ppm2pwg("../ppm2pwg", {});
  ppm2pwg.stdin().write((char*)ppm.raw(), ppm.size());
  ppm2pwg.close();

  ASSERT(ppm2pwg.wait() == 0);

  stringstream ss;
  ss << ppm2pwg.stdout().rdbuf();

  PwgPgHdr hdr;
  Bytestream pwg(ss.str().c_str(), ss.str().size());

  ppm2pwg.stdout().rdbuf()->pubseekpos(0);
  ppm2pwg.stdout().rdbuf()->sgetn((char*)pwg.raw(), pwg.size());

  ASSERT(pwg >>= "RaS2");
  hdr.decode_from(pwg);

  Bytestream enc = RightSideUp();

  ASSERT(pwg >>= enc);
  ASSERT(pwg.atEnd());

  std::ifstream ifs("pacman.pwg", std::ios::in | std::ios::binary | std::ios::ate);
  std::ifstream::pos_type pwgSize = ifs.tellg();
  ifs.seekg(0, std::ios::beg);
  Bytestream expected_pwg(pwgSize);
  ifs.read((char*)(expected_pwg.raw()), pwgSize);

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

  stringstream ss;
  ss << ppm2pwg.stdout().rdbuf();

  PwgPgHdr hdr1, hdr2;
  Bytestream pwg(ss.str().c_str(), ss.str().size());

  ppm2pwg.stdout().rdbuf()->pubseekpos(0);
  ppm2pwg.stdout().rdbuf()->sgetn((char*)pwg.raw(), pwg.size());

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

  stringstream ss;
  ss << ppm2pwg.stdout().rdbuf();

  PwgPgHdr hdr1, hdr2;
  Bytestream pwg(ss.str().c_str(), ss.str().size());

  ppm2pwg.stdout().rdbuf()->pubseekpos(0);
  ppm2pwg.stdout().rdbuf()->sgetn((char*)pwg.raw(), pwg.size());

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

  stringstream ss;
  ss << ppm2pwg.stdout().rdbuf();

  PwgPgHdr hdr1, hdr2;
  Bytestream pwg(ss.str().c_str(), ss.str().size());

  ppm2pwg.stdout().rdbuf()->pubseekpos(0);
  ppm2pwg.stdout().rdbuf()->sgetn((char*)pwg.raw(), pwg.size());

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

  stringstream ss;
  ss << ppm2pwg.stdout().rdbuf();

  PwgPgHdr hdr1, hdr2;
  Bytestream pwg(ss.str().c_str(), ss.str().size());

  ppm2pwg.stdout().rdbuf()->pubseekpos(0);
  ppm2pwg.stdout().rdbuf()->sgetn((char*)pwg.raw(), pwg.size());

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
