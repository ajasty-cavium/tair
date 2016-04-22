#ifndef TAIR_RPC_COMMAND_HPP_
#define TAIR_RPC_COMMAND_HPP_

#include <google/protobuf/message.h>
#include "rpc_session.h"
#include "tair_message.h"
#include "mongoose.h"


namespace tair
{
namespace rpc
{

class RpcServerContext;

class RpcHttpConsoleCommand
{
 public:
  typedef int (* Command)(void *handler, struct mg_connection *conn);

  virtual const Command GetCommand() const = 0;

  virtual const std::string uri() const = 0;

  virtual ~RpcHttpConsoleCommand() {}
};

class TairRpcCommand
{
 public:
  virtual ~TairRpcCommand() {}

  virtual void DoCommand(void *handler, RpcSession *session) = 0;

  virtual int16_t request_id() = 0;

  virtual int16_t response_id() = 0;

  virtual TairMessage* NewRequestMessage() = 0; 

  virtual TairMessage* NewResponseMessage() = 0; 
};

template<int16_t RequestID, class RequestT, int16_t ResponseID, class ResponseT> 
class TairRpcCommandAbstract : public TairRpcCommand
{
 public:

  TairRpcCommandAbstract() : request_id_(RequestID), response_id_(ResponseID)
  {
  }

  virtual void DoCommand(void *handler, RpcSession *session) = 0;

  static int16_t GetRequestID() { return RequestID; }

  virtual int16_t request_id() { return request_id_; }

  virtual int16_t response_id() { return response_id_; }

  virtual TairMessage* NewRequestMessage() { return new RequestT(); }

  virtual TairMessage* NewResponseMessage() { return new ResponseT(); }
 
 private:
  int16_t request_id_;
  int16_t response_id_;
};

class RpcCommand
{
 public:
  virtual ~RpcCommand() {}

  virtual void DoCommand(void *handler, RpcSession *session) = 0;

  virtual int16_t  request_id() = 0;

  virtual int16_t  response_id() = 0;

  virtual ::google::protobuf::Message* NewRequestMessage() = 0; 

  virtual ::google::protobuf::Message* NewResponseMessage() = 0; 
};

template<int16_t RequestID, class RequestT, int16_t ResponseID, class ResponseT> 
class RpcCommandAbstract : public RpcCommand
{
 public:

  RpcCommandAbstract() : request_id_(RequestID), response_id_(ResponseID)
  {
  }

  virtual void DoCommand(void *handler, RpcSession *session) = 0;

  virtual int16_t request_id() { return request_id_; }

  virtual int16_t response_id() { return response_id_; }

  virtual ::google::protobuf::Message* NewRequestMessage() { return new RequestT(); }

  virtual ::google::protobuf::Message* NewResponseMessage() { return new ResponseT(); }
 
 private:
  int16_t request_id_;
  int16_t response_id_;
};

}
}

#endif

