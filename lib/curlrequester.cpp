#include "curlrequester.h"
#include <cstring>
#include <chrono>
#include <thread>

CurlRequester::CurlRequester(std::string addr, bool ignoreSslErrors, bool verbose)
  : _verbose(verbose), _curl(curl_easy_init())
{
  curl_easy_setopt(_curl, CURLOPT_URL, addr.c_str());
  curl_easy_setopt(_curl, CURLOPT_VERBOSE, verbose);
  curl_easy_setopt(_curl, CURLOPT_CONNECTTIMEOUT_MS, 2000);
  curl_easy_setopt(_curl, CURLOPT_FAILONERROR, 1);

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

bool CurlIppPosterBase::write(const void* data, size_t size)
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
  _compression = _nextCompression;
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
    _reading = true;
  }

  size_t remaining = (_size - _offset);
  size_t bytesWritten = 0;

  if(_compression != Compression::None)
  {
    _zstrm.next_out = (Bytef*)dest;
    _zstrm.avail_out = size;
    _zstrm.next_in = (_data + _offset);
    _zstrm.avail_in = remaining;
    deflate(&_zstrm, _done ? Z_FINISH : Z_NO_FLUSH);
    _offset += (remaining - _zstrm.avail_in);
    bytesWritten = size - _zstrm.avail_out;
  }
  else // No compression = memcpy
  {
    bytesWritten = std::min(size, remaining);
    memcpy(dest, (_data+_offset), bytesWritten);
    _offset += bytesWritten;
  }

  if(!_done)
  {
    if(_offset == _size)
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
  if(_nextCompression != Compression::None)
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

CurlIppPosterBase::CurlIppPosterBase(std::string addr, bool ignoreSslErrors, bool verbose)
  : CurlRequester("http"+addr.erase(0,3), ignoreSslErrors, verbose)
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
  _data = Array<uint8_t>(0);
  _size = 0;
  _offset = 0;
  _canRead.unlock();
  return CurlRequester::await(data);
}

CurlIppPoster::CurlIppPoster(std::string addr, const Bytestream& data, bool ignoreSslErrors, bool verbose)
  : CurlIppPosterBase(addr, ignoreSslErrors, verbose)
{
  curl_easy_setopt(_curl, CURLOPT_POSTFIELDSIZE, data.size());
  write(data.raw(), data.size());
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
