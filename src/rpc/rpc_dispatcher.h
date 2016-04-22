#ifndef TAIR_RPC_DISPATCHER_H_
#define TAIR_RPC_DISPATCHER_H_

#include <pthread.h>
#include <queue>
#include <cassert>
#include <easy_io.h>

#include "rpc_command_factory.h"

namespace tair
{
namespace rpc
{

class RpcFuture;
class RpcSession;
class ReferenceCount;
class RpcServerContext;

class RpcDispatcher 
{
 public:
  RpcDispatcher() : context_(NULL) {}

  virtual ~RpcDispatcher() {}
  virtual void ClientDispatch(RpcFuture *) {}
  virtual int  ServerDispatch(RpcSession *) { return EASY_AGAIN; }

  virtual void Start(easy_io_t *eio) {}
  virtual void Stop() {}
  virtual void Join() {}
 protected:
  RpcServerContext *context_;

 private:
  void set_server_context(RpcServerContext *context) { this->context_ = context; }
  friend class RpcServerContext;
};

class RpcDispatcherAbstract : public RpcDispatcher 
{
 public:
  RpcDispatcherAbstract() { command_factory_ = RpcCommandFactory::Instance(); }
  virtual ~RpcDispatcherAbstract() {}
  
  virtual void ClientDispatch(RpcFuture *future);

  virtual int ServerDispatch(RpcSession *session);
  
 protected:
  virtual void Dispatch(ReferenceCount *rc, int32_t index) = 0;

  void Handle(ReferenceCount *rc);

  void ServerHandle(RpcSession *session);

  void ClientHandle(RpcFuture *future) {}

 private:
  RpcCommandFactory *command_factory_;
};

class RpcDispatcherDefault : public RpcDispatcherAbstract
{
 public:
  RpcDispatcherDefault(int workcount);
  virtual ~RpcDispatcherDefault();

  virtual void Start(); 
  virtual void Stop();
  virtual void Join();

 protected:
  virtual void Dispatch(ReferenceCount *rc, int32_t index);

 private:
  struct WorkerInfo
  {
    RpcDispatcherDefault          *dispatcher;
    pthread_t                     pid;
    pthread_mutex_t               mutex;
    pthread_cond_t                cond;
    std::queue<ReferenceCount *>  task_queue;
  };

  static void* ThreadFunc(void *);

  inline WorkerInfo* mutable_worker_info(int i)  { assert(i >= 0); assert(i < workcount_); return worker_infos_ + i; }

  void Run(WorkerInfo *wi);

 private:
  WorkerInfo  *worker_infos_;
  int         workcount_;
  int         last_robin_thread_;
  bool        stop_;
};

}
}

#endif

