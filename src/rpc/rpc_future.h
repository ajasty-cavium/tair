#ifndef TAIR_RPC_FUTURE_H_
#define TAIR_RPC_FUTURE_H_

#include "rpc_packet.h"
#include "reference_count.h"

namespace tair
{
namespace rpc
{

class RpcPacket;
class RpcDispatcher;

class RpcFuture : public ReferenceCount
{
 public:

  typedef void (* CallBack)(RpcFuture *future);

  RpcFuture() : request_(NULL), response_(NULL), dispatcher_(NULL), cb_func_(NULL)
              , arg_(NULL), errno_(0), done_(false)
  {
    pthread_mutex_init(&mutex_, NULL);
    pthread_cond_init(&cond_, NULL);
  }

  ~RpcFuture()
  {
    if (request_ != NULL)
    {
      request_->UnRef();
      request_ = NULL;
    }
    if (response_ != NULL)
    {
      response_->UnRef();
      response_ = NULL;
    }
    arg_ = NULL;
    cb_func_ = NULL;
    pthread_cond_destroy(&cond_);
    pthread_mutex_destroy(&mutex_);
  }
  
  const RpcPacket::Header&  request_header() const
  {
    return request_->header();
  }

  const ::google::protobuf::Message* request() const 
  { 
    assert(request_ != NULL);
    return request_->body();
  } 

  const TairMessage* tair_response() const 
  { 
    return response_ == NULL ? NULL : response_->tair_body();
  }

  const ::google::protobuf::Message* response() const 
  { 
    return response_ == NULL ? NULL : response_->body();
  } 
 
  void set_request_packet(RpcPacket *request) { request->Ref(); request_ = request; }

  void set_response_packet(RpcPacket *response) { response->Ref(); response_ = response; }

  void set_callback(CallBack func) { cb_func_ = func; }

  void set_arg(void *arg) 
  { 
      arg_ = arg; 
  }

  void set_dispatcher(RpcDispatcher *dispatcher) { dispatcher_ = dispatcher; }

  void* arg() { return arg_; }

  void Done() 
  {
    Notify();
    Dispatch();
    HandleCallBack();
  }

  //TODO: enum err message
  void set_exception(int e) { errno_ = e; }

  int  exception() const { return errno_; }

  void Wait();
  bool TimedWait(long ms);

 protected:
  void Notify();

  void HandleCallBack() { if (cb_func_ != NULL) (*cb_func_)(this); }

  void Dispatch();
 
 private:
  RpcPacket       *request_;
  RpcPacket       *response_;
  RpcDispatcher   *dispatcher_;
  CallBack        cb_func_;
  void            *arg_;
  int             errno_;

  volatile bool   done_;
  pthread_mutex_t mutex_;
  pthread_cond_t  cond_;
};

}
}



#endif

