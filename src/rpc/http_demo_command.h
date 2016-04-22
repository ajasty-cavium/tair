#ifndef TAIR_RPC_HTTP_DEMO_COMMAND_H_
#define TAIR_RPC_HTTP_DEMO_COMMAND_H_

#include "rpc_command.hpp"
namespace tair
{
namespace rpc
{

class RpcHttpDemoCommand : public RpcHttpConsoleCommand
{
 public: 
  virtual const Command GetCommand() const
  {
    return RpcHttpDemoCommand::DoCommand;
  }

  virtual const std::string uri() const 
  {
    return "/console/";
  }

 private:
  static int DoCommand(void *handler, struct mg_connection *conn)
  {
    const char *jsons = "{\"Response\":\"Hello Tair Rpc\"}";

    mg_printf(conn, "HTTP/1.0 200 OK\r\n"
        "Content-Length: %d\r\n"
        "Content-Type: application/json\r\n\r\n"
        "%s", 
        strlen(jsons), jsons);
    return 1;
  }
};

}
}

#endif
