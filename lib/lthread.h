#include <thread>
#include <functional>

class LThread
{
public:
  LThread()
  {}
  ~LThread()
  {
    await();
  }
  LThread(const LThread&) = delete;
  LThread& operator=(const LThread&) = delete;

  typedef std::function<void()> runnable;

  void run(runnable lambda)
  {
    _lambda = lambda;
    _complete = false;
    _thread = std::thread([this](){_lambda(); _complete = true;});
  }

  bool isRunning()
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