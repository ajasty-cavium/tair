#ifndef TAIR_RPC_COMMAND_FACTORY_H_
#define TAIR_RPC_COMMAND_FACTORY_H_

#include <typeinfo>
#include <tbsys.h>
#include <map>
#include "unordered_map.h"
#include "rpc_command.hpp"

namespace tair
{
namespace rpc
{ 
class RpcCommandFactory
{
 public:
  static void Initialize()
  {
    assert(inst == NULL);
    RpcCommandFactory::inst = new RpcCommandFactory();
  }

  static RpcCommandFactory* Instance()
  {
    return RpcCommandFactory::inst;
  }

  static void Destroy()
  {
    delete RpcCommandFactory::inst;
    RpcCommandFactory::inst = NULL;
  }

  inline RpcCommand* FindCommand(int16_t id)
  {
    std::map<int16_t, RpcCommand*>::iterator iter = commands_.find(id);
    if (iter != commands_.end())
    {
      return iter->second;
    }
    return NULL;
  }

  static google::protobuf::Message* NewPacketMessage(int16_t id)
  {
    std::map<int16_t, google::protobuf::Message *>::iterator iter = RpcCommandFactory::inst->packets_.find(id);
    if (iter != RpcCommandFactory::inst->packets_.end())
    {
      return iter->second->New();
    }
    return NULL;
  }

  static TairMessage* NewPacketTairMessage(int16_t id)
  {
    std::map<int16_t, TairMessage *>::iterator iter = RpcCommandFactory::inst->tair_packets_.find(id);
    if (iter != RpcCommandFactory::inst->tair_packets_.end())
    {
      return iter->second->New();
    }
    return NULL;
  }

  const std::unordered_map<std::string, RpcHttpConsoleCommand::Command>& http_commands() const;

  void RegisterHttpCommand(const RpcHttpConsoleCommand &cmd);

  void RegisterTairCommand(TairRpcCommand *cmd);
  
  void RegisterCommand(RpcCommand *cmd);
  
  ~RpcCommandFactory()
  {
    Clear();
  }
 private:
  void Clear();

  void RegisterTairPacket(int16_t pid, TairMessage *message);
  
  void RegisterPacket(int16_t pid, google::protobuf::Message *message);

  RpcCommandFactory(){}

  friend class RpcCommandFactoryBuilder;
 
 private:
  std::map<int16_t, RpcCommand*>                  commands_;
  std::map<int16_t, google::protobuf::Message *>  packets_;

  std::map<int16_t, TairMessage *>    tair_packets_;
  std::map<int16_t, TairRpcCommand*>  tair_commands_;

  std::unordered_map<std::string, RpcHttpConsoleCommand::Command> http_commands_;

  static RpcCommandFactory *inst;
};

}
}

#endif

