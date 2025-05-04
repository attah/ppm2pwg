#ifndef LOG_H
#define LOG_H

#include <map>

#define LOG(category, ...) if(LogController::instance().isEnabled(category))\
                           {std::cerr __VA_ARGS__ << std::endl;}
#define DBG(...) LOG(LogController::Debug, __VA_ARGS__)

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
    Debug
  };

  void enable(Category category)
  {
    _enabled[category] = true;
  }

  void disable(Category category)
  {
    _enabled[category] = false;
  }

  bool isEnabled(Category category)
  {
    return _enabled[category];
  }

private:
  std::map<Category, bool> _enabled;

};

#endif // LOG_H
