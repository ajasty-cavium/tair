#ifndef TAIR_RPC_THREAD_HPP_
#define TAIR_RPC_THREAD_HPP_

#include <string>
#include <vector>
#include <sys/prctl.h>
#include <cassert>
#include <pthread.h>

namespace tair
{
namespace rpc 
{

template<typename T = void *>
class RpcThread
{
 public:
  RpcThread() : worker_count_(0), stop_(false) {}
  virtual ~RpcThread() {}

  virtual void Start()
  {
    assert(worker_args_.size() >= (size_t)worker_count_);
    for(int32_t i = 0; i < worker_count_; ++i)
    {
      pthread_t pid;
      ThreadArg *arg = new ThreadArg();
      arg->inst   = this;
      arg->index  = i;
      pthread_create(&pid, NULL, RpcThread<T>::ThreadFunc, arg);
      pids_.push_back(pid);
    }
  }

  virtual void Stop()
  {
    stop_ = true;
  }

  virtual void Wait()
  {
    for (int32_t i = 0; i < worker_count_; ++i)
    {
      pthread_join(pids_[i], NULL);
    }
  }

  inline bool IsStop() { return stop_; };

 protected:
  virtual void Run(T &arg, int32_t index) = 0;

  virtual std::string thread_name() const = 0;

  void InitializeWorker(int32_t worker_count, const std::vector<T> &worker_args)
  {
    assert(worker_args.size() >= (size_t)worker_count);
    this->worker_count_   = worker_count;
    this->worker_args_    = worker_args;
  }

  std::vector<T> &worker_args() { return worker_args_; }

 private:
  struct ThreadArg
  {
    RpcThread *inst;
    int32_t   index;
  };

  static void* ThreadFunc(void *arg)
  {
    ThreadArg *ta = reinterpret_cast<ThreadArg *>(arg);
    int32_t   index   = ta->index;
    RpcThread *inst   = ta->inst;
    delete ta;
    prctl(PR_SET_NAME, inst->thread_name().c_str()); 
    inst->Run(inst->worker_args_[index], index);
    return NULL;
  }

 private:
  int32_t                   worker_count_; 
  std::vector<T>            worker_args_;
  std::vector<pthread_t>    pids_;
  volatile bool             stop_; 
};

}
}

#endif

