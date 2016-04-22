#include "rpc_future.h"
#include "rpc_packet.h"
#include "rpc_dispatcher.h"

namespace tair
{
namespace rpc
{

void RpcFuture::Dispatch()
{
  if (dispatcher_ == NULL)
    return;
  dispatcher_->ClientDispatch(this);
}

void RpcFuture::Notify() 
{
  pthread_mutex_lock(&mutex_);
  done_ = true;
  pthread_cond_broadcast(&cond_);
  pthread_mutex_unlock(&mutex_);
}

void RpcFuture::Wait()
{
  if (done_) return;
  pthread_mutex_lock(&mutex_);
  if (done_ == false)
    pthread_cond_wait(&cond_, &mutex_);
  pthread_mutex_unlock(&mutex_);
  assert(done_);
}

bool RpcFuture::TimedWait(long ms)
{
  if (done_) return true;
  timespec to; 
  to.tv_sec = time(NULL) + ms / 1000;
  to.tv_nsec = (ms % 1000) * 1000000; 

  pthread_mutex_lock(&mutex_);
  if (done_ == false)
    pthread_cond_timedwait(&cond_, &mutex_, &to);
  pthread_mutex_unlock(&mutex_);
  return done_; 
}

}
}

