#ifndef IPPATTR_H
#define IPPATTR_H

#include "list.h"
#include <cstdint>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <iostream>
#include "polymorph.h"

struct IppOneSetOf;
struct IppCollection;
class IppAttr;

struct IppIntRange
{
  int32_t low = 0;
  int32_t high = 0;

  bool operator==(const IppIntRange& other) const
  {
    return other.low == low && other.high == high;
  }
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

  bool operator==(const IppResolution& other) const
  {
    return other.units == units && other.x == x && other.y == y;
  }

  std::string toStr() const
  {
    std::stringstream ss;
    ss << x << "x" << y << (units == DPI ? "dpi" : units == DPCM ? "dots/cm" : "unknown");
    return ss.str();
  }
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

  bool operator==(const IppDateTime& other) const
  {
    return other.year == year &&
           other.month == month &&
           other.day == day &&
           other.hour == hour &&
           other.minutes == minutes &&
           other.seconds == seconds &&
           other.deciSeconds == deciSeconds &&
           other.plusMinus == plusMinus &&
           other.utcHOffset == utcHOffset &&
           other.utcMOffset == utcMOffset;
  }
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
  IppAttr(uint8_t tag, T value) : IppValue(value), _tag(tag) {}

  uint8_t tag() const {return _tag;}
  IppValue value() const {return *this;}

private:
  uint8_t _tag;

};

struct IppAttrs: public std::map<std::string, IppAttr>
{
  using std::map<std::string, IppAttr>::map;

  bool has(std::string key) const
  {
    return find(key) != end();
  }

  template <typename T>
  T get(std::string name, T res=T()) const
  {
    if(find(name) != end())
    {
      res = at(name).get<T>();
    }
    return res;
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
};

std::ostream& operator<<(std::ostream& os, const IppIntRange& iv);
std::ostream& operator<<(std::ostream& os, const IppResolution& iv);
std::ostream& operator<<(std::ostream& os, const IppDateTime& iv);
std::ostream& operator<<(std::ostream& os, const IppValue& iv);

std::ostream& operator<<(std::ostream& os, const IppAttrs& as);

#endif // IPPATTR_H
