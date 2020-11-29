#include "bytestream.h"
#include "test.h"
#include "subprocess.hpp"
#include "PwgPgHdr.h"
#include <cstring>
using namespace std;

#define REPEAT(N) (uint8_t)(N-1)
#define VERBATIM(N) (uint8_t)(257-N)

TEST(ppm2pwg)
{
  Bytestream W {(uint8_t)255, (uint8_t)255, (uint8_t)255};
  Bytestream R {(uint8_t)255, (uint8_t)0,   (uint8_t)0};
  Bytestream G {(uint8_t)0,   (uint8_t)255, (uint8_t)0};
  Bytestream B {(uint8_t)0,   (uint8_t)0,   (uint8_t)255};
  Bytestream Y {(uint8_t)255, (uint8_t)255, (uint8_t)0};
  Bytestream ppm {string("P6\n8 8\n255\n")};
  ppm << W << Y << Y << Y << W << W << W << W
      << Y << B << Y << W << W << W << G << W
      << Y << Y << W << W << W << G << G << G
      << Y << Y << Y << W << W << W << G << W
      << W << Y << Y << Y << W << W << W << W
      << W << W << W << W << W << W << W << W
      << R << R << R << R << R << R << R << R
      << R << R << R << R << R << R << R << R;

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

  stringstream ss;
  ss << ppm2pwg.stdout().rdbuf();

  PwgPgHdr hdr;
  Bytestream pwg(ss.str().c_str(), ss.str().size());

  ppm2pwg.stdout().rdbuf()->pubseekpos(0);
  ppm2pwg.stdout().rdbuf()->sgetn((char*)pwg.raw(), pwg.size());

  ASSERT(pwg >>= "RaS2");
  hdr.decode_from(pwg);

  Bytestream enc;
  enc << (uint8_t)0 << REPEAT(1) << W << REPEAT(3) << Y << REPEAT(4) << W
      << (uint8_t)0 << VERBATIM(3) << Y << B << Y << REPEAT(3) << W
                    << VERBATIM(2) << G << W
      << (uint8_t)0 << REPEAT(2) << Y << REPEAT(3) << W << REPEAT(3) << G
      << (uint8_t)0 << REPEAT(3) << Y << REPEAT(3) << W << VERBATIM(2) << G << W
      << (uint8_t)0 << REPEAT(1) << W << REPEAT(3) << Y << REPEAT(4) << W
      << (uint8_t)0 << REPEAT(8) << W
      << (uint8_t)1 << REPEAT(8) << R;

  ASSERT(pwg >>= enc);
  ASSERT(pwg.atEnd());

  std::ifstream ifs("pacman.pwg", std::ios::in | std::ios::binary | std::ios::ate);
  std::ifstream::pos_type pwgSize = ifs.tellg();
  ifs.seekg(0, std::ios::beg);
  Bytestream expected_pwg(pwgSize);
  ifs.read((char*)(expected_pwg.raw()), pwgSize);

  ASSERT(pwg == expected_pwg);

}
