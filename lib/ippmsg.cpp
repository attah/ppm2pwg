#include "ippmsg.h"
#include <iostream>

uint32_t IppMsg::_reqId=1;

IppMsg::IppMsg(Bytestream& bts)
{
  uint32_t reqId;

  bts >> _majVsn >> _minVsn >> _opOrStatus >> reqId;

  IppAttrs attrs;
  Tag currentAttrType = EndAttrs;

  while(!bts.atEnd())
  {
    if(bts.peekU8() <= UnsupportedAttrs)
    {
      if(currentAttrType == OpAttrs)
      {
        _opAttrs = attrs;
      }
      else if (currentAttrType == JobAttrs)
      {
        _jobAttrs.push_back(attrs);
      }
      else if (currentAttrType == PrinterAttrs)
      {
        _printerAttrs = attrs;
      }
      else if (currentAttrType == UnsupportedAttrs)
      {
        std::cerr << "WARNING: unsupported attrs reported: " << std::endl
                  << attrs;
      }

      if(bts >>= EndAttrs)
      {
        break;
      }

      currentAttrType = (Tag)bts.getU8();
      attrs = IppAttrs();
    }
    else
    {
      consumeAttributes(attrs, bts);
    }
  }
}

IppMsg::IppMsg(uint16_t opOrStatus, IppAttrs opAttrs, IppAttrs jobAttrs,
               uint8_t majVsn, uint8_t minVsn, IppAttrs printerAttrs)
{
  _opOrStatus = opOrStatus;
  _majVsn = majVsn;
  _minVsn = minVsn;
  _opAttrs = opAttrs;
  _jobAttrs = {jobAttrs};
  _printerAttrs = printerAttrs;
}

Bytestream IppMsg::encode() const
{
  Bytestream ipp;
  ipp << _majVsn << _minVsn;
  ipp << _opOrStatus;
  ipp << _reqId++;
  ipp << OpAttrs;

  IppAttrs opAttrs = _opAttrs;

  // attributes-charset and attributes-natural-language are required to be first
  // some printers fail if the other mandatory parameters are not in this specific order
  std::list<std::string> InitialAttrs = {"attributes-charset",
                                         "attributes-natural-language",
                                         "printer-uri",
                                         "job-id",
                                         "requesting-user-name"};
  for(const std::string& key : InitialAttrs)
  {
    if(opAttrs.has(key))
    {
      IppAttr val = opAttrs.at(key);
      encodeAttribute(ipp, key, val);
      opAttrs.erase(key);
    }
  }
  encodeAttributes(ipp, opAttrs);
  for(IppAttrs attrs : _jobAttrs)
  {
    if (!attrs.empty()) {
      ipp << JobAttrs;
      encodeAttributes(ipp, attrs);
    }
  }
  if (!_printerAttrs.empty()) {
    ipp << PrinterAttrs;
    encodeAttributes(ipp, _printerAttrs);
  }
  ipp << EndAttrs;
  return ipp;
}

void IppMsg::setOpAttr(std::string name, IppAttr attr)
{
  _opAttrs.insert({name, attr});
}

IppAttrs IppMsg::baseOpAttrs(std::string url)
{
  char* user = getenv("USER");
  std::string name = user ? user : "anonymous";
  IppAttrs o
  {
    {"attributes-charset",          IppAttr(IppMsg::Charset,             "utf-8")},
    {"attributes-natural-language", IppAttr(IppMsg::NaturalLanguage,     "en-us")},
    {"printer-uri",                 IppAttr(IppMsg::Uri,                 url)},
    {"requesting-user-name",        IppAttr(IppMsg::NameWithoutLanguage, name)},
  };
  return o;
}

IppValue IppMsg::decodeValue(uint8_t tag, Bytestream& data) const
{
  uint16_t tmpLen;

  switch (tag)
  {
    case OpAttrs:
    case JobAttrs:
    case EndAttrs:
    case PrinterAttrs:
    case UnsupportedAttrs:
      throw std::logic_error("Unexpected tag");
    case Integer:
    case Enum:
    {
      uint32_t tmp_u32=0;
      if(!(data >>= (uint16_t)0))
      {
        data >> (uint16_t)4 >> tmp_u32;
      }
      return IppValue((int)tmp_u32);
    }
    case Boolean:
    {
      uint8_t tmp_bool=0;
      if(!(data >>= (uint16_t)0))
      {
        data >> (uint16_t)1 >> tmp_bool;
      }
      return IppValue((bool)tmp_bool);
    }
    case DateTime:
    {
      IppDateTime tmpDateTime;
      if(!(data >>= (uint16_t)0))
      {
        data >> (uint16_t)11 >> tmpDateTime.year >> tmpDateTime.month >> tmpDateTime.day
             >> tmpDateTime.hour >> tmpDateTime.minutes >> tmpDateTime.seconds >> tmpDateTime.deciSeconds
             >> tmpDateTime.plusMinus >> tmpDateTime.utcHOffset >> tmpDateTime.utcMOffset;
      }
      return IppValue(tmpDateTime);
    }
    case Resolution:
    {
      IppResolution tmpResolution;
      if(!(data >>= (uint16_t)0))
      {
        data >> (uint16_t)9 >> tmpResolution.x >> tmpResolution.y >> tmpResolution.units;
      }
      return IppValue(tmpResolution);
    }
    case IntegerRange:
    {
      IppIntRange tmpIntegerRange;
      if(!(data >>= (uint16_t)0))
      {
        data >> (uint16_t)8 >> tmpIntegerRange.low >> tmpIntegerRange.high;
      }
      return IppValue(tmpIntegerRange);
    }
    case OctetStringUnknown:
    case TextWithLanguage:
    case NameWithLanguage:
    case TextWithoutLanguage:
    case NameWithoutLanguage:
    case Keyword:
    case Uri:
    case UriScheme:
    case Charset:
    case NaturalLanguage:
    case MimeMediaType:
    default:
    {
      std::string tmp_str;
      data >> tmpLen;
      data/tmpLen >> tmp_str;
      return IppValue(tmp_str);
    }
  };
}

List<IppAttr> IppMsg::getUnnamedAttributes(Bytestream& data) const
{
  uint8_t tag;
  List<IppAttr> attrs;
  while(data.remaining())
  {
    data >> tag;
    if(data >>= (uint16_t)0)
    {
      attrs.push_back(IppAttr(tag, decodeValue(tag, data)));
    }
    else
    {
      data -= 1;
      break;
    }
  }
  return attrs;
}

IppValue IppMsg::collectAttributes(List<IppAttr>& attrs) const
{
  IppOneSetOf resArr;
  IppCollection resVal;
  while(!attrs.empty())
  {
    IppAttr tmpval = attrs.takeFront();
    uint8_t tag = tmpval.tag();
    if(tag == MemberName)
    {
      std::string key = tmpval.get<std::string>();
      tmpval = attrs.takeFront();
      if(tmpval.tag() == BeginCollection)
      {
        resVal.insert({key, IppAttr(BeginCollection, collectAttributes(attrs))});
      }
      else
      { // This should be general data attributes
        IppOneSetOf restOfSet;
        while(attrs.front().tag() == tmpval.tag())
        {
          restOfSet.push_back(attrs.takeFront().value());
        }

        if(restOfSet.empty())
        {
          resVal.insert({key, tmpval});
        }
        else
        {
          restOfSet.push_front(tmpval.value());
          tmpval = IppAttr(tmpval.tag(), restOfSet);
          resVal.insert({key, tmpval});
        }
      }
    }
    else if(tag == EndCollection)
    {
      resArr.push_back(resVal);
      resVal = IppCollection();
      if(attrs.empty())
      { // end of collection
        break;
      }
      else if(attrs.front().tag() == BeginCollection)
      { // this is a 1setOf
        attrs.pop_front();
        continue;
      }
      else
      { // the following attribute(s) belong to an outer collection
        break;
      }
    }
    else
    {
      std::cerr << "out of sync with collection" << tmpval.tag();
    }
  }

  if(resArr.size()==1)
  { // The code above unconditionally produces arrays, collapse if they are just a single object
    return resArr.front();
  }
  else
  {
    return resArr;
  }
}

std::string IppMsg::consumeAttributes(IppAttrs& attrs, Bytestream& data) const
{
  uint8_t tag;
  uint16_t tmpLen;
  std::string name;

  data >> tag >> tmpLen;
  data/tmpLen >> name;

  IppValue value = decodeValue(tag, data);
  List<IppAttr> unnamed = getUnnamedAttributes(data);

  if(tag == BeginCollection)
  {
    value = collectAttributes(unnamed);
  }

  if(!unnamed.empty())
  {
    IppOneSetOf tmpOneSet;
    tmpOneSet.push_back(value);
    for(IppAttr a : unnamed)
    {
      tmpOneSet.push_back(a.value());
    }
    attrs.insert({name, IppAttr(tag, tmpOneSet)});
  }
  else
  {
    attrs.insert({name, IppAttr(tag, value)});
  }
  return name;
}

void IppMsg::encodeAttributes(Bytestream& msg, const IppAttrs& attrs) const
{
  for(std::pair<const std::string, IppAttr>  attr : attrs)
  {
    encodeAttribute(msg, attr.first, attr.second);
  }
}

void IppMsg::encodeAttribute(Bytestream& msg, std::string name, IppAttr attr, bool subCollection) const
{
  uint8_t tag = attr.tag();
  if(subCollection)
  {
    msg << MemberName << (uint16_t)0 << (uint16_t)name.length() << name;
    name = "";
  }

  msg << tag << uint16_t(name.length()) << name;

  if(attr.value().is<IppOneSetOf>())
  {
    IppOneSetOf oneSet = attr.get<IppOneSetOf>();
    encodeValue(msg, tag, oneSet.takeFront());
    while(!oneSet.empty())
    {
      msg << tag << uint16_t(0);
      encodeValue(msg, tag, oneSet.takeFront());
    }
  }
  else
  {
    encodeValue(msg, attr.tag(), attr.value());
  }
}

void IppMsg::encodeValue(Bytestream& msg, uint8_t tag, IppValue val) const
{
  switch(tag)
  {
    case OpAttrs:
    case JobAttrs:
    case EndAttrs:
    case PrinterAttrs:
      throw std::logic_error("Unexpected tag");
    case Integer:
    case Enum:
    {
      uint32_t tmp_u32 = val.get<int>();
      msg << (uint16_t)4 << tmp_u32;
      break;
    }
    case Boolean:
    {
      uint8_t tmp_u8 = val.get<bool>();
      msg << (uint16_t)1 << tmp_u8;
      break;
    }
    case DateTime:
    {
      IppDateTime tmpDateTime = val.get<IppDateTime>();
      msg << (uint16_t)11 << tmpDateTime.year << tmpDateTime.month << tmpDateTime.day
          << tmpDateTime.hour << tmpDateTime.minutes << tmpDateTime.seconds << tmpDateTime.deciSeconds
          << tmpDateTime.plusMinus << tmpDateTime.utcHOffset << tmpDateTime.utcMOffset;
      break;
    }
    case Resolution:
    {
      IppResolution tmpRes = val.get<IppResolution>();
      msg << (uint16_t)9 << tmpRes.x << tmpRes.y << tmpRes.units;
      break;
    }
    case IntegerRange:
    {
      IppIntRange tmpRange = val.get<IppIntRange>();
      msg << (uint16_t)8 << tmpRange.low << tmpRange.high;
      break;
    }
    case BeginCollection:
    {
      msg << (uint16_t)0; // length of value
      IppCollection collection = val.get<IppCollection>();
      for(auto const& x : collection)
      {
        encodeAttribute(msg, x.first, x.second, true);
      }
      msg << EndCollection << (uint16_t)0 << (uint16_t)0;
      break;
    }
    case OctetStringUnknown:
    case TextWithLanguage:
    case NameWithLanguage:
    case TextWithoutLanguage:
    case NameWithoutLanguage:
    case Keyword:
    case Uri:
    case UriScheme:
    case Charset:
    case NaturalLanguage:
    case MimeMediaType:
    {
      std::string tmpstr = val.get<std::string>();
      msg << uint16_t(tmpstr.length());
      msg.putBytes(tmpstr.data(), tmpstr.length());
      break;
    }
    default:
      std::cerr << "uncaught tag " << tag;
      throw std::logic_error("Uncaught tag");
      break;
  }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
  Bytestream bts(Data, Size);
  try
  {
    IppMsg msg(bts);
  }
  catch(const std::exception& e)
  {

  }
  return 0;
}
