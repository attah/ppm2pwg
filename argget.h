#ifndef ARGGET_H
#define ARGGET_H
// Angry goat in Swedish (only half kidding)

#include <string>
#include <sstream>
#include <list>
#include <iomanip>
#include <type_traits>

class ArgBase
{
public:

  virtual bool parse(std::list<std::string>&) = 0;

  virtual std::string docName() const = 0;
  virtual std::string doc() const = 0;

private:

};

template<typename T>
class SwitchArg : public ArgBase
{
public:
  SwitchArg() = delete;
  SwitchArg(const SwitchArg&) = delete;
  SwitchArg& operator=(const SwitchArg&) = delete;

  SwitchArg(T& value, std::initializer_list<std::string> names, std::string doc)
  : _value(value), _names(names), _doc(doc)
  {
    if(names.size() < 1)
    {
      throw(std::logic_error("No parameter definitions"));
    }
  }

  bool parse(std::list<std::string>& argv)
  {
    if(match(argv))
    {
      if(!get_value(_value, argv))
      {
        throw std::invalid_argument("");
      }
      return true;
    }
    return false;
  }

  bool match(std::list<std::string>& argv)
  {
    for(const std::string& name: _names)
    {
      if(argv.front() == name)
      {
        argv.pop_front();
        return true;
      }
    }
    return false;
  }

  std::string docName() const
  {
    std::stringstream docName;
    std::string sep = ", ";
    std::list<std::string>::const_iterator it = _names.cbegin();
    docName << *it;
    it++;
    for(; it != _names.cend(); it++)
    {
      docName << sep << *it;
    }
    docName << hint();
    return docName.str();
  }

  std::string doc() const
  {
    return _doc;
  }

private:
  bool get_value(bool& value, std::list<std::string>&)
  {
    value = true;
    return true;
  }
  bool get_value(std::string& value, std::list<std::string>& argv)
  {
    if(!argv.empty())
    {
      value = argv.front();
      argv.pop_front();
      return true;
    }
    return false;
  }
  template<typename TT>
  bool get_value(TT& value, std::list<std::string>& argv)
  {
    if(!argv.empty())
    {
      convert(value, argv.front());
      argv.pop_front();
      return true;
    }
    return false;
  }

  std::string hint() const
  {
    if(std::is_same<T, bool>::value)
    {
      return "";
    }
    else if(std::is_same<T, std::string>::value)
    {
      return " <string>";
    }
    else if(std::is_integral<T>::value)
    {
      return " <int>";
    }
  }

  void convert(int& res, std::string s) {res = std::stoi(s);}
  void convert(long& res, std::string s) {res = std::stol(s);}
  void convert(long long& res, std::string s) {res = std::stoll(s);}
  void convert(unsigned int& res, std::string s) {res = std::stoul(s);} // Silly C++ has no stou
  void convert(unsigned long& res, std::string s) {res = std::stoul(s);}
  void convert(unsigned long long& res, std::string s) {res = std::stoull(s);}

  T& _value;
  std::list<std::string> _names;
  std::string _doc;
};

class PosArg : public ArgBase
{
public:
  PosArg() = delete;
  PosArg(const PosArg&) = delete;
  PosArg& operator=(const PosArg&) = delete;

  PosArg(std::string& value, std::string name, bool optional=false)
  : _value(value), _name(name), _optional(optional)
  {}

  bool parse(std::list<std::string>& argv)
  {
    if(!argv.empty())
    {
      _value = argv.front();
      argv.pop_front();
      return true;
    }
    return _optional;
  }

  std::string docName() const
  {
    return _optional ? "["+_name+"]" : "<"+_name+">";
  }

  std::string doc() const
  {
    return "";
  }

private:
  std::string& _value;
  std::string _name;
  bool _optional;
};

class ArgGet
{
public:
  ArgGet() = delete;
  ArgGet(const ArgGet&) = delete;
  ArgGet& operator=(const ArgGet&) = delete;

  ArgGet(std::initializer_list<ArgBase*> argDefs,
         std::initializer_list<PosArg*> posArgDefs = {})
  : _argDefs(argDefs), _posArgDefs(posArgDefs)
  {}

  bool get_args(int argc, char** argv)
  {
    std::list<std::string> argList;
    _errMsg = std::stringstream();

    if(argc < 1)
    {
      throw(std::logic_error("No program name"));
    }
    _name = argv[0];

    for(int i = 1; i < argc; i++)
    {
      argList.push_back(argv[i]);
    }

    bool progress = true;
    while(progress && argList.size() != 0)
    {
      progress = false;
      for(ArgBase* argDef : _argDefs)
      {
        try
        {
          if(argDef->parse(argList))
          {
            progress = true;
            break;
          }
        }
        catch(const std::exception&)
        {
          if(argList.size() == 0)
          {
            _errMsg << "Missing value for " << argDef->docName();
          }
          else
          {
            _errMsg << "Bad value for " << argDef->docName()
                    << " (" << argList.front() << ")";
          }
          return false;
        }
      }
    }

    if(argList.size() > _posArgDefs.size())
    { // Cannot consume all - fail early.
      _errMsg << "Unknown argument: " << argList.front();
      return false;
    }

    for(PosArg* posArg : _posArgDefs)
    {
      if(!posArg->parse(argList))
      {
        _errMsg << "Missing positional argument " << (posArg->docName());
        return false;
      }
    }

    if(argList.empty())
    {
      return true;
    }
    else
    {
      _errMsg << "Unknown argument: " << argList.front();
      return false;
    }
  }

  std::string argHelp()
  {
    std::stringstream help;
    size_t w = 0;

    help << "Usage: " << _name << " [options]";
    for(PosArg* posArg : _posArgDefs)
    {
      help << " " << posArg->docName();
    }
    help << std::endl;

    for(ArgBase* argDef : _argDefs)
    {
      w = std::max(w, argDef->docName().length());
    }
    for(ArgBase* argDef : _argDefs)
    {
      help << "  " << std::left << std::setw(w) << argDef->docName() << "    "
           << argDef->doc() << std::endl;
    }
    return help.str();
  }

  std::string name()
  {
    return _name;
  }

  std::string errmsg()
  {
    return _errMsg.str();
  }

private:
  std::string _name;
  std::list<ArgBase*> _argDefs;
  std::list<PosArg*> _posArgDefs;
  std::stringstream _errMsg;
};

#endif //ARGGET_H
