#ifndef URL_H
#define URL_H

#include <string>
#include <regex>

class Url
{
public:
  Url() = delete;
  Url(const std::string& str)
  {
    match(str);
  }

  Url& operator=(const std::string& str)
  {
    match(str);
    return *this;
  }

  operator std::string()
  {
    return toStr();
  }

  std::string toStr()
  {
    std::string maybePort = _port != 0 ? ":" + std::to_string(_port) : "";
    return _scheme + "://" + _host + maybePort + _path;
  }

  std::string getScheme() const
  {
    return _scheme;
  }

  void setScheme(std::string scheme)
  {
    _scheme = std::move(scheme);
  }

  std::string getHost() const
  {
    return _host;
  }

  void setHost(std::string host)
  {
    _host = std::move(host);
  }

  uint16_t getPort() const
  {
    return _port;
  }

  void setPort(uint16_t port)
  {
    _port = port;
  }

  std::string getPath() const
  {
    return _path;
  }

  void setPath(std::string path)
  {
    _path = std::move(path);
  }

private:

  void match(const std::string& str)
  {
    static const std::regex regex("(([a-z]+)://)([a-z0-9-.]+)(:([0-9]+))?(/.*)?$");
    std::smatch match;
    if(!std::regex_match(str, match, regex))
    {
      throw(std::logic_error("Invalid url"));
    }
    _scheme = match[2];
    _host = match[3];
    _port = match[5] == "" ? 0 : stoul(match[5]);
    _path = match[6];
  }

  std::string _scheme;
  std::string _host;
  uint16_t _port;
  std::string _path;
};

#endif // URL_H
