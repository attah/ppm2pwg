#include "ippattr.h"
#include <iomanip>

bool IppAttr::isList() const
{
  return is<IppOneSetOf>();
}
IppOneSetOf IppAttr::asList() const
{
  if(isList())
  {
    return get<IppOneSetOf>();
  }
  else
  {
    return IppOneSetOf {{value()}};
  }
}

std::ostream& operator<<(std::ostream& os, const IppIntRange& ir)
{
  os << "{\"min\": " << ir.low << ", \"max\": " << ir.high << "}";
  return os;
}
std::ostream& operator<<(std::ostream& os, const IppResolution& res)
{
  os << "{\"x\": " << res.x
     << ", \"y\": " << res.y
     << ", \"units\": " << +res.units << "}";
  return os;
}
std::ostream& operator<<(std::ostream& os, const IppDateTime& dt)
{
  // 2000-01-02T00:00:57 GMT+0200
  os << "\"" << std::setfill('0') << std::setw(4) << dt.year << "-"
     << std::setw(2) << +dt.month << "-" << std::setw(2) << +dt.day
     << "T" << std::setw(2) << +dt.hour << ":"
     << std::setw(2) << +dt.minutes << ":" << std::setw(2) << +dt.seconds
     << "." << std::setw(3) << dt.deciSeconds*100 << " GMT" << dt.plusMinus
     << std::setw(2) << +dt.utcHOffset << std::setw(2) << +dt.utcMOffset << "\"";
  return os;
}

std::ostream& operator<<(std::ostream& os, const IppValue& iv)
{
  if(iv.is<std::string>())
  {
    os << "\""  << iv.get<std::string>() << "\"";
  }
  else if(iv.is<int>())
  {
    os << iv.get<int>();
  }
  else if(iv.is<bool>())
  {
    os << iv.get<bool>();
  }
  else if(iv.is<IppIntRange>())
  {
    os << iv.get<IppIntRange>();
  }
  else if(iv.is<IppResolution>())
  {
    os << iv.get<IppResolution>();
  }
  else if(iv.is<IppDateTime>())
  {
    os << iv.get<IppDateTime>();
  }
  else if(iv.is<IppOneSetOf>())
  {
    IppOneSetOf oneSet = iv.get<IppOneSetOf>();
    os << "[" << oneSet.takeFront();
    for(IppValue iv2 : oneSet)
    {
      os << ", " << iv2;
    }
    os << "]";
  }
  else if(iv.is<IppCollection>())
  {
    os << "{";
    IppCollection ippCollection = iv.get<IppCollection>();
    IppCollection::const_iterator it = ippCollection.cbegin();
    if(it != ippCollection.cend())
    {
      os << "\"" << it->first << "\": " << it->second.value();
      it++;
      for(; it != ippCollection.cend(); it++)
      {
        os << ", \"" << it->first << "\": " << it->second.value();
      }
    }
    os << "}";
  }
  else
  {
    os << "unhandled value";
  }

  return os;
}

std::ostream& operator<<(std::ostream& os, const IppAttrs& as)
{
  if(as.empty())
  {
    os << "{}" << std::endl;
    return os;
  }

  IppAttrs::const_iterator it = as.cbegin();
  os << "{"
     << "\"" << it->first << "\": {\"tag\": " << +it->second.tag()
     << ", \"value\": " << it->second.value() << "}";
  it++;
  for(; it != as.cend(); it++)
  {
    os << "," << std::endl
       << "\"" << it->first << "\": {\"tag\": " << +it->second.tag()
       << ", \"value\": " << it->second.value() << "}";
  }
  os << "}" << std::endl;
  return os;
}

bool IppCollection::has(std::string key) const
{
  return find(key) != end();
}

void IppCollection::set(std::string key, IppAttr value)
{
  insert_or_assign(key, value);
}
