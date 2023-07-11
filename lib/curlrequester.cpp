#include "curlrequester.h"
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>

CurlRequester::CurlRequester(std::string addr, bool ignoreSslErrors, bool verbose)
  : _verbose(verbose), _curl(curl_easy_init())
{
  curl_easy_setopt(_curl, CURLOPT_URL, addr.c_str());
  curl_easy_setopt(_curl, CURLOPT_VERBOSE, verbose);

  if(ignoreSslErrors)
  {
    curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYSTATUS, 0L);
  }

  _opts = NULL;
#ifdef USER_AGENT
  _opts = curl_slist_append(_opts, "User-Agent: " USER_AGENT);
#endif

}

CurlRequester::~CurlRequester()
{
  await();
  curl_slist_free_all(_opts);
  curl_easy_cleanup(_curl);
}

void CurlRequester::doRun()
{
  curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, _opts);

  _worker.run([this]()
  {
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

CURLcode CurlRequester::await(Bytestream* data)
{
  _worker.await();

  if(data != nullptr)
  {
    (*data) = _resultMsg;
  }
  return _result;
}

bool CurlIppPosterBase::write(const char* data, size_t size)
{
  while(!_canWrite.try_lock())
  {
    if(!_worker.isRunning())
    {
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  _data = Array<uint8_t>(size);
  memcpy(_data, data, size);
  _size = size;
  _offset = 0;
  _canRead.unlock();
  return true;
}

bool CurlIppPosterBase::give(Bytestream& bts)
{
  while(!_canWrite.try_lock())
  {
    if(!_worker.isRunning())
    {
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  size_t size = bts.size();
  _data = bts.eject();
  _size = size;
  _offset = 0;
  _canRead.unlock();
  return true;
}

size_t CurlIppPosterBase::requestWrite(char* dest, size_t size)
{
  if(!_reading)
  {
    _canRead.lock();
    if(_done)
    {
      return 0;
    }
    _reading = true;
  }

  size_t actualSize = std::min(size, (_size - _offset));

  memcpy(dest, (_data+_offset), actualSize);
  _offset += actualSize;

  if(_offset == _size)
  {
    _reading = false;
    _canWrite.unlock();
  }
  return actualSize;
}

CurlIppPosterBase::CurlIppPosterBase(std::string addr, bool ignoreSslErrors, bool verbose)
  : CurlRequester(addr, ignoreSslErrors, verbose)
{
  _canWrite.unlock();
  _canRead.lock();

  curl_easy_setopt(_curl, CURLOPT_POST, 1L);
  curl_easy_setopt(_curl, CURLOPT_READFUNCTION, trampoline);
  curl_easy_setopt(_curl, CURLOPT_READDATA, this);

  _opts = curl_slist_append(_opts, "Expect:");
  _opts = curl_slist_append(_opts, "Content-Type: application/ipp");
  _opts = curl_slist_append(_opts, "Accept-Encoding: identity");
}

CURLcode CurlIppPosterBase::await(Bytestream* data)
{
  while(_worker.isRunning() && !_canWrite.try_lock())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  _done = true;
  _canRead.unlock();
  return CurlRequester::await(data);
}

CurlIppPoster::CurlIppPoster(std::string addr, const Bytestream& data, bool ignoreSslErrors, bool verbose)
  : CurlIppPosterBase(addr, ignoreSslErrors, verbose)
{
  curl_easy_setopt(_curl, CURLOPT_POSTFIELDSIZE, data.size());
  write((char*)(data.raw()), data.size());
  doRun();
}

CurlIppStreamer::CurlIppStreamer(std::string addr, bool ignoreSslErrors, bool verbose)
  : CurlIppPosterBase(addr, ignoreSslErrors, verbose)
{
  _opts = curl_slist_append(_opts, "Transfer-Encoding: chunked");
  doRun();
}

CurlHttpGetter::CurlHttpGetter(std::string addr, bool ignoreSslErrors, bool verbose)
  : CurlRequester(addr, ignoreSslErrors, verbose)
{
  doRun();
}
