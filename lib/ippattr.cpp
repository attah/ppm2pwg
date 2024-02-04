#include "ippattr.h"
#include <iomanip>

bool IppAttr::isList() const
{
  return is<IppOneSetOf>();
}

bool IppIntRange::operator==(const IppIntRange& other) const
{
  return other.low == low && other.high == high;
}

std::string IppIntRange::toStr() const
{
  std::stringstream ss;
  ss << low << "-" << high;
  return ss.str();
}

bool IppResolution::operator==(const IppResolution& other) const
{
  return other.units == units && other.x == x && other.y == y;
}

std::string IppResolution::toStr() const
{
  std::stringstream ss;
  ss << x << "x" << y << (units == DPI ? "dpi" : units == DPCM ? "dots/cm" : "unknown");
  return ss.str();
}

bool IppDateTime::operator==(const IppDateTime& other) const
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

std::string IppDateTime::toStr() const
{
  // 2000-01-02T00:00:57 GMT+0200
  std::stringstream ss;
  ss << "\"" << std::setfill('0') << std::setw(4) << year << "-"
     << std::setw(2) << +month << "-" << std::setw(2) << +day
     << "T" << std::setw(2) << +hour << ":"
     << std::setw(2) << +minutes << ":" << std::setw(2) << +seconds
     << "." << std::setw(3) << deciSeconds*100 << " GMT" << plusMinus
     << std::setw(2) << +utcHOffset << std::setw(2) << +utcMOffset << "\"";
  return ss.str();
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

IppAttr IppAttr::fromString(std::string string, IppTag tag)
{
  switch(tag)
  {
    case IppTag::OpAttrs:
    case IppTag::JobAttrs:
    case IppTag::EndAttrs:
    case IppTag::PrinterAttrs:
      throw std::logic_error("Unexpected tag");
    case IppTag::Integer:
    case IppTag::Enum:
    {
      return IppAttr(tag, std::stoi(string));
    }
    case IppTag::Boolean:
    {
      if(string == "true")
      {
        return IppAttr(tag, true);
      }
      else if(string == "false")
      {
        return IppAttr(tag, false);
      }
      else
      {
        throw std::out_of_range("Bool not given as true or false");
      }
    }
    // TODO:
    case IppTag::DateTime:
    case IppTag::Resolution:
    case IppTag::IntegerRange:
    case IppTag::OctetStringUnknown:
      throw std::logic_error("Unhandled tag");
    case IppTag::TextWithLanguage:
    case IppTag::NameWithLanguage:
    case IppTag::TextWithoutLanguage:
    case IppTag::NameWithoutLanguage:
    case IppTag::Keyword:
    case IppTag::Uri:
    case IppTag::UriScheme:
    case IppTag::Charset:
    case IppTag::NaturalLanguage:
    case IppTag::MimeMediaType:
    {
      return IppAttr(tag, string);
    }
    case IppTag::BeginCollection:
    default:
      throw std::logic_error("Unhandled tag");
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
  os << dt.toStr();
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
    os << (iv.get<bool>() ? "true" : "false");
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
    for(const IppValue& iv2 : oneSet)
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
     << "\"" << it->first << "\": {\"tag\": " << +(uint8_t)(it->second.tag())
     << ", \"value\": " << it->second.value() << "}";
  it++;
  for(; it != as.cend(); it++)
  {
    os << "," << std::endl
       << "\"" << it->first << "\": {\"tag\": " << +(uint8_t)(it->second.tag())
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
