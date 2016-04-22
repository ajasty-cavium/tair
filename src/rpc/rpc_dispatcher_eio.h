#ifndef TAIR_RPC_DISPATCHER_EIO_H_
#define TAIR_RPC_DISPATCHER_EIO_H_

#include <pthread.h>
#include <deque>
#include <queue>
#include <cassert>
#include <easy_io.h>

#include "rpc_dispatcher.h"

namespace tair
{
namespace rpc
{

class RpcDispatcherEIO : public RpcDispatcherAbstract
{
 public:
  RpcDispatcherEIO() : watchers_count_(0), last_robin_thread_(0) {}

  virtual void Start(easy_io_t *eio); 

  virtual ~RpcDispatcherEIO();

 protected:
  struct WatcherInfo 
  {
    ev_async                      watcher;
    struct ev_loop                *loop;
    RpcDispatcherEIO              *dispatcher;
    pthread_mutex_t               mutex;
    std::deque<ReferenceCount *>  task_queue;
    std::deque<ReferenceCount *>  task_pending_queue;
  };

  virtual void Dispatch(ReferenceCount *rc, int req_index);

  static void DispatcherCallBack(EV_P_ ev_async *w, int revents);

 private:
  void AsyncRun(WatcherInfo *w);

 private:
  WatcherInfo *watchers_;
  int	      watchers_count_;
  int         last_robin_thread_;
};

}
}

#endif

