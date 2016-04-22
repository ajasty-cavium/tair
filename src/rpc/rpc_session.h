#ifndef TAIR_RPC_SESSION_H_
#define TAIR_RPC_SESSION_H_

#include "rpc_packet.h"
#include "reference_count.h"

namespace tair
{
namespace rpc
{

class RpcPacket;

class RpcSession: public ReferenceCount
{
 public:

  RpcSession()  : request_(NULL), response_(NULL), r_(NULL)
  {
  }

  ~RpcSession()
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
  }
  
  void set_request_packet(RpcPacket *request) { request->Ref(); request_ = request; }

  inline const RpcPacket::Header&  request_header() const
  {
    return request_->header();
  }

  inline RpcPacket::Header&  mutable_response_header()
  {
    return response_->mutable_header();
  }

  inline const ::google::protobuf::Message* request() const 
  { 
    assert(request_ != NULL);
    return request_->body();
  } 

  inline const ::google::protobuf::Message* response() const 
  { 
    return response_ == NULL ? NULL : response_->body();
  } 

  inline ::google::protobuf::Message* mutable_response() const 
  { 
    return response_ == NULL ? NULL : response_->mutable_body();
  }

  void set_easy_request(easy_request_t *r) { r_ = r; }

  easy_request_t* easy_request() { return r_; }

  void Build(int32_t pid, ::google::protobuf::Message* resp)
  {
    response_ = new RpcPacket;
    response_->set_ref(1);
    response_->mutable_header() = request_->header();
    response_->mutable_header().packet_id = pid;
    response_->set_body(resp);
  }

  void Done() 
  { 
    response_->set_body(response_->mutable_body());
    response_->PreEncode();
    r_->opacket = response_;
    response_ = NULL;
    easy_request_wakeup(r_);
  }
 protected:

 private:
  RpcPacket       *request_;
  RpcPacket       *response_;
  easy_request_t  *r_;
};

}
}



#endif

