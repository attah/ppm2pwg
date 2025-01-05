#ifndef LTHREADH_H
#define LTHREADH_H

#include <functional>
#include <thread>

class LThread
{
public:
  LThread() = default;
  ~LThread()
  {
    await();
  }
  LThread(const LThread&) = delete;
  LThread& operator=(const LThread&) = delete;

  using runnable = std::function<void()>;

  void run(runnable lambda)
  {
    _lambda = std::move(lambda);
    _complete = false;
    _thread = std::thread([this](){_lambda(); _complete = true;});
  }

  bool isRunning() const
  {
    return !_complete;
  }

  void await()
  {
    if(_thread.joinable())
    {
      _thread.join();
    }
  }

private:
  std::thread _thread;
  runnable _lambda;
  bool _complete = false;

};

#endif // LTHREADH_H
