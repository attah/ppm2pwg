#ifndef ARGGET_H
#define ARGGET_H
// Angry goat in Swedish (only half kidding)

#include <map>
#include <iomanip>
#include <list>
#include <sstream>
#include <string>
#include <type_traits>

class ArgBase
{
public:
  virtual bool parse(std::list<std::string>&) = 0;
  virtual std::string docName() const = 0;

  bool isSet() const
  {
    return _set;
  }

protected:
  bool _set = false;

};

class SwitchArgBase : public ArgBase
{
public:
  SwitchArgBase(std::initializer_list<std::string> names,
                std::string doc, std::string errorHint="")
  : _names(names), _doc(std::move(doc)), _errorHint(std::move(errorHint))
  {
    if(names.size() < 1)
    {
      throw(std::logic_error("No parameter definitions"));
    }
  }

  bool parse(std::list<std::string>& argv) override
  {
    if(match(argv))
    {
      if(!get_value(argv))
      {
        throw std::invalid_argument("");
      }
      _set = true;
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

  virtual bool get_value(std::list<std::string>&) = 0;

  std::string docName() const override
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
    docName << typeHint();
    return docName.str();
  }

  virtual std::string typeHint() const = 0;

  std::string doc()
  {
    return _doc;
  }
  std::string errorHint() const
  {
    return _errorHint;
  }

private:
  std::list<std::string> _names;
  std::string _doc;
  std::string _errorHint;

};

template<typename T>
class SwitchArg : public SwitchArgBase
{
public:
  SwitchArg() = delete;
  SwitchArg(const SwitchArg&) = delete;
  SwitchArg& operator=(const SwitchArg&) = delete;

  SwitchArg(T& value, std::initializer_list<std::string> names, std::string doc)
  : SwitchArgBase(names, std::move(doc)), _value(value)
  {}

private:
  bool get_value(std::list<std::string>& argv) override
  {
    if(std::is_same_v<T, bool>)
    {
      _value = true;
      return true;
    }
    else if(!argv.empty())
    {
      convert(_value, argv.front());
      argv.pop_front();
      return true;
    }
    return false;
  }

  std::string typeHint() const override
  {
    if(std::is_same_v<T, bool>)
    {
      return "";
    }
    else if(std::is_same_v<T, std::string>)
    {
      return " <string>";
    }
    else if(std::is_integral_v<T>)
    {
      return " <int>";
    }
  }

  void convert(std::string& res, const std::string& s) {res = s;}
  void convert(bool& res, const std::string&) {res = true;}
  void convert(int& res, const std::string& s) {res = std::stoi(s);}
  void convert(long& res, const std::string& s) {res = std::stol(s);}
  void convert(long long& res, const std::string& s) {res = std::stoll(s);}
  void convert(unsigned int& res, const std::string& s) {res = std::stoul(s);} // Silly C++ has no stou
  void convert(unsigned long& res, const std::string& s) {res = std::stoul(s);}
  void convert(unsigned long long& res, const std::string& s) {res = std::stoull(s);}

  T& _value;
};

template<typename T>
class EnumSwitchArg : public SwitchArgBase
{
public:
  using Mappings = std::map<std::string, T>;
  using BackMappings = std::map<T, std::string>;

  EnumSwitchArg() = delete;
  EnumSwitchArg(const EnumSwitchArg&) = delete;
  EnumSwitchArg& operator=(const EnumSwitchArg&) = delete;

  EnumSwitchArg(T& value, const Mappings& mappings, std::initializer_list<std::string> names,
                std::string doc, std::string errorHint="")
  : SwitchArgBase(names, std::move(doc), std::move(errorHint)), _value(value), _mappings(mappings)
  {
    for(const auto& [k, v] : mappings)
    {
      _backMappings[v] = k;
    }
  }

  std::string typeHint() const override
  {
    return " <choice>";
  }

  std::string getAlias(T value)
  {
    return _backMappings.at(value);
  }

private:

  bool get_value(std::list<std::string>& argv) override
  {
    if(!argv.empty())
    {
      typename std::map<std::string, T>::const_iterator it = _mappings.find(argv.front());
      if(it != _mappings.cend())
      {
        _value = it->second;
        argv.pop_front();
        return true;
      }
    }
    return false;
  }

  T& _value;
  Mappings _mappings;
  BackMappings _backMappings;
};

class PosArg : public ArgBase
{
public:
  PosArg() = delete;
  PosArg(const PosArg&) = delete;
  PosArg& operator=(const PosArg&) = delete;

  PosArg(std::string& value, std::string name, bool optional=false)
  : _value(value), _name(std::move(name)), _optional(optional)
  {}

  bool parse(std::list<std::string>& argv) override
  {
    if(!argv.empty())
    {
      _value = argv.front();
      argv.pop_front();
      _set = true;
      return true;
    }
    return _optional;
  }

  std::string docName() const override
  {
    return _optional ? "["+_name+"]" : "<"+_name+">";
  }

private:
  std::string& _value;
  std::string _name;
  bool _optional;
};

class ArgGet
{
friend class SubArgGet;

public:
  ArgGet() = default;
  ArgGet(const ArgGet&) = default;
  ArgGet& operator=(const ArgGet&) = default;

  ArgGet(std::list<SwitchArgBase*> argDefs, std::list<PosArg*> posArgDefs = {})
  : _argDefs(std::move(argDefs)), _posArgDefs(std::move(posArgDefs))
  {}

  bool get_args(int argc, char** argv)
  {
    return _get_args(argc, argv, _argDefs);
  }

  std::string argHelp()
  {
    std::string help = "Usage: " + _name + _argHelp();
    return help;
  }

  std::string errmsg()
  {
    return _errMsg;
  }

protected:

  bool _get_args(int argc, char** argv, const std::list<SwitchArgBase*>& argDefs)
  {
    std::list<std::string> argList;

    if(argc < 1)
    {
      throw(std::logic_error("No program name"));
    }
    _name = argv[0];

    for(int i = 1; i < argc; i++)
    {
      argList.emplace_back(argv[i]);
    }

    bool progress = true;
    while(progress && argList.size() != 0)
    {
      progress = false;
      for(SwitchArgBase* argDef : argDefs)
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
            _errMsg = "Missing value for " + argDef->docName();
          }
          else if(argDef->errorHint() != "")
          {
            _errMsg = argDef->errorHint()
                    + " (" + argList.front() + ")";
          }
          else
          {
            _errMsg = "Bad value for " + argDef->docName()
                    + " (" + argList.front() + ")";
          }
          return false;
        }
      }
    }

    if(argList.size() > _posArgDefs.size())
    { // Cannot consume all - fail early.
      _errMsg = "Unknown argument: " + argList.front();
      return false;
    }

    for(PosArg* posArg : _posArgDefs)
    {
      if(!posArg->parse(argList))
      {
        _errMsg = "Missing positional argument " + (posArg->docName());
        return false;
      }
    }

    if(argList.empty())
    {
      return true;
    }
    else
    {
      _errMsg = "Unknown argument: " + argList.front();
      return false;
    }
  }

  std::string _argHelp(bool commonOptions=false) const
  {
    std::stringstream help;

    if(!_argDefs.empty() || commonOptions)
    {
      help << " [options]";
    }
    for(PosArg* posArg : _posArgDefs)
    {
      help << " " << posArg->docName();
    }
    help << std::endl;
    help << _argDefHelp(_argDefs);
    return help.str();
  }

  static std::string _argDefHelp(const std::list<SwitchArgBase*>& argDefs)
  {
    std::stringstream help;
    size_t w = 0;

    for(SwitchArgBase* argDef : argDefs)
    {
      w = std::max(w, argDef->docName().length());
    }
    for(SwitchArgBase* argDef : argDefs)
    {
      help << "  " << std::left << std::setw(w) << argDef->docName() << "    "
           << argDef->doc() << std::endl;
    }
    return help.str();
  }

private:
  std::string _name;
  std::list<SwitchArgBase*> _argDefs;
  std::list<PosArg*> _posArgDefs;
  std::string _errMsg;

};

class SubArgGet
{
public:
  SubArgGet() = delete;
  SubArgGet(const SubArgGet&) = delete;
  SubArgGet& operator=(const SubArgGet&) = delete;

  using SubArgs = std::list<std::pair<std::string,std::pair<std::list<SwitchArgBase*>,std::list<PosArg*>>>>;

  SubArgGet(const SubArgs& subArgs) : SubArgGet({}, subArgs)
  {}
  SubArgGet(std::list<SwitchArgBase*> commonArgDefs, const SubArgs& subArgs)
  : _commonArgDefs(std::move(commonArgDefs))
  {
    for(const auto& [subCommand, args] : subArgs)
    {
      _order.emplace_back(subCommand);
      _subCommands[subCommand] = ArgGet(args.first, args.second);
    }
  }

  bool get_args(int argc, char** argv)
  {
    std::list<std::string> argList;

    if(argc < 1)
    {
      throw(std::logic_error("No program name"));
    }
    _name = argv[0];

    for(int i = 1; i < argc; i++)
    {
      argList.emplace_back(argv[i]);
    }

    if(argc < 2)
    {
      _errMsg = "No sub-command given";
      return false;
    }
    _subCommand = argv[1];
    if(_subCommands.find(_subCommand) == _subCommands.cend())
    {
      _errMsg = "Invalid sub-command";
      return false;
    }

    ArgGet& argGet = _subCommands[_subCommand];
    std::list<SwitchArgBase*> allArgDefs = _commonArgDefs;
    allArgDefs.insert(allArgDefs.cend(), argGet._argDefs.cbegin(), argGet._argDefs.cend());
    if(!argGet._get_args(argc-1, &argv[1], allArgDefs))
    {
      _errMsg = argGet.errmsg();
      return false;
    }

    return true;
  }

  std::string argHelp(const std::string& subCommand="")
  {
    std::stringstream help;

    help << "Usage: " << _name << " <sub-command> [options]" << std::endl << std::endl;
    if(_subCommands.find(subCommand) != _subCommands.cend())
    {
      help << _name << " " << subCommand << _subCommands[subCommand]._argHelp(true) << std::endl;
    }
    else
    {
      for(const std::string& subCommand : _order)
      {
        help << _name << " " << subCommand << _subCommands.at(subCommand)._argHelp(true) << std::endl;
      }
    }
    if(!_commonArgDefs.empty())
    {
      help << "Common options for all sub-commands:" << std::endl;
      help << ArgGet::_argDefHelp(_commonArgDefs) << std::endl;
    }
    return help.str();
  }

  std::string errmsg()
  {
    return _errMsg;
  }

  std::string subCommand()
  {
    return _subCommand;
  }

private:
  std::string _name;
  std::string _subCommand;
  std::list<SwitchArgBase*> _commonArgDefs;
  std::map<std::string, ArgGet> _subCommands;
  std::list<std::string> _order;
  std::string _errMsg;

};

#endif //ARGGET_H
