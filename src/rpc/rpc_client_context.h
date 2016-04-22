#ifndef TAIR_RPC_CLIENT_CONTEXT_H_
#define TAIR_RPC_CLIENT_CONTEXT_H_

#include <easy_io.h>
#include "rpc_future.h"

namespace tair
{
namespace rpc
{

class RpcPacket;
class RpcFuture;
class RpcDispatcher;

class RpcClientContext
{
 public:
  RpcClientContext(easy_io_t *eio, RpcDispatcher *dispatcher);
  // eio must stoped
  ~RpcClientContext() {};

  RpcFuture* Call(const easy_addr_t &addr, 
                  RpcPacket *packet,
                  int timeout_ms, 
                  RpcFuture::CallBack cb, void *arg);

  RpcPacket* NewRpcPacket(int request_id, ::google::protobuf::Message *request);
  RpcPacket* NewTairRpcPacket(int packet_id, TairMessage *request);

 private:
  static int HandlePacket(easy_request_t *r);

  RpcFuture* CallAndDispatch(const easy_addr_t &addr, RpcPacket *request, int timeout_ms, void *arg);

  RpcFuture* CallAndDispatch(const easy_addr_t &addr, RpcPacket *request, int timeout_ms, 
                               RpcDispatcher *dispatcher, void *arg);


  void Send(const easy_addr_t &addr, RpcPacket *request, int timeout_ms, RpcFuture *future);

  int32_t NextChannelSeq() { return __sync_add_and_fetch(&s_channel_seq, 1); }

 private:
  easy_io_t           *eio_;  
  easy_io_handler_pt  ehandler_;
  RpcDispatcher       *dispatcher_;

  static int32_t s_channel_seq; 
};

}
}

#endif

