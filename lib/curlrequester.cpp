#include "curlrequester.h"

#include "log.h"

#include <chrono>
#include <cstring>
#include <thread>

CurlRequester::CurlRequester(const std::string& addr, const SslConfig& sslConfig)
  : _curl(curl_easy_init())
{
  bool debugEnabled = LogController::instance().isEnabled(LogController::Debug);
  curl_easy_setopt(_curl, CURLOPT_URL, addr.c_str());
  curl_easy_setopt(_curl, CURLOPT_VERBOSE, debugEnabled);
  curl_easy_setopt(_curl, CURLOPT_CONNECTTIMEOUT_MS, 2000);
  curl_easy_setopt(_curl, CURLOPT_FAILONERROR, 1);

  if(!sslConfig.verifySsl)
  {
    curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYSTATUS, 0L);
  }

  if(sslConfig.pinnedPublicKey != "")
  {
    curl_easy_setopt(_curl, CURLOPT_PINNEDPUBLICKEY, sslConfig.pinnedPublicKey.c_str());
  }

  _opts = nullptr;
#ifdef USER_AGENT
  _opts = curl_slist_append(_opts, "User-Agent: " USER_AGENT);
#endif

}

CurlRequester::~CurlRequester()
{
  CurlRequester::await();
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

    _result = curl_easy_perform(_curl);
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

CurlIppPosterBase::~CurlIppPosterBase()
{
  CurlIppPosterBase::await();
}

bool CurlIppPosterBase::write(Bytestream&& data)
{
  while(!_canWrite.try_lock())
  {
    if(!_worker.isRunning())
    {
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  _data = std::move(data);
  _compression = _nextCompression;
  _canRead.unlock();
  return true;
}

size_t CurlIppPosterBase::requestWrite(char* dest, size_t size)
{
  if(!_reading)
  {
    _canRead.lock();
    _reading = true;
  }

  size_t remaining = _data.remaining();
  size_t bytesWritten = 0;

  if(_compression != NoCompression)
  {
    _zstrm.next_out = (Bytef*)dest;
    _zstrm.avail_out = size;
    _zstrm.next_in = (_data.raw() + _data.pos());
    _zstrm.avail_in = remaining;
    deflate(&_zstrm, _done ? Z_FINISH : Z_NO_FLUSH);
    bytesWritten = size - _zstrm.avail_out;
    _data += (remaining - _zstrm.avail_in);
  }
  else // No compression = memcpy
  {
    bytesWritten = std::min(size, remaining);
    if(bytesWritten != 0) // Please the ubsan
    {
      _data.getBytes(dest, bytesWritten);
    }
  }

  if(!_done)
  {
    if(_data.atEnd())
    { // End of input
      _reading = false;
      _canWrite.unlock();
    }
    if(bytesWritten == 0)
    { // Compression produced no output this time,
      // we'll most likely have gone to write mode,
      // keep waiting.
      return requestWrite(dest, size);
    }
  }

  return bytesWritten;
}

void CurlIppPosterBase::setCompression(Compression compression)
{
  if(_nextCompression != NoCompression)
  {
    throw std::logic_error("unsetting/changing compression");
  }
  _nextCompression = compression;

  _zstrm.zalloc = Z_NULL;
  _zstrm.zfree = Z_NULL;
  _zstrm.opaque = Z_NULL;

  int level = 11;
  if(compression == Compression::Deflate)
  {
    level *= -1;
  }
  else if(compression == Compression::Gzip)
  {
    level += 16;
  }

  deflateInit2(&_zstrm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, level, 7, Z_DEFAULT_STRATEGY);
}

std::string http_url(const std::string& str)
{
  Url url(str);
  if(url.getScheme() == "ipp")
  {
    url.setScheme("http");
    if(!url.getPort())
    {
      url.setPort(631);
    }
  }
  else if(url.getScheme() == "ipps")
  {
    url.setScheme("https");
    if(!url.getPort())
    {
      url.setPort(443);
    }
  }
  return url.toStr();
}

CurlIppPosterBase::CurlIppPosterBase(const std::string& addr, const SslConfig& sslConfig)
  : CurlRequester(http_url(addr), sslConfig)
{
  _canWrite.unlock();
  _canRead.lock();

  curl_easy_setopt(_curl, CURLOPT_POST, 1L);
  curl_easy_setopt(_curl, CURLOPT_UPLOAD_BUFFERSIZE, 2*1024*1024);
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
  _data = Bytestream();
  _canRead.unlock();
  return CurlRequester::await(data);
}

CurlIppPoster::CurlIppPoster(const std::string& addr, Bytestream&& data, const SslConfig& sslConfig)
  : CurlIppPosterBase(addr, sslConfig)
{
  curl_easy_setopt(_curl, CURLOPT_POSTFIELDSIZE, data.size());
  write(std::move(data));
  doRun();
}

CurlIppStreamer::CurlIppStreamer(const std::string& addr, const SslConfig& sslConfig)
  : CurlIppPosterBase(addr, sslConfig)
{
  _opts = curl_slist_append(_opts, "Transfer-Encoding: chunked");
  doRun();
}

CurlHttpGetter::CurlHttpGetter(const std::string& addr, const SslConfig& sslConfig)
  : CurlRequester(addr, sslConfig)
{
  doRun();
}
