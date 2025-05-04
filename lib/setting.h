#ifndef SETTING_H
#define SETTING_H

#include "ippattr.h"

#include <sstream>

template <typename T>
class Setting
{
public:
  Setting() = delete;
  Setting(Setting&) = delete;
  Setting& operator=(const Setting&) = delete;

  Setting(IppAttrs* printerAttrs, IppAttrs* attrs, IppTag tag,
          std::string name, std::string subKey = "")
  : _printerAttrs(printerAttrs), _attrs(attrs), _tag(tag),
    _name(std::move(name)), _subKey(std::move(subKey))
  {}

  std::string name()
  {
    return _name;
  }

  virtual bool isSupported() const
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
    return _printerAttrs->get<T>(_name+"-default", std::move(fallback));
  }

  virtual bool isSupportedValue(T value) const = 0;

  void set(const T& value)
  {
    if(_subKey != "")
    {
      IppCollection col;
      if(_attrs->has(_subKey))
      {
        col = _attrs->at(_subKey).get<IppCollection>();
      }
      col.set(_name, IppAttr(_tag, value));
      _attrs->set(_subKey, IppAttr(IppTag::BeginCollection, col));
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
        _attrs->set(_subKey, IppAttr(IppTag::BeginCollection, col));
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
      return getDefault(std::move(fallback));
    }
  }

protected:
  IppAttrs* _printerAttrs;
  IppAttrs* _attrs;
  IppTag _tag;
  std::string _name;
  std::string _subKey;
};

template <typename T>
class ChoiceSetting : public Setting<T>
{
public:
  ChoiceSetting(IppAttrs* printerAttrs, IppAttrs* attrs, IppTag tag,
                std::string name, std::string subKey = "")
  : Setting<T>(printerAttrs, attrs, tag, std::move(name), std::move(subKey))
  {}

  bool isSupportedValue(T value) const override
  {
    return getSupported().contains(value);
  }

  List<T> getSupported() const
  {
    return _getSupported(T());
  }

private:
  template <typename U>
  List<U> _getSupported(const U&) const
  {
    IppAttrs* pa = this->_printerAttrs;
    return pa->getList<U>(this->_name+"-supported");
  }

  List<int> _getSupported(int) const
  {
    IppAttrs* pa = this->_printerAttrs;
    if(pa->has(this->_name+"-supported"))
    {
      IppAttr val = pa->at(this->_name+"-supported");
      if(val.is<IppIntRange>())
      {
        List<int> res;
        IppIntRange range = val.get<IppIntRange>();
        for(int i = range.low; i <= range.high; i++)
        {
          res.push_back(i);
        }
        return res;
      }
    }
    return pa->getList<int>(this->_name+"-supported");
  }

};

template <typename T>
class PreferredChoiceSetting : public ChoiceSetting<T>
{
public:
  PreferredChoiceSetting(IppAttrs* printerAttrs, IppAttrs* attrs, IppTag tag,
                         std::string name, std::string pref)
  : ChoiceSetting<T>(printerAttrs, attrs, tag, std::move(name)), _pref(std::move(pref))
  {}

  List<T> getPreferred() const
  {
    List<T> preferred;
    if(this->_printerAttrs->has(this->_name+"-"+_pref))
    {
      IppAttrs* pa = this->_printerAttrs;
      preferred = pa->getList<T>(this->_name+"-"+_pref);
    }
    return preferred;
  }

  std::string supportedStr() override
  {
    if(this->_printerAttrs->has(this->_name+"-supported"))
    {
      std::stringstream ss;
      IppOneSetOf supported = this->_printerAttrs->at(this->_name+"-supported").asList();
      IppOneSetOf preferred;
      if(this->_printerAttrs->has(this->_name+"-"+_pref))
      {
        preferred = this->_printerAttrs->at(this->_name+"-"+_pref).asList();
      }
      for(IppOneSetOf::const_iterator it = supported.cbegin(); it != supported.cend(); it++)
      {
        ss << *it;
        if(preferred.contains(*it))
        {
          ss << "(" << _pref << ")";
        }
        if(std::next(it) != supported.cend())
        {
          ss << ", ";
        }
      }
      return ss.str();
    }
    else
    {
      return "unsupported";
    }
  }

private:
  std::string _pref;
};

class IntegerSetting : public Setting<int>
{
public:
  IntegerSetting(IppAttrs* printerAttrs, IppAttrs* attrs, IppTag tag, std::string name)
  : Setting<int>(printerAttrs, attrs, tag, std::move(name))
  {}

  bool isSupportedValue(int value) const override
  {
    return isSupported() && value >= getSupportedMin() && value <= getSupportedMax();
  }

  int getSupportedMin() const
  {
    return _printerAttrs->get<IppIntRange>(this->_name+"-supported").low;
  }

  int getSupportedMax() const
  {
    return _printerAttrs->get<IppIntRange>(this->_name+"-supported").high;
  }
};

class IntegerRangeListSetting : public Setting<IppOneSetOf>
{
public:
  IntegerRangeListSetting(IppAttrs* printerAttrs, IppAttrs* attrs, IppTag tag, std::string name)
  : Setting<IppOneSetOf>(printerAttrs, attrs, tag, std::move(name))
  {}

  bool isSupportedValue(IppOneSetOf) const override
  {
    return isSupported();
  }

  bool getSupported() const
  {
    return _printerAttrs->get<bool>(this->_name+"-supported");
  }
};

#endif // SETTING_H
