
#include "rpc_dispatcher_eio.h"
#include "reference_count.h"

#include <iostream>
using namespace std;

namespace tair
{
namespace rpc
{

void RpcDispatcherEIO::Start(easy_io_t *eio) 
{
  easy_thread_pool_t      *tp = eio->io_thread_pool;
  easy_io_thread_t        *ioth = NULL;

  easy_thread_pool_for_each(ioth, tp, 0) 
  {
    ++watchers_count_;
  }
  watchers_ = new WatcherInfo[watchers_count_];
  int pos = 0;
  easy_thread_pool_for_each(ioth, tp, 0) 
  {
    watchers_[pos].loop         = ioth->loop;
    watchers_[pos].dispatcher	= this;
    pthread_mutex_init(&(watchers_[pos].mutex), NULL);
    ev_async_init(&(watchers_[pos].watcher), RpcDispatcherEIO::DispatcherCallBack);
    ev_async_start(watchers_[pos].loop,&(watchers_[pos].watcher));
    ++pos;
  }
}

RpcDispatcherEIO::~RpcDispatcherEIO() 
{
  for (int i = 0; i < watchers_count_; ++i)
  {
    pthread_mutex_destroy(&watchers_[i].mutex);
    while(watchers_[i].task_queue.empty() == false)
    {
      ReferenceCount *rc = watchers_[i].task_queue.front();
      watchers_[i].task_queue.pop_front();
      rc->UnRef();
    }
  }
  if (watchers_ != NULL)
    delete[] watchers_;
}

void RpcDispatcherEIO::AsyncRun(WatcherInfo *w)
{
  int ret = pthread_mutex_trylock(&w->mutex); 
  if (ret == 0)
  {
    w->task_pending_queue.insert(w->task_pending_queue.end(), 
			  w->task_queue.begin(), 
			  w->task_queue.end());
    w->task_queue.clear();
    pthread_mutex_unlock(&w->mutex); 
  } 
  else if (ret == EBUSY)
  {
    ev_async_send(w->loop, &w->watcher);
  }

  ReferenceCount *rc = NULL;
  int i = 0;
  for (i = 0; i < 1024; ++i)  
  {
    rc = NULL;
    if (w->task_pending_queue.empty() == false)
    {
      rc = w->task_pending_queue.front();
      w->task_pending_queue.pop_front();
    }
    if (rc == NULL)
      return;
    Handle(rc);
    rc->UnRef();
  }
  if (i >= 1024) 
  {
    ev_async_send(w->loop, &w->watcher);
  }
}

void RpcDispatcherEIO::Dispatch(ReferenceCount *rc, int32_t req_index)
{
  int index = abs((req_index == 0 ? 
               __sync_fetch_and_add(&last_robin_thread_, 1) : req_index) % watchers_count_);

  WatcherInfo &w = watchers_[index];
  {
    pthread_mutex_lock(&w.mutex);
    w.task_queue.push_back(rc);
    pthread_mutex_unlock(&w.mutex);
    ev_async_send(w.loop, &w.watcher);
  }
}

void RpcDispatcherEIO::DispatcherCallBack(EV_P_ ev_async *w, int revents)
{
  WatcherInfo *watcher = (WatcherInfo *)(w);
  watcher->dispatcher->AsyncRun(watcher);
}

} // end namespace rpc
} // end namespace tair

