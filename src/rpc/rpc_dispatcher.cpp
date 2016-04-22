#include "rpc_dispatcher.h"

#include <pthread.h>
#include <sys/prctl.h>
#include "rpc_packet.h"
#include "rpc_future.h"
#include "rpc_session.h"
#include "rpc_command.hpp"
#include "echo_command.hpp"
#include "rpc_command_factory.h"
#include "rpc_server_context.h"

namespace tair
{
namespace rpc
{


void RpcDispatcherAbstract::ClientDispatch(RpcFuture *future)
{
  Dispatch(future, future->request_header().index);
}

int RpcDispatcherAbstract::ServerDispatch(RpcSession *session)
{
  easy_request_sleeping(session->easy_request());
  Dispatch(session, session->request_header().index);
  return EASY_AGAIN;
}

void RpcDispatcherAbstract::Handle(ReferenceCount *rc)
{
  RpcSession *session = dynamic_cast<RpcSession *>(rc);
  if (session != NULL)
  {
    this->ServerHandle(session);
  }
  else
  {
    RpcFuture *future = dynamic_cast<RpcFuture *>(rc);
    assert(future != NULL);
    this->ClientHandle(future);
  }
}

void RpcDispatcherAbstract::ServerHandle(RpcSession *session)
{
  int pid = session->request_header().packet_id;
  RpcCommand *cmd = command_factory_->FindCommand(pid);
  assert(cmd != NULL);
  session->Build(cmd->response_id(), cmd->NewResponseMessage());
  context_->DoCommand(cmd, session);
}


RpcDispatcherDefault::RpcDispatcherDefault(int workcount) 
    : RpcDispatcherAbstract(), workcount_(workcount), last_robin_thread_(0), stop_(false)
{
  worker_infos_ = new WorkerInfo[workcount];  
  for (int i = 0; i < workcount; ++i)
  {
    WorkerInfo *wi = mutable_worker_info(i);
    wi->pid = 0;
    wi->dispatcher = this;
    pthread_mutex_init(&wi->mutex, NULL);
    pthread_cond_init(&wi->cond, NULL);
  }
}

void RpcDispatcherDefault::Join()
{
  for (int i = 0; i < workcount_; ++i)
  {
    WorkerInfo *wi = mutable_worker_info(i);
    pthread_join(wi->pid, NULL);
  }
}

void RpcDispatcherDefault::Start()
{
  for(int i = 0, np = sysconf(_SC_NPROCESSORS_ONLN) - 1; 
      i < workcount_; ++i, --np)
  {
    if (np < 0) np = sysconf(_SC_NPROCESSORS_ONLN) - 1;
    WorkerInfo *wi = mutable_worker_info(i);
    pthread_create(&wi->pid, NULL, ThreadFunc, wi);
    //bind cpu
    cpu_set_t mask;  
    CPU_ZERO(&mask);  
    CPU_SET(np,&mask);  
    pthread_setaffinity_np(wi->pid, sizeof(mask), &mask);
  }
}

void RpcDispatcherDefault::Stop()
{
  stop_ = true;
  for (int i = 0; i < workcount_; ++i)
  {
    WorkerInfo *wi = mutable_worker_info(i);
    pthread_mutex_lock(&wi->mutex);
    pthread_cond_signal(&wi->cond);
    pthread_mutex_unlock(&wi->mutex);
  }
}

RpcDispatcherDefault::~RpcDispatcherDefault()
{
  for (int i = 0; i < workcount_; ++i)
  {
    WorkerInfo *wi = mutable_worker_info(i);
    pthread_mutex_destroy(&wi->mutex);
    pthread_cond_destroy(&wi->cond);
    while (wi->task_queue.empty() == false)
    {
      ReferenceCount *rc = wi->task_queue.front();
      wi->task_queue.pop();
      rc->UnRef();
    }
  }
  delete[] worker_infos_;
}

void RpcDispatcherDefault::Dispatch(ReferenceCount *rc, int32_t req_index)
{
  rc->Ref();
  // TODO: strategy design
  int index = (req_index == 0 ? 
               __sync_fetch_and_add(&last_robin_thread_, 1) : req_index) % workcount_;
  WorkerInfo *wi = mutable_worker_info(index);
  {
    pthread_mutex_lock(&wi->mutex);
    wi->task_queue.push(rc);
    pthread_cond_signal(&wi->cond); 
    pthread_mutex_unlock(&wi->mutex);
  }
}

void* RpcDispatcherDefault::ThreadFunc(void *arg)
{
  prctl(PR_SET_NAME, "rpc-dispatcher");
  WorkerInfo *wi = reinterpret_cast<WorkerInfo *>(arg);
  RpcDispatcherDefault *this_inst = wi->dispatcher;
  this_inst->Run(wi);
  return NULL;
}

void RpcDispatcherDefault::Run(WorkerInfo *wi)
{
  while (this->stop_ == false)
  {
    ReferenceCount *rc = NULL;
    pthread_mutex_lock(&(wi->mutex));
    if (wi->task_queue.empty())
      pthread_cond_wait(&(wi->cond), &(wi->mutex));
    if (this->stop_ == false && wi->task_queue.empty() == false)
    {
      rc = wi->task_queue.front();
      wi->task_queue.pop();
    }
    pthread_mutex_unlock(&(wi->mutex));
    if (rc == NULL)
    {
      continue;
    }
    Handle(rc);
    rc->UnRef();
  }
}

}
}

