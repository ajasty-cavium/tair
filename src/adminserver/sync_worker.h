#ifndef ADMIN_SYNC_WORKER_H_
#define ADMIN_SYNC_WORKER_H_

#include <pthread.h>
#include <vector>
#include <stdint.h>
#include "tair_rpc.h"
#include "administrator_handler.h"

namespace tair
{
namespace admin
{

class TaskScheduler;

class SyncWorker : public tair::rpc::RpcThread<void *>
{
 public:
  SyncWorker();

  virtual void Stop();

  void set_handler(AdministratorHandler *handler)
  {
    this->handler_ = handler;
  }

  void set_worker_count(int32_t c)
  {
    std::vector<void *>     args;
    args.assign(c, NULL);
    InitializeWorker(c, args); 
  }

 protected:
  virtual std::string thread_name() const 
  {
    return "admin-sync-worker";
  }

  virtual void Run(void *&arg, int32_t index);

 private:
  AdministratorHandler *handler_;
};

}
}

#endif

