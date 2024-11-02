#ifndef IPPATTR_H
#define IPPATTR_H

#include "json11.hpp"
#include "list.h"
#include "polymorph.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace json11;

struct IppOneSetOf;
struct IppCollection;
class IppAttr;

enum class IppTag : uint8_t
{
  OpAttrs             = 0x01,
  JobAttrs            = 0x02,
  EndAttrs            = 0x03,
  PrinterAttrs        = 0x04,
  UnsupportedAttrs    = 0x05,
  Unsupported         = 0x10,
  Integer             = 0x21,
  Boolean             = 0x22,
  Enum                = 0x23,
  OctetStringUnknown  = 0x30,
  DateTime            = 0x31,
  Resolution          = 0x32,
  IntegerRange        = 0x33,
  BeginCollection     = 0x34,
  TextWithLanguage    = 0x35,
  NameWithLanguage    = 0x36,
  EndCollection       = 0x37,
  TextWithoutLanguage = 0x41,
  NameWithoutLanguage = 0x42,
  Keyword             = 0x44,
  Uri                 = 0x45,
  UriScheme           = 0x46,
  Charset             = 0x47,
  NaturalLanguage     = 0x48,
  MimeMediaType       = 0x49,
  MemberName          = 0x4A
};

struct IppIntRange
{
  int32_t low = 0;
  int32_t high = 0;

  bool operator==(const IppIntRange& other) const;
  std::string toStr() const;
  static IppIntRange fromJSON(const Json::object json);
  Json::object toJSON() const;
};

struct IppResolution
{
  enum Units : uint8_t
  {
    Invalid=0,
    DPI=3,
    DPCM=4
  };

  uint32_t x = 0;
  uint32_t y = 0;
  uint8_t units = Invalid;

  bool operator==(const IppResolution& other) const;
  std::string toStr() const;
  static IppResolution fromJSON(const Json::object json);
  Json::object toJSON() const;
};

struct IppDateTime
{
  uint16_t year = 0;
  uint8_t month = 0;
  uint8_t day = 0;
  uint8_t hour = 0;
  uint8_t minutes = 0;
  uint8_t seconds = 0;
  uint8_t deciSeconds = 0;
  uint8_t plusMinus = '+';
  uint8_t utcHOffset = 0;
  uint8_t utcMOffset = 0;

  bool operator==(const IppDateTime& other) const;
  std::string toStr() const;
  static IppDateTime fromJSON(const Json::object json);
  Json toJSON() const;
};

using IppValue = Polymorph<std::string, int, bool,
                           IppIntRange, IppResolution, IppDateTime,
                           IppOneSetOf, IppCollection>;

struct IppOneSetOf: public List<IppValue>
{
  using List<IppValue>::List;
};

struct IppCollection: public std::map<std::string, IppAttr>
{
  using std::map<std::string, IppAttr>::map;
  bool has(std::string key) const;
  void set(std::string key, IppAttr value);
};

class IppAttr : public IppValue
{
public:
  bool isList() const ;
  IppOneSetOf asList() const;

  template <typename T>
  IppAttr(IppTag tag, T value) : IppValue(value), _tag(tag) {}

  IppTag tag() const {return _tag;}
  IppValue value() const {return *this;}
  static IppAttr fromString(std::string string, IppTag tag);

  static IppAttr fromJSON(Json::object json);
  Json toJSON() const;

private:
  static IppValue valuefromJSON(IppTag tag, const Json& json);
  static Json valueToJSON(IppValue value);

  IppTag _tag;

};

struct IppAttrs: public std::map<std::string, IppAttr>
{
  using std::map<std::string, IppAttr>::map;

  bool has(std::string key) const
  {
    return find(key) != end();
  }

  template <typename T>
  T get(std::string name, T defaultValue=T()) const
  {
    try
    {
      return at(name).get<T>();
    }
    catch (const std::out_of_range&)
    {
      return defaultValue;
    }
  }

  void set(std::string key, IppAttr value)
  {
    insert_or_assign(key, value);
  }

  template <typename T>
  List<T> getList(std::string name) const
  {
    List<T> res;
    if(has(name))
    {
      for(const IppValue& v : at(name).asList())
      {
        res.push_back(v.get<T>());
      }
    }
    return res;
  }

  static IppAttrs fromJSON(Json::object json);
  Json toJSON() const;

};

std::ostream& operator<<(std::ostream& os, const IppIntRange& iv);
std::ostream& operator<<(std::ostream& os, const IppResolution& iv);
std::ostream& operator<<(std::ostream& os, const IppDateTime& iv);
std::ostream& operator<<(std::ostream& os, const IppValue& iv);

std::ostream& operator<<(std::ostream& os, const IppAttrs& as);

#endif // IPPATTR_H
