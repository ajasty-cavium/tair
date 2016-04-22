#include "rpc_server_context.h"
#include "tbsys.h"
#include "rpc_dispatcher.h"
#include "rpc_session.h"
#include "rpc_http_console.h"

namespace tair
{
namespace rpc
{

RpcServerContext::RpcServerContext(easy_io_t *eio, RpcDispatcher *dispatcher, void *handler)
    : eio_(eio), port_(-1), dispatcher_(dispatcher), http_console_(NULL), command_handler_(handler)
{
  bzero(&ehandler_, sizeof(ehandler_));
  // TODO: HandlePacket Client And Service should Shared
  ehandler_.user_data      = this;  
  ehandler_.process        = RpcServerContext::HandlePacket;
  ehandler_.get_packet_id  = RpcPacket::HandlePacketID;
  ehandler_.encode         = RpcPacket::HandlePacketEncode;
  ehandler_.decode         = RpcPacket::HandlePacketDecode;
  ehandler_.cleanup        = RpcPacket::HandlePacketCleanup;
  dispatcher->set_server_context(this);
}

void RpcServerContext::SetupHttpConsole(RpcHttpConsole *console)
{
  http_console_ = console;
  console->set_server_context(this);
}

bool RpcServerContext::Run()
{
  if (easy_io_add_listen(eio_, NULL, port_, &ehandler_) == NULL)
  {
    TBSYS_LOG(ERROR, "Listen %d failed", port_);
    return false;
  } 
  if (easy_io_start(eio_) != EASY_OK)
  {
    TBSYS_LOG(ERROR, "eio start failed");
    return false;
  }
  TBSYS_LOG(INFO, "Rpc Server Listening %d and Running...", port_);
  return true;
}

void RpcServerContext::Wait()
{
  easy_io_wait(eio_);
}

void RpcServerContext::Stop()
{
  easy_io_stop(eio_);
}

int RpcServerContext::DoHttpConsole(RpcHttpConsoleCommand::Command cmd, struct mg_connection *conn)
{
  return (*cmd)(this->command_handler_, conn); 
}

void RpcServerContext::DoCommand(RpcCommand *cmd, RpcSession *session)
{
  cmd->DoCommand(this->command_handler_, session);
}

int RpcServerContext::HandlePacket(easy_request_t *r)
{
  RpcServerContext *this_inst = reinterpret_cast<RpcServerContext *> (r->ms->c->handler->user_data);
  RpcPacket *request = reinterpret_cast<RpcPacket *>(r->ipacket);
  assert(request != NULL);
  RpcSession *session = new RpcSession();
  session->set_ref(1);
  session->set_request_packet(request);
  session->set_easy_request(r);
  int ret = this_inst->dispatcher_->ServerDispatch(session);
  session->UnRef();
  return ret;
}
 
}
}

