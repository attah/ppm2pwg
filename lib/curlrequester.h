#ifndef CURLREQUESTER_H
#define CURLREQUESTER_H

#include "bytestream.h"
#include "lthread.h"
#include "url.h"

#include <curl/curl.h>
#include <zlib.h>
#include <mutex>

class SslConfig
{
friend class CurlRequester;
public:
  SslConfig(bool verifySsl=true, std::string pinnedPublicKey="")
  : _verifySsl(verifySsl), _pinnedPublicKey(std::move(pinnedPublicKey))
  {}
private:
  bool _verifySsl = true;
  std::string _pinnedPublicKey;
};

class CurlRequester
{
public:
  CurlRequester(const CurlRequester&) = delete;
  CurlRequester& operator=(const CurlRequester&) = delete;

  ~CurlRequester();

  virtual CURLcode await(Bytestream* = nullptr);

  static void setUserAgent(std::string userAgent);

protected:

  CurlRequester(const Url& addr, const SslConfig& sslConfig);

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

  LThread _worker;

  static std::string _userAgent;
};

class CurlIppPosterBase : public CurlRequester
{
public:

  enum Compression
  {
    NoCompression = 0,
    Deflate,
    Gzip
  };

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
  CurlIppPosterBase(Url addr, const SslConfig& sslConfig=SslConfig());

private:
  std::mutex _canWrite;
  std::mutex _canRead;
  bool _reading = false;
  bool _done = false;

  z_stream _zstrm;
  Compression _nextCompression = NoCompression;
  Compression _compression = NoCompression;
};

class CurlIppPoster : public CurlIppPosterBase
{
public:
  CurlIppPoster(const Url& addr, Bytestream&& data, const SslConfig& sslConfig=SslConfig());
};

class CurlIppStreamer : public CurlIppPosterBase
{
public:
  CurlIppStreamer(const Url& addr, const SslConfig& sslConfig=SslConfig());
};

class CurlHttpGetter : public CurlRequester
{
public:
  CurlHttpGetter(const Url& addr, const SslConfig& sslConfig=SslConfig());
};

#endif // CURLREQUESTER_H
