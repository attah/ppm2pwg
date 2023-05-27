#ifndef CURLREQUESTER_H
#define CURLREQUESTER_H

#ifndef USER_AGENT
#define USER_AGENT ""
#endif

#include <curl/curl.h>
#include "lthread.h"
#include <bytestream.h>
#include <mutex>

class CurlRequester
{
public:
  enum Role {
    IppRequest,
    HttpGetRequest
  };

  CurlRequester(std::string addr, bool ignoreSslErrors = false, bool verbose = false,
                Role role = IppRequest, Bytestream* = nullptr);
  ~CurlRequester();

  CurlRequester(const CurlRequester&) = delete;
  CurlRequester& operator=(const CurlRequester&) = delete;

  CURLcode await(Bytestream* = nullptr);

  bool write(const char *data, size_t size);
  size_t requestWrite(char* dest, size_t size);

  static size_t write_callback(char *ptr, size_t size, size_t nmemb, void* userdata)
  {
    size_t bytes_to_write = size*nmemb;
    ((Bytestream*)userdata)->putBytes(ptr, bytes_to_write);
    return bytes_to_write;
  }

private:
  CurlRequester();

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

  bool _verbose;

  std::string _userAgent = USER_AGENT;

  std::mutex _canWrite;
  std::mutex _canRead;
  bool _reading = false;
  bool _done = false;

  CURLcode _result;
  Bytestream _resultMsg;

  char* _dest = nullptr;
  size_t _size = 0;
  size_t _offset = 0;

  friend class CurlWorker;

  CURL* _curl;
  struct curl_slist* _opts = NULL;

  LThread _worker;
};

#endif // CURLREQUESTER_H