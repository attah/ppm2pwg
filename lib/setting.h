#ifndef SETTING_H
#define SETTING_H

#include <sstream>

template <typename T>
class Setting
{
public:
  Setting() = delete;
  Setting& operator=(const Setting& other) = delete;
  Setting(IppAttrs* printerAttrs, IppAttrs* attrs, IppMsg::Tag tag, std::string name, std::string subKey = "")
  : _printerAttrs(printerAttrs), _attrs(attrs), _tag(tag), _name(name), _subKey(subKey)
  {}

  std::string name()
  {
    return _name;
  }

  virtual bool isSupported()
  {
    return _printerAttrs->has(_name+"-supported");
  }

  virtual std::string supportedStr()
  {
    if(_printerAttrs->has(_name+"-supported"))
    {
      std::stringstream ss;
      ss << _printerAttrs->at(_name+"-supported");
      return ss.str();
    }
    else
    {
      return "unsupported";
    }
  }

  T getDefault(T fallback=T()) const
  {
    return _printerAttrs->get<T>(_name+"-default", fallback);
  }

  virtual bool isSupportedValue(T value) = 0;

  void set(T value)
  {
    if(_subKey != "")
    {
      IppCollection col;
      if(_attrs->has(_subKey))
      {
        col = _attrs->at(_subKey).get<IppCollection>();
      }
      col.set(_name, IppAttr(_tag, value));
      _attrs->set(_subKey, IppAttr(IppMsg::BeginCollection, col));
    }
    else
    {
      _attrs->set(_name, IppAttr(_tag, value));
    }
  }

  bool isSet() const
  {
    if(_subKey != "")
    {
      return _attrs->has(_subKey) && _attrs->at(_subKey).get<IppCollection>().has(_name);
    }
    else
    {
      return _attrs->has(_name);
    }
  }

  void unset()
  {
    if(_subKey != "")
    {
      IppCollection col;
      if(_attrs->has(_subKey))
      {
        col = _attrs->at(_subKey).get<IppCollection>();
      }
      col.erase(_name);
      if(col.empty())
      {
        _attrs->erase(_subKey);
      }
      else
      {
        _attrs->set(_subKey, IppAttr(IppMsg::BeginCollection, col));
      }
    }
    else
    {
      _attrs->erase(_name);
    }
  }

  T get(T fallback=T()) const
  {
    if(isSet())
    {
      if(_subKey != "")
      {
        return _attrs->at(_subKey).get<IppCollection>().at(_name).get<T>();
      }
      else
      {
        return _attrs->at(_name).get<T>();
      }
    }
    else
    {
      return getDefault(fallback);
    }
  }

protected:
  IppAttrs* _printerAttrs;
  IppAttrs* _attrs;
  IppMsg::Tag _tag;
  std::string _name;
  std::string _subKey;
};

template <typename T>
class ChoiceSetting : public Setting<T>
{
public:
  ChoiceSetting(IppAttrs* printerAttrs, IppAttrs* attrs, IppMsg::Tag tag, std::string name, std::string subKey = "")
  : Setting<T>(printerAttrs, attrs, tag, name, subKey)
  {}

  virtual bool isSupportedValue(T value)
  {
    return getSupported().contains(value);
  }

  List<T> getSupported() const
  {
    IppAttrs* pa = this->_printerAttrs;
    return pa->getList<T>(this->_name+"-supported");
  }
};

template <typename T>
class PreferredChoiceSetting : public ChoiceSetting<T>
{
public:
  PreferredChoiceSetting(IppAttrs* printerAttrs, IppAttrs* attrs, IppMsg::Tag tag, std::string name, std::string pref)
  : ChoiceSetting<T>(printerAttrs, attrs, tag, name), _pref(pref)
  {}

  List<T> getPreferred() const
  {
    IppAttrs* pa = this->_printerAttrs;
    return pa->getList<T>(this->_name+"-"+_pref);
  }

private:
  std::string _pref;
};

class IntegerSetting : public Setting<int>
{
public:
  IntegerSetting(IppAttrs* printerAttrs, IppAttrs* attrs, IppMsg::Tag tag, std::string name)
  : Setting<int>(printerAttrs, attrs, tag, name)
  {}

  virtual bool isSupportedValue(int value)
  {
    return isSupported() && value >= getSupportedMin() && value <= getSupportedMax();
  }

  int getSupportedMin()
  {
    return _printerAttrs->get<IppIntRange>(this->_name+"-supported").low;
  }

  int getSupportedMax()
  {
    return _printerAttrs->get<IppIntRange>(this->_name+"-supported").high;
  }
};

class IntegerRangeListSetting : public Setting<IppOneSetOf>
{
public:
  IntegerRangeListSetting(IppAttrs* printerAttrs, IppAttrs* attrs, IppMsg::Tag tag, std::string name)
  : Setting<IppOneSetOf>(printerAttrs, attrs, tag, name)
  {}

  virtual bool isSupportedValue(IppOneSetOf)
  {
    return isSupported();
  }

  bool getSupported() const
  {
    return _printerAttrs->get<bool>(this->_name+"-supported");
  }
};

class IntegerChoiceSetting : public Setting<int>
{
public:
  IntegerChoiceSetting(IppAttrs* printerAttrs, IppAttrs* attrs, IppMsg::Tag tag, std::string name)
  : Setting<int>(printerAttrs, attrs, tag, name)
  {}

  virtual bool isSupportedValue(int value)
  {
    return getSupported().contains(value);
  }

  List<int> getSupported() const
  {
    List<int> res;
    if(_printerAttrs->has(_name+"-supported") &&
        _printerAttrs->at(_name+"-supported").is<IppIntRange>())
    {
      IppIntRange range = _printerAttrs->at(_name+"-supported").get<IppIntRange>();
      for(int i = range.low; i <= range.high; i++)
      {
        res.push_back(i);
      }
    }
    else
    {
      res = _printerAttrs->getList<int>(_name+"-supported");
    }
    return res;
  }
};

#endif // SETTING_H
