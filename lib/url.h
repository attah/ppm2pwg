#ifndef URL_H
#define URL_H

#include <cstdint>
#include <string>
#include <regex>

class Url
{
public:
  Url() = default;
  Url(const std::string& str)
  {
    match(str);
  }

  Url& operator=(const std::string& str)
  {
    match(str);
    return *this;
  }

  bool isValid() const
  {
    return _valid;
  }

  operator std::string() const
  {
    return toStr();
  }

  std::string toStr() const
  {
    if(!_valid)
    {
      return "";
    }
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
    static const std::regex regex("(([a-z]+)://)((\\[[:a-zA-Z0-9]+\\])|([a-zA-Z0-9-.]*))(:([0-9]+))?(/.*)?$");
    std::smatch match;

    _valid = false;
    _scheme = "";
    _host = "";
    _port = 0;
    _path = "";

    if(std::regex_match(str, match, regex))
    {
      _valid = true;
      _scheme = match[2];
      _host = match[3];
      _port = match[7] == "" ? 0 : stoul(match[7]);
      _path = match[8];
    }
  }

  bool _valid = false;
  std::string _scheme;
  std::string _host;
  uint16_t _port = 0;
  std::string _path;
};

#endif // URL_H
