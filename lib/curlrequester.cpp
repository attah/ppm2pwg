#include "curlrequester.h"
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>

static size_t trampoline(char* dest, size_t size, size_t nmemb, void* userp)
{
  CurlRequester* cid = (CurlRequester*)userp;
  return cid->requestWrite(dest, size*nmemb);
}

CurlRequester::CurlRequester(std::string addr, bool ignoreSslErrors, bool verbose,
                             Role role, Bytestream* singleData)
  : _verbose(verbose), _curl(curl_easy_init())
{
  _canWrite.unlock();
  _canRead.lock();

  curl_easy_setopt(_curl, CURLOPT_URL, addr.c_str());

  curl_easy_setopt(_curl, CURLOPT_VERBOSE, verbose);

  if(ignoreSslErrors)
  {
    curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYSTATUS, 0L);
  }

  _opts = NULL;
  if(_userAgent != "")
  {
    std::string userAgentHdr = "User-Agent: " + _userAgent;
    _opts = curl_slist_append(_opts, userAgentHdr.c_str());
  }

  switch (role) {
    case IppRequest:
    {
      curl_easy_setopt(_curl, CURLOPT_POST, 1L);
      curl_easy_setopt(_curl, CURLOPT_READFUNCTION, trampoline);
      curl_easy_setopt(_curl, CURLOPT_READDATA, this);

      _opts = curl_slist_append(_opts, "Expect:");
      if(singleData != nullptr)
      {
        curl_easy_setopt(_curl, CURLOPT_POSTFIELDSIZE, singleData->size());
        write((char*)(singleData->raw()), singleData->size());
      }
      else
      {
        _opts = curl_slist_append(_opts, "Transfer-Encoding: chunked");
      }
      _opts = curl_slist_append(_opts, "Content-Type: application/ipp");
      _opts = curl_slist_append(_opts, "Accept-Encoding: identity");
      break;
    }
    case HttpGetRequest:
    {
      curl_easy_setopt(_curl, CURLOPT_HTTPGET, 1L);
      break;
    }
  }

  curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, _opts);

  _worker.run([this](){
    Bytestream buf;
    curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, write_callback);

    CURLcode res = curl_easy_perform(_curl);
    if(res != CURLE_OK)
      std::cerr <<  "curl_easy_perform() failed: " << curl_easy_strerror(res);

    _result = res;
    _resultMsg = buf;
  });
}

CurlRequester::~CurlRequester()
{
  await();

  if(_dest != nullptr)
  {
    delete _dest;
  }

  curl_slist_free_all(_opts);
  curl_easy_cleanup(_curl);
}

CURLcode CurlRequester::await(Bytestream* data)
{
  while(_worker.isRunning() && !_canWrite.try_lock())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  _done = true;
  _canRead.unlock();
  _worker.await();

  if(data != nullptr)
  {
    (*data) = _resultMsg;
  }
  return _result;
}

bool CurlRequester::write(const char *data, size_t size)
{
  while(!_canWrite.try_lock())
  {
    if(!_worker.isRunning())
    {
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  if(_dest != nullptr)
  {
    delete _dest;
  }
  _dest = new char[size];
  memcpy(_dest, data, size);
  _size = size;
  _offset = 0;
  _canRead.unlock();
  return true;
}

size_t CurlRequester::requestWrite(char* dest, size_t size)
{
  if(!_reading)
  {
    _canRead.lock();
    if(_done) // Can only have been set by await() - only relevant to check if strating to write
    {
      return 0;
    }
    _reading = true;
  }

  size_t remaining = _size - _offset;

  size_t actualSize = std::min(size, remaining);

  memcpy(dest, (_dest+_offset), actualSize);
  _offset += actualSize;

  remaining = _size - _offset;
  if(remaining == 0)
  {
    _reading = false;
    _canWrite.unlock();
  }
  return actualSize;
}