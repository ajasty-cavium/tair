/*=============================================================================
#     FileName: time_event_scheduler.h
#         Desc: event type:[tair check, config check, initlize]
#       Author: yexiang.ych
#        Email: yexiang.ych@taobao.com
#     HomePage: 
#      Version: 0.0.1
#   LastChange: 2014-03-15 21:29:28
#      History:
=============================================================================*/
#ifndef ADMIN_TIME_SCHEDULER_H_
#define ADMIN_TIME_SCHEDULER_H_

#include <queue>
#include <pthread.h>
#include "tair_rpc.h"
#include "administrator_handler.h"

namespace tair
{
namespace admin
{

class AdministratorClient;

struct TimeEvent
{
  TairInfo        *tair_info;
  time_t          scheduler_time;
  bool operator<(const TimeEvent &rhs) const
  {
    return scheduler_time > rhs.scheduler_time;
  }
};

class TimeEventScheduler : public tair::rpc::RpcThread<void *>
{
 public:
  TimeEventScheduler();
  virtual ~TimeEventScheduler();

  void PushEvent(TairInfo *tair_info, time_t t);

  void set_handler(AdministratorHandler *handler)
  {
    this->handler_ = handler;
  }

  virtual void Stop();

 protected:
  virtual void Run(void *&arg, int32_t index);

  virtual std::string thread_name() const { return "admin-time-event"; }

  void AddEvent(const TimeEvent &e);

  void DoEvent(TimeEvent &event);

 private:
  std::priority_queue<TimeEvent> event_queue_;
  pthread_mutex_t             mutex_;
  pthread_cond_t              cond_;
  AdministratorHandler        *handler_;
};

}
}

#endif

