#include "ippmsg.h"

#include <iostream>

uint32_t IppMsg::_reqId=1;

IppMsg::IppMsg(Bytestream& msg)
{
  uint32_t reqId;

  msg >> _majVsn >> _minVsn >> _opOrStatus >> reqId;

  IppAttrs attrs;
  IppTag currentAttrType = IppTag::EndAttrs;

  while(!msg.atEnd())
  {
    if(msg.peek<uint8_t>() <= (uint8_t)IppTag::UnsupportedAttrs)
    {
      if(currentAttrType == IppTag::OpAttrs)
      {
        _opAttrs = attrs;
      }
      else if(currentAttrType == IppTag::JobAttrs)
      {
        _jobAttrs.push_back(attrs);
      }
      else if(currentAttrType == IppTag::PrinterAttrs)
      {
        _printerAttrs = attrs;
      }
      else if(currentAttrType == IppTag::UnsupportedAttrs)
      {
#ifndef FUZZ // Too much spam for fuzzing
        std::cerr << "WARNING: unsupported attrs reported: " << std::endl
                  << attrs.toJSON().dump();
#endif
      }

      if(msg >>= (uint8_t)IppTag::EndAttrs)
      {
        break;
      }

      currentAttrType = (IppTag)msg.get<uint8_t>();
      attrs = IppAttrs();
    }
    else
    {
      consumeAttributes(attrs, msg);
    }
  }
}

IppMsg::IppMsg(uint16_t opOrStatus, const IppAttrs& opAttrs, const IppAttrs& jobAttrs, const IppAttrs& printerAttrs)
{
  _opOrStatus = opOrStatus;
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
  ipp << (uint8_t)IppTag::OpAttrs;

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
  for(const IppAttrs& attrs : _jobAttrs)
  {
    if(!attrs.empty())
    {
      ipp << (uint8_t)IppTag::JobAttrs;
      encodeAttributes(ipp, attrs);
    }
  }
  if(!_printerAttrs.empty())
  {
    ipp << (uint8_t)IppTag::PrinterAttrs;
    encodeAttributes(ipp, _printerAttrs);
  }
  ipp << (uint8_t)IppTag::EndAttrs;
  return ipp;
}

void IppMsg::setOpAttr(const std::string& name, const IppAttr& attr)
{
  _opAttrs.insert({name, attr});
}

void IppMsg::setVersion(uint8_t majVsn, uint8_t minVsn)
{
  _majVsn = majVsn;
  _minVsn = minVsn;
}

IppAttrs IppMsg::baseOpAttrs(const std::string& url)
{
  char* user = getenv("USER");
  std::string name = user != nullptr ? user : "anonymous";
  IppAttrs o
  {
    {"attributes-charset",          IppAttr(IppTag::Charset,             "utf-8")},
    {"attributes-natural-language", IppAttr(IppTag::NaturalLanguage,     "en-us")},
    {"printer-uri",                 IppAttr(IppTag::Uri,                 url)},
    {"requesting-user-name",        IppAttr(IppTag::NameWithoutLanguage, name)},
  };
  return o;
}

IppValue IppMsg::decodeValue(IppTag tag, Bytestream& data) const
{
  switch(tag)
  {
    case IppTag::OpAttrs:
    case IppTag::JobAttrs:
    case IppTag::EndAttrs:
    case IppTag::PrinterAttrs:
    case IppTag::UnsupportedAttrs:
      throw std::logic_error("Unexpected tag");
    case IppTag::Integer:
    case IppTag::Enum:
    {
      uint32_t tmpU32=0;
      if(!(data >>= (uint16_t)0))
      {
        data >> (uint16_t)4 >> tmpU32;
      }
      return IppValue((int)tmpU32);
    }
    case IppTag::Boolean:
    {
      uint8_t tmpBool=0;
      if(!(data >>= (uint16_t)0))
      {
        data >> (uint16_t)1 >> tmpBool;
      }
      return IppValue((bool)tmpBool);
    }
    case IppTag::DateTime:
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
    case IppTag::Resolution:
    {
      IppResolution tmpResolution;
      if(!(data >>= (uint16_t)0))
      {
        data >> (uint16_t)9 >> tmpResolution.x >> tmpResolution.y >> tmpResolution.units;
      }
      return IppValue(tmpResolution);
    }
    case IppTag::IntegerRange:
    {
      IppIntRange tmpIntegerRange;
      if(!(data >>= (uint16_t)0))
      {
        data >> (uint16_t)8 >> tmpIntegerRange.low >> tmpIntegerRange.high;
      }
      return IppValue(tmpIntegerRange);
    }
    case IppTag::OctetStringUnknown:
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
    default:
    {
      return IppValue(data.getString(data.get<uint16_t>()));
    }
  };
}

List<IppAttr> IppMsg::getUnnamedAttributes(Bytestream& data) const
{
  IppTag tag;
  List<IppAttr> attrs;
  while(data.remaining())
  {
    tag = (IppTag)data.get<uint8_t>();
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
    IppTag tag = tmpval.tag();
    if(tag == IppTag::MemberName)
    {
      std::string key = tmpval.get<std::string>();
      tmpval = attrs.takeFront();
      if(tmpval.tag() == IppTag::BeginCollection)
      {
        resVal.insert({key, IppAttr(IppTag::BeginCollection, collectAttributes(attrs))});
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
    else if(tag == IppTag::EndCollection)
    {
      resArr.push_back(resVal);
      resVal = IppCollection();
      if(attrs.empty())
      { // end of collection
        break;
      }
      else if(attrs.front().tag() == IppTag::BeginCollection)
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
      throw std::logic_error("out of sync with collection");
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
  IppTag tag = (IppTag)data.get<uint8_t>();
  std::string name;

  name = data.getString(data.get<uint16_t>());

  IppValue value = decodeValue(tag, data);
  List<IppAttr> unnamed = getUnnamedAttributes(data);

  if(tag == IppTag::BeginCollection)
  {
    value = collectAttributes(unnamed);
  }

  if(!unnamed.empty())
  {
    IppOneSetOf tmpOneSet;
    tmpOneSet.push_back(value);
    for(const IppAttr& a : unnamed)
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
  for(const auto& [name, attr] : attrs)
  {
    encodeAttribute(msg, name, attr);
  }
}

void IppMsg::encodeAttribute(Bytestream& msg, std::string name, const IppAttr& attr, bool subCollection) const
{
  IppTag tag = attr.tag();
  if(subCollection)
  {
    msg << (uint8_t)IppTag::MemberName << (uint16_t)0 << (uint16_t)name.length() << name;
    name = "";
  }

  msg << (uint8_t)tag << uint16_t(name.length()) << name;

  if(attr.value().is<IppOneSetOf>())
  {
    IppOneSetOf oneSet = attr.get<IppOneSetOf>();
    encodeValue(msg, tag, oneSet.takeFront());
    while(!oneSet.empty())
    {
      msg << (uint8_t)tag << uint16_t(0);
      encodeValue(msg, tag, oneSet.takeFront());
    }
  }
  else
  {
    encodeValue(msg, attr.tag(), attr.value());
  }
}

void IppMsg::encodeValue(Bytestream& msg, IppTag tag, const IppValue& val) const
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
      uint32_t tmpU32 = val.get<int>();
      msg << (uint16_t)4 << tmpU32;
      break;
    }
    case IppTag::Boolean:
    {
      uint8_t tmpU8 = val.get<bool>();
      msg << (uint16_t)1 << tmpU8;
      break;
    }
    case IppTag::DateTime:
    {
      IppDateTime tmpDateTime = val.get<IppDateTime>();
      msg << (uint16_t)11 << tmpDateTime.year << tmpDateTime.month << tmpDateTime.day
          << tmpDateTime.hour << tmpDateTime.minutes << tmpDateTime.seconds << tmpDateTime.deciSeconds
          << tmpDateTime.plusMinus << tmpDateTime.utcHOffset << tmpDateTime.utcMOffset;
      break;
    }
    case IppTag::Resolution:
    {
      IppResolution tmpRes = val.get<IppResolution>();
      msg << (uint16_t)9 << tmpRes.x << tmpRes.y << tmpRes.units;
      break;
    }
    case IppTag::IntegerRange:
    {
      IppIntRange tmpRange = val.get<IppIntRange>();
      msg << (uint16_t)8 << tmpRange.low << tmpRange.high;
      break;
    }
    case IppTag::BeginCollection:
    {
      msg << (uint16_t)0; // length of value
      IppCollection collection = val.get<IppCollection>();
      for(const auto& [name, attr] : collection)
      {
        encodeAttribute(msg, name, attr, true);
      }
      msg << (uint8_t)IppTag::EndCollection << (uint16_t)0 << (uint16_t)0;
      break;
    }
    case IppTag::OctetStringUnknown:
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
      std::string tmpstr = val.get<std::string>();
      msg << uint16_t(tmpstr.length());
      msg.putBytes(tmpstr.data(), tmpstr.length());
      break;
    }
    default:
      std::cerr << "uncaught tag " << +(uint8_t)tag;
      throw std::logic_error("Uncaught tag");
  }
}

#ifdef FUZZ
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size)
{
  Bytestream bts(Data, Size);
  try
  {
    IppMsg msg(bts);
    msg.getPrinterAttrs().toJSON();
  }
  catch(const std::exception& e)
  {
  }
  return 0;
}
#endif
