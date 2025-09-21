#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <set>

#define LOG(category, ...) if(LogController::instance().isEnabled(category))\
                           {std::cerr __VA_ARGS__ << std::endl;}
#define DBG(...) LOG(LogController::Debug, __VA_ARGS__)
#define WARN(...) LOG(LogController::Warning, __VA_ARGS__)
#define ERROR(...) LOG(LogController::Error, __VA_ARGS__)

class LogController
{
private:
  LogController() {}
public:
  LogController(const LogController&) = delete;
  LogController& operator=(const LogController &) = delete;
  LogController(LogController &&) = delete;
  LogController & operator=(LogController &&) = delete;

  static LogController& instance()
  {
    static LogController logController;
    return logController;
  }

  enum Category
  {
    Debug,
    Warning,
    Error
  };

  void enable(Category category)
  {
    _enabled.insert(category);
  }

  void disable(Category category)
  {
    _enabled.erase(category);
  }

  bool isEnabled(Category category)
  {
    return _enabled.find(category) != _enabled.cend();
  }

private:
  std::set<Category> _enabled {Warning, Error};

};

#endif // LOG_H
