#ifndef TAIR_RPC_CONTEXT_H_
#define TAIR_RPC_CONTEXT_H_

#include <easy_io.h>
#include "rpc_client_context.h"
#include "rpc_server_context.h"
#include "rpc_dispatcher.h"
#include "rpc_http_console.h"

namespace tair
{
namespace rpc
{

class RpcCommandFactoryBuilder
{
 public:
  /**
   * Not support now
   */
  static RpcCommandFactoryBuilder *NewRpcCommandFactory()
  {
    return new RpcCommandFactoryBuilder(new RpcCommandFactory());
  }

  RpcCommandFactory *Build()
  {
    RpcCommandFactory *temp = instance_;
    delete this;
    return temp;
  }

  RpcCommandFactoryBuilder* RegisterTairCommand(TairRpcCommand *cmd)
  {
    instance_->RegisterTairCommand(cmd);
    return this; 
  }

  RpcCommandFactoryBuilder* RegisterHttpCommand(const RpcHttpConsoleCommand &cmd)
  {
    instance_->RegisterHttpCommand(cmd);
    return this; 
  }

  RpcCommandFactoryBuilder* RegisterCommand(RpcCommand *cmd)
  {
    instance_->RegisterCommand(cmd);
    return this; 
  }

 private:
  RpcCommandFactory *instance_;
  RpcCommandFactoryBuilder(RpcCommandFactory *instance) : instance_(instance){}
  friend class RpcContext;
};

class RpcContext
{
 public:
  static void Cast2String(const easy_addr_t &addr, std::stringstream &ss)
  {
    char buffer[64];
    easy_inet_addr_to_str(const_cast<easy_addr_t *>(&addr), buffer, 64);
    ss << buffer;
  }

  static easy_addr_t Cast2Easy(uint64_t server) 
  {
    easy_addr_t addr;
    memset(&addr, 0, sizeof(easy_addr_t));
    addr.family = AF_INET;
    addr.port   = htons( (server >> 32) & 0xffff);
    addr.u.addr = (server & 0xffffffffUL);
    addr.cidx   = 256;
    return addr;
  }

  struct Properties
  {
    int rpc_port;
    int io_count;
    int http_port;
    int http_thread_count;
    void *user_handler;
    RpcDispatcher *server_dispatcher;
    RpcDispatcher *client_dispatcher;

    Properties() : rpc_port(0), io_count(4), http_port(0), http_thread_count(0)
                 , user_handler(NULL), server_dispatcher(NULL), client_dispatcher(NULL)
    {
    }
  };

  RpcContext();

  RpcCommandFactoryBuilder* GetRpcCommandFactoryBuilder();

  bool Startup(const Properties& prop);

  void Wait();

  void Stop();

  void Shutdown();

  RpcClientContext *client_context() { return client_context_; }
  
 private:
  void OnStartupFailed();

 private:
  easy_io_t *eio_;
  RpcHttpConsole   *http_console_;
  RpcServerContext *server_context_;
  RpcDispatcher    *server_dispatcher_;
  RpcClientContext *client_context_;
  RpcDispatcher    *client_dispatcher_;
};

}
}

#endif

