/*=============================================================================
#     FileName: time_event_scheduler.cpp
#         Desc:
#       Author: yexiang.ych
#        Email: yexiang.ych@taobao.com
#     HomePage:
#      Version: 0.0.1
#   LastChange: 2014-03-17 10:53:28
#      History:
=============================================================================*/
#include "time_event_scheduler.h"

namespace tair
{
namespace admin
{

TimeEventScheduler::TimeEventScheduler()
    : handler_(NULL)
{
  std::vector<void *> arg(1, NULL);
  InitializeWorker(1, arg);
  pthread_mutex_init(&mutex_, NULL);
  pthread_cond_init(&cond_, NULL);
}

TimeEventScheduler::~TimeEventScheduler()
{
  while(event_queue_.empty() == false)
  {
    event_queue_.pop();
  }
  pthread_mutex_destroy(&mutex_);
  pthread_cond_destroy(&cond_);
}

void TimeEventScheduler::Stop()
{
  pthread_cond_signal(&cond_);
  tair::rpc::RpcThread<void *>::Stop();
}

void TimeEventScheduler::Run(void *&arg, int32_t index)
{
  while(IsStop() == false)
  {
    bool doevent = false;
    TimeEvent event;
    pthread_mutex_lock(&mutex_);
    while (event_queue_.empty() && IsStop() == false)
    {
      pthread_cond_wait(&cond_, &mutex_);
    }
    if (IsStop() == false)
    {
      event = event_queue_.top();
      doevent = true;
      time_t now = time(NULL);
      if (now < event.scheduler_time)
      {
        timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += (event.scheduler_time - now);
        pthread_cond_timedwait(&cond_, &mutex_, &ts);
      }
      event_queue_.pop();
    }
    pthread_mutex_unlock(&mutex_);
    if (doevent)
    {
      DoEvent(event);
    }
  }
}

void TimeEventScheduler::DoEvent(TimeEvent &event)
{
  TBSYS_LOG(DEBUG, "Check And Maybe Sync TairInfo:%s",
                event.tair_info->ToString().c_str());
  std::vector<easy_addr_t> addrs;
  bool changed = false;
  Status status = handler_->UpdateDataServers(event.tair_info, addrs);
  if (status.ok())
  {
    changed = true;
  }
  else if (status.IsNotFound())
  {
    changed = false;
    status = Status::OK();
  }
  else
  {
    TBSYS_LOG(ERROR, "Update DataServers Failed TairInfo:%s Status:%s",
              event.tair_info->ToString().c_str(), status.str());
  }
  if (status.ok() && addrs.size() > 0)
  {
    TairConfigurationIterator iter       = event.tair_info->begin();
    TairConfigurationIterator iter_end   = event.tair_info->end();
    for (; iter != iter_end; ++iter)
    {
      time_t now = time(NULL);
      iter->second.Lock();
      if ((changed || iter->second.last_check_time() + 2 * 60 < now)
          && addrs.size() > 0)
      {
        std::string uuid;
        if (handler_->GenerateUUID(uuid).ok())
        {
          handler_->AddOplog(uuid, event.tair_info->tid(), iter->first,
                             "", "", iter->second.version(), iter->second.version(), kCheck);
        }
        handler_->task_scheduler().PushTask(uuid, iter->first, iter->second.entries(),
                                            0, iter->second.version(), addrs, kCheck);
        iter->second.set_last_check_time(now);
      }
      iter->second.Unlock();
    }
  }
  event.scheduler_time += 5; // per 5 seconds check
  AddEvent(event);
}

void TimeEventScheduler::PushEvent(TairInfo *tair_info, time_t t)
{
  TBSYS_LOG(INFO, "%s SchedulerTime:%ld TairInfo:%s", __FUNCTION__,
            t, tair_info->ToString().c_str());
  TimeEvent event;
  event.tair_info = tair_info;
  event.scheduler_time = t;
  pthread_mutex_lock(&mutex_);
  event_queue_.push(event);
  pthread_cond_broadcast(&cond_);
  pthread_mutex_unlock(&mutex_);
}

void TimeEventScheduler::AddEvent(const TimeEvent &te)
{
  pthread_mutex_lock(&mutex_);
  event_queue_.push(te);
  pthread_cond_broadcast(&cond_);
  pthread_mutex_unlock(&mutex_);
}

}
}
