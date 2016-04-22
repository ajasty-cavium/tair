#include <easy_io.h>
#include "rpc_context.h"
#include "rpc_client_context.h"
#include "rpc_server_context.h"
#include "rpc_dispatcher.h"
#include "rpc_http_console.h"

namespace tair
{
namespace rpc
{

RpcContext::RpcContext() : eio_(NULL), http_console_(NULL), server_context_(NULL)
                         , server_dispatcher_(NULL), client_context_(NULL)
                         , client_dispatcher_(NULL)
{
  RpcCommandFactory::Initialize();
}

RpcCommandFactoryBuilder* RpcContext::GetRpcCommandFactoryBuilder()
{
  return new RpcCommandFactoryBuilder(RpcCommandFactory::Instance());
}

void RpcContext::Stop()
{
  easy_io_stop(eio_);
  if (client_dispatcher_ != NULL)
    client_dispatcher_->Stop();
  if (server_dispatcher_ != NULL)
    server_dispatcher_->Stop();
  if (http_console_ != NULL)
    http_console_->Stop();
}

void RpcContext::Wait()
{
  easy_io_wait(eio_);
  if (client_dispatcher_ != NULL)
    client_dispatcher_->Join();
  if (server_dispatcher_ != NULL)
    server_dispatcher_->Join();
  if (http_console_ != NULL)
    http_console_->Join();
}

void RpcContext::OnStartupFailed()
{
  if (client_context_ != NULL)
  {
    if (client_dispatcher_ != NULL)
    {
      client_dispatcher_->Stop();
      client_dispatcher_->Join();
    }
    delete client_context_;  
  }
  if (server_context_ != NULL)
  {
    server_dispatcher_->Stop();
    server_dispatcher_->Join();
    if (http_console_ != NULL)
    {
      http_console_->Stop();
      http_console_->Join();
      delete http_console_;
    }
    delete server_context_;
  }
  easy_io_destroy(eio_);
  delete eio_;
}

bool RpcContext::Startup(const Properties& prop)
{
  this->client_dispatcher_ = prop.client_dispatcher;
  this->server_dispatcher_ = prop.server_dispatcher;
  eio_ = new easy_io_t;
  memset(eio_, 0, sizeof(*eio_));
  if (easy_io_create(eio_, prop.io_count) == 0) 
  {
    delete eio_;
    eio_ = NULL;
    return false;
  }
  eio_->do_signal = 0;
  eio_->no_redispatch = 0;
  eio_->no_reuseport = 1;
  eio_->listen_all = 1;
  eio_->tcp_nodelay = 1;
  eio_->tcp_cork = 0;
  eio_->affinity_enable = 1; 
  
  bool success = true;

  client_context_ = new RpcClientContext(eio_, client_dispatcher_);
  if (client_dispatcher_ != NULL)
  {
    client_dispatcher_->Start(eio_);
  }

  if (prop.rpc_port == 0)
  {
    if (easy_io_start(eio_) != EASY_OK)
    {
      success = false;
    }
  } 
  else 
  {
    assert(server_dispatcher_ != NULL);
    assert(prop.rpc_port > 0);
    server_context_ = new RpcServerContext(eio_, server_dispatcher_, prop.user_handler);
    server_context_->set_port(prop.rpc_port);
    if (prop.http_port != 0)
    {
    //TODO: just start http console only on service
      http_console_   = new RpcHttpConsole(prop.http_port, prop.http_thread_count, RpcCommandFactory::Instance());
      server_context_->SetupHttpConsole(http_console_);
      http_console_->Start();
    }
    server_dispatcher_->Start(eio_);
    if (server_context_->Run() == false)
    {
      success = false;
    }
  }
  if (success == false)
    OnStartupFailed();
  return success;
}

void RpcContext::Shutdown()
{
  easy_io_destroy(eio_); // destroy eio first, beacuse call handle of context
  delete eio_;
  if (http_console_ != NULL)
    delete http_console_;
  if (server_context_ != NULL)
    delete server_context_;
  if (client_context_ != NULL)
    delete client_context_;
  RpcCommandFactory::Destroy();
}

}
}

