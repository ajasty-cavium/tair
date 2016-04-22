#include "rpc_client_context.h"

#include <tbsys.h>
#include "rpc_future.h"
#include "rpc_packet.h"
#include "rpc_dispatcher.h"

namespace tair
{
namespace rpc
{

int32_t RpcClientContext::s_channel_seq = 0;

RpcClientContext::RpcClientContext(easy_io_t *eio, RpcDispatcher *dispatcher) 
    : eio_(eio), dispatcher_(dispatcher)
{
  bzero(&ehandler_, sizeof(ehandler_));
  // TODO: HandlePacket Client And Service should Shared
  ehandler_.process        = RpcClientContext::HandlePacket;
  ehandler_.get_packet_id  = RpcPacket::HandlePacketID;
  ehandler_.encode         = RpcPacket::HandlePacketEncode;
  ehandler_.decode         = RpcPacket::HandlePacketDecode;
  ehandler_.cleanup        = RpcPacket::HandlePacketCleanup;
}

void RpcClientContext::Send(const easy_addr_t &addr, RpcPacket *request, int timeout_ms, RpcFuture *future)
{
  assert(request->mutable_header().channel_seq != 0);
  assert(request != NULL);
  assert(future != NULL);
  request->PreEncode();
  easy_session_t *es = easy_session_create(0);
  if (es == NULL)
  {
    //TODO: LOG
    future->set_exception(-1);
  }
  else
  {
    es->status     = EASY_CONNECT_SEND;
    es->packet_id  = 0;
    es->thread_ptr = &ehandler_;
    es->process    = RpcClientContext::HandlePacket;
    easy_session_set_request(es, request, timeout_ms, future);
    future->Ref();
    request->Ref();
    if (easy_client_dispatch(eio_, addr, es) != EASY_OK) 
    {
      future->set_exception(-1);
      future->UnRef();
      request->UnRef();
      easy_session_destroy(es);
      es = NULL;
    }
  }
  if (es == NULL)
  {
    future->Done();
  }
  return;
}

RpcFuture* RpcClientContext::CallAndDispatch(const easy_addr_t &addr, RpcPacket *request, int timeout_ms, RpcDispatcher *dispatcher, void *arg)
{
  RpcFuture *future = new RpcFuture();
  future->set_ref(1);
  future->set_arg(arg);
  future->set_request_packet(request);
  future->set_dispatcher(dispatcher);
  Send(addr, request, timeout_ms, future);
  return future;
}

RpcFuture* RpcClientContext::CallAndDispatch(const easy_addr_t &addr, RpcPacket *request, int timeout_ms, void *arg)
{
  assert(dispatcher_ != NULL);
  return CallAndDispatch(addr, request, timeout_ms, dispatcher_, arg);
}

RpcPacket* RpcClientContext::NewTairRpcPacket(int packet_id, TairMessage *request)
{
  RpcPacket *packet = new RpcPacket();
  packet->set_ref(0);
  packet->set_body(request);
  packet->mutable_header().magic_code   = RpcPacket::Header::TAIR_MAGIC;
  packet->mutable_header().channel_seq  = NextChannelSeq();
  packet->mutable_header().index        = 0;
  packet->mutable_header().style        = 0;
  packet->mutable_header().packet_id    = packet_id;
  packet->mutable_header().version      = 0;
  packet->mutable_header().caller       = 0;
  packet->mutable_header().reserved1    = 0;
  packet->mutable_header().header_len   = RpcPacket::Header::TAIR_LENGTH;
  return packet;
}

RpcPacket* RpcClientContext::NewRpcPacket(int packet_id, ::google::protobuf::Message *request)
{
  RpcPacket *packet = new RpcPacket();
  packet->set_ref(0);
  packet->set_body(request);
  packet->mutable_header().magic_code   = RpcPacket::Header::MAGIC;
  packet->mutable_header().channel_seq  = NextChannelSeq();
  packet->mutable_header().index        = 0;
  packet->mutable_header().style        = 0;
  packet->mutable_header().packet_id    = packet_id;
  packet->mutable_header().version      = 0;
  packet->mutable_header().caller       = 0;
  packet->mutable_header().reserved1    = 0;
  packet->mutable_header().header_len   = RpcPacket::Header::LENGTH;
  return packet;
}

RpcFuture* RpcClientContext::Call(const easy_addr_t &addr, 
                                  RpcPacket *packet, 
                                  int timeout_ms, 
                                  RpcFuture::CallBack cb, void *arg)
{
  RpcFuture *future = new RpcFuture();
  future->set_ref(1);
  future->set_arg(arg);
  future->set_request_packet(packet);
  future->set_callback(cb);
  Send(addr, packet, timeout_ms, future);
  return future;
}

int RpcClientContext::HandlePacket(easy_request_t *r)
{
  RpcFuture *future     = reinterpret_cast<RpcFuture *>(r->args);
  assert(future != NULL);
  RpcPacket *response   = reinterpret_cast<RpcPacket *>(r->ipacket);
  if (response == NULL)
  {
    // timedout
    // TODO: log
    future->set_exception(-2);
  }
  else
  {
    future->set_response_packet(response); 
  }
  future->Done();
  future->UnRef();
  easy_session_destroy(r->ms);
  return EASY_OK;
}

}
}

