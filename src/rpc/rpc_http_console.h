#ifndef TAIR_PRC_HTTP_CONSOLE_H_
#define TAIR_PRC_HTTP_CONSOLE_H_

#include <vector>
#include <pthread.h>
#include <cassert>
#include "rpc_server_context.h"
#include "mongoose.h"

namespace tair
{
namespace rpc
{

class RpcHttpConsole
{
 public:
  RpcHttpConsole(int port, int count, RpcCommandFactory *cmd_factory);
  ~RpcHttpConsole();

  void Stop();

  void Start();

  void Join();

  void set_server_context(RpcServerContext *context) { this->context_ = context; }
 protected:
  RpcServerContext *context_;

 private:
  struct WorkerInfo
  {
    RpcHttpConsole   *console;
    struct mg_server *http_server;
    pthread_t        pid;
  };

  inline WorkerInfo* mutable_worker_info(int i)  { assert(i >= 0); assert(i < worker_count_); return worker_infos_ + i; }

  static void* ThreadFunc(void *arg);

  void Run(WorkerInfo *wi);

private:
  WorkerInfo              *worker_infos_;
  int                     port_;
  int                     worker_count_;
  bool                    stop_;

  RpcCommandFactory 	  *cmd_factory_;
};

}
}

#endif
