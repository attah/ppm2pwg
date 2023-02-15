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

  virtual std::string docname() const = 0;
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
        throw std::invalid_argument("foo");
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

  std::string docname() const
  {
    std::stringstream docname;
    std::string sep = ", ";
    std::list<std::string>::const_iterator it = _names.cbegin();
    docname << *it;
    it++;
    for(; it != _names.cend(); it++)
    {
      docname << sep << *it;
    }
    docname << hint();
    return docname.str();
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

  std::string docname() const
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

  ArgGet(std::initializer_list<ArgBase*> argdefs,
         std::initializer_list<PosArg*> posargdefs = {})
  : _argdefs(argdefs), _posargdefs(posargdefs)
  {}

  bool get_args(int argc, char** argv)
  {
    std::list<std::string> arglist;
    _errmsg = std::stringstream();

    if(argc < 1)
    {
      throw(std::logic_error("No program name"));
    }
    _name = argv[0];

    for(int i = 1; i < argc; i++)
    {
      arglist.push_back(argv[i]);;
    }

    bool progress = true;
    while(progress && arglist.size() != 0)
    {
      progress = false;
      for(ArgBase* argdef : _argdefs)
      {
        try
        {
          if(argdef->parse(arglist))
          {
            progress = true;
            break;
          }
        }
        catch(std::exception& e)
        {
          if(arglist.size() == 0)
          {
            _errmsg << "Missing value for " << argdef->docname();
          }
          else
          {
            _errmsg << "Bad value for " << argdef->docname()
                    << " (" << arglist.front() << ")";
          }
          return false;
        }
      }
    }

    for(PosArg* posarg : _posargdefs)
    {
      if(!posarg->parse(arglist))
      {
        _errmsg << "Missing positional argument " << (posarg->docname());
        return false;
      }
    }

    if(arglist.empty())
    {
      return true;
    }
    else
    {
      _errmsg << "Unknown argument: " << arglist.front();
      return false;
    }
  }

  std::string arghelp()
  {
    std::stringstream help;
    size_t w = 0;

    help << "Usage: " << _name << " [options]";
    for(PosArg* posarg : _posargdefs)
    {
      help << " " << posarg->docname();
    }
    help << std::endl;

    for(ArgBase* argdef : _argdefs)
    {
      w = std::max(w, argdef->docname().length());
    }
    for(ArgBase* argdef : _argdefs)
    {
      help << "  " << std::left << std::setw(w) << argdef->docname() << "    "
           << argdef->doc() << std::endl;
    }
    return help.str();
  }

  std::string name()
  {
    return _name;
  }

  std::string errmsg()
  {
    return _errmsg.str();
  }

private:
  std::string _name;
  std::list<ArgBase*> _argdefs;
  std::list<PosArg*> _posargdefs;
  std::stringstream _errmsg;
};

#endif //ARGGET_H
