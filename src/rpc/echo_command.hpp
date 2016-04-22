#ifndef TAIR_RPC_ECHO_COMMAND_HPP_
#define TAIR_RPC_ECHO_COMMAND_HPP_

#include "echo_message.pb.h"
#include "tbsys.h"

namespace tair
{
namespace rpc
{

class EchoCommand : public RpcCommandAbstract<0x0001, tair::packet::EchoMessage, 0x0001, tair::packet::EchoMessage> 
{
 public:

  virtual void DoCommand(void *ctx, RpcSession *session)
  {
    assert(ctx != NULL);
    const tair::packet::EchoMessage *request= dynamic_cast<const tair::packet::EchoMessage *>(session->request());
    
    TBSYS_LOG(INFO, "Echo:%s", request->mesg().c_str()); 

    tair::packet::EchoMessage *response = dynamic_cast<tair::packet::EchoMessage *>(session->mutable_response());
    response->set_mesg(request->mesg().c_str());
    session->Done();
  }
};

}
}

#endif

