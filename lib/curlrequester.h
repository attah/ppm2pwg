#ifndef CURLREQUESTER_H
#define CURLREQUESTER_H

#include "array.h"
#include "bytestream.h"
#include "lthread.h"
#include "url.h"

#include <curl/curl.h>
#include <zlib.h>
#include <mutex>

#ifndef USER_AGENT
#define USER_AGENT ""
#endif

enum class Compression
{
  None = 0,
  Deflate,
  Gzip
};

struct SslConfig
{
  bool verifySsl = true;
  std::string pinnedPublicKey;
};

class CurlRequester
{
public:
  ~CurlRequester();

  CurlRequester(const CurlRequester&) = delete;
  CurlRequester& operator=(const CurlRequester&) = delete;

  virtual CURLcode await(Bytestream* = nullptr);

protected:

  CurlRequester(const std::string& addr, const SslConfig& sslConfig=SslConfig());

  void doRun();

  CurlRequester();

  static size_t write_callback(char *ptr, size_t size, size_t nmemb, void* userdata)
  {
    size_t bytes_to_write = size*nmemb;
    ((Bytestream*)userdata)->putBytes(ptr, bytes_to_write);
    return bytes_to_write;
  }

  // Container for the cURL global init and cleanup
  class GlobalEnv
  {
  public:
    GlobalEnv()
    {
      curl_global_init(CURL_GLOBAL_DEFAULT);
    }
    ~GlobalEnv()
    {
      curl_global_cleanup();
    }
  };
  // Must be run exactly once, thus static
  static GlobalEnv _gEnv;

  CURLcode _result;
  Bytestream _resultMsg;

  Bytestream _data;

  CURL* _curl;
  struct curl_slist* _opts = nullptr;

  z_stream _zstrm;
  Compression _nextCompression = Compression::None;
  Compression _compression = Compression::None;
  LThread _worker;
};

class CurlIppPosterBase : public CurlRequester
{
public:
  ~CurlIppPosterBase();
  CURLcode await(Bytestream* = nullptr) override;

  bool write(Bytestream&& data);
  size_t requestWrite(char* dest, size_t size);

  void setCompression(Compression compression);

  static size_t trampoline(char* dest, size_t size, size_t nmemb, void* userp)
  {
    CurlIppPosterBase* cid = (CurlIppPosterBase*)userp;
    return cid->requestWrite(dest, size*nmemb);
  }

protected:
  CurlIppPosterBase(const std::string& addr, const SslConfig& sslConfig=SslConfig());

private:
  std::mutex _canWrite;
  std::mutex _canRead;
  bool _reading = false;
  bool _done = false;
};

class CurlIppPoster : public CurlIppPosterBase
{
public:
  CurlIppPoster(const std::string& addr, Bytestream&& data, const SslConfig& sslConfig=SslConfig());
};

class CurlIppStreamer : public CurlIppPosterBase
{
public:
  CurlIppStreamer(const std::string& addr, const SslConfig& sslConfig=SslConfig());
};

class CurlHttpGetter : public CurlRequester
{
public:
  CurlHttpGetter(const std::string& addr, const SslConfig& sslConfig=SslConfig());
};

#endif // CURLREQUESTER_H
