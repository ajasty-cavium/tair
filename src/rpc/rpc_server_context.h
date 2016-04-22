#ifndef TAIR_RPC_SERVER_CONTEXT_H_
#define TAIR_RPC_SERVER_CONTEXT_H_

#include <easy_io.h>
#include "mongoose.h"
#include "rpc_command_factory.h"

namespace tair
{
namespace rpc
{

class RpcDispatcher;
class RpcDispatcherDefault;
class RpcHttpConsole;

class RpcServerContext
{
 public:
  RpcServerContext(easy_io_t *eio, RpcDispatcher *dispatcher, void *command_handler);

  static void Initialize() { RpcCommandFactory::Initialize(); }

  static void Destroy() 
  { 
    RpcCommandFactory::Destroy();
    google::protobuf::ShutdownProtobufLibrary();
  }

  void SetupHttpConsole(RpcHttpConsole *console);

  bool Run();

  void Wait();

  void Stop(); 

  void set_port(int p) { port_ = p; }

  void DoCommand(RpcCommand *cmd, RpcSession *session);

 private:
  static int HandlePacket(easy_request_t *r);

  int  DoHttpConsole(RpcHttpConsoleCommand::Command cmd, struct mg_connection *conn);
  
  friend class RpcHttpConsole;
 private:
  easy_io_t           *eio_;  
  easy_io_handler_pt  ehandler_;

  int                 port_;

  RpcDispatcher       *dispatcher_;
  RpcHttpConsole      *http_console_;

  void                *command_handler_;
};

}
}

#endif

