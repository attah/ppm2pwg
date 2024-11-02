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

IppIntRange IppIntRange::fromJSON(const Json::object json)
{
  return IppIntRange {json.at("low").int_value(), json.at("high").int_value()};
}

Json::object IppIntRange::toJSON() const
{
  return Json::object {{"low", low}, {"high", high}};
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

IppResolution IppResolution::fromJSON(const Json::object json)
{
  return IppResolution {(uint32_t)json.at("x").int_value(),
                        (uint32_t)json.at("y").int_value(),
                        (uint8_t)json.at("units").int_value()};
}

Json::object IppResolution::toJSON() const
{
  return Json::object {{"x", (int)x}, {"y", (int)y}, {"units", units}};
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
  ss << std::setfill('0') << std::setw(4) << year << "-"
     << std::setw(2) << +month << "-" << std::setw(2) << +day
     << "T" << std::setw(2) << +hour << ":"
     << std::setw(2) << +minutes << ":" << std::setw(2) << +seconds
     << "." << std::setw(3) << deciSeconds*100 << " GMT" << plusMinus
     << std::setw(2) << +utcHOffset << std::setw(2) << +utcMOffset;
  return ss.str();
}

IppDateTime IppDateTime::fromJSON(const Json::object)
{
  // TODO
  return IppDateTime();
}

Json IppDateTime::toJSON() const
{
  return toStr();
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

IppAttr IppAttr::fromJSON(Json::object json)
{
  IppTag tag = (IppTag)json.at("tag").int_value();
  return IppAttr(tag, valuefromJSON(tag, json.at("value")));
}

Json IppAttr::toJSON() const
{
  return Json::object {{"tag", (int)tag()}, {"value", valueToJSON(value())}};
}

IppValue IppAttr::valuefromJSON(IppTag tag, const Json& json)
{
  if(json.is_string())
  {
    return IppValue(json.string_value());
  }
  else if(json.is_number())
  {
    return IppValue(json.int_value());
  }
  else if(json.is_bool())
  {
    return IppValue(json.bool_value());
  }
  else if(json.is_array())
  {
    IppOneSetOf oneSet;
    for(const Json& j : json.array_items())
    {
      oneSet.push_back(valuefromJSON(tag, j));
    }
    return oneSet;
  }
  else if(json.is_object() && tag == IppTag::IntegerRange)
  {
    return IppValue(IppIntRange::fromJSON(json.object_items()));
  }
  else if(json.is_object() && tag == IppTag::Resolution)
  {
    return IppValue(IppResolution::fromJSON(json.object_items()));
  }
  else if(json.is_object() && tag == IppTag::DateTime)
  {
    return IppValue(IppDateTime::fromJSON(json.object_items()));
  }
  else if(json.is_object() && tag == IppTag::BeginCollection)
  {
    IppCollection collection;
    for(const auto& [k, v] : json.object_items())
    {
      collection.insert_or_assign(k, fromJSON(v.object_items()));
    }
    return IppValue(collection);
  }
  else
  {
    throw std::logic_error("Unhandled data type");
  }
}

Json IppAttr::valueToJSON(IppValue value)
{
  Json j;
  if(value.is<std::string>())
  {
    j = value.get<std::string>();
  }
  else if(value.is<int>())
  {
    j = value.get<int>();
  }
  else if(value.is<bool>())
  {
    j = value.get<bool>();
  }
  else if(value.is<IppIntRange>())
  {
    j = value.get<IppIntRange>().toJSON();
  }
  else if(value.is<IppResolution>())
  {
    j = value.get<IppResolution>().toJSON();
  }
  else if(value.is<IppDateTime>())
  {
    j = value.get<IppDateTime>().toJSON();
  }
  else if(value.is<IppOneSetOf>())
  {
    Json::array arr;
    IppOneSetOf oneSet = value.get<IppOneSetOf>();
    for(const IppValue& v : oneSet)
    {
      arr.push_back(valueToJSON(v));
    }
    j = arr;
  }
  else if(value.is<IppCollection>())
  {
    Json::object obj;
    for(const auto& [k, v] : value.get<IppCollection>())
    {
      obj[k] = v.toJSON();
    }
    j = obj;
  }
  return j;
}

std::ostream& operator<<(std::ostream& os, const IppValue& iv)
{
  if(iv.is<std::string>())
  {
    os << iv.get<std::string>();
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
    os << iv.get<IppIntRange>().toStr();
  }
  else if(iv.is<IppResolution>())
  {
    os << iv.get<IppResolution>().toStr();
  }
  else if(iv.is<IppDateTime>())
  {
    os << iv.get<IppDateTime>().toStr();
  }
  else if(iv.is<IppOneSetOf>())
  {
    IppOneSetOf oneSet = iv.get<IppOneSetOf>();
    for(IppOneSetOf::const_iterator it = oneSet.cbegin(); it != oneSet.cend(); it++)
    {
      os << *it;
      if(std::next(it) != oneSet.cend())
      {
        os << ", ";
      }
    }
  }
  else if(iv.is<IppCollection>())
  {
    os << "<collection>";
  }
  else
  {
    os << "unhandled value";
  }

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

IppAttrs IppAttrs::fromJSON(Json::object json)
{
  IppAttrs attrs;
  for(const auto& [key, value] : json)
  {
    attrs.insert_or_assign(key, IppAttr::fromJSON(value.object_items()));
  }
  return attrs;
}

Json IppAttrs::toJSON() const
{
  Json::object json;
  for(const auto& [key, value] : *this)
  {
    json[key] = value.toJSON();
  }
  return json;
}
