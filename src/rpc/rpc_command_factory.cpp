#include "rpc_command_factory.h"

namespace tair
{
namespace rpc
{

RpcCommandFactory* RpcCommandFactory::inst = NULL;

const std::unordered_map<std::string, RpcHttpConsoleCommand::Command>& RpcCommandFactory::http_commands() const
{
  return RpcCommandFactory::inst->http_commands_;
}

void RpcCommandFactory::Clear()
{
  {
    std::map<int16_t, RpcCommand *>::iterator iter = commands_.begin();
    for (; iter != commands_.end(); ++iter)
    {
      delete iter->second;
    }
    commands_.clear();
  }
  {
    std::map<int16_t, google::protobuf::Message *>::iterator iter = packets_.begin();
    for (; iter != packets_.end(); ++iter)
    {
      delete iter->second;
    }
    packets_.clear();
  }
  {
    std::map<int16_t, TairRpcCommand *>::iterator iter = tair_commands_.begin();
    for (; iter != tair_commands_.end(); ++iter)
    {
      delete iter->second;
    }
    tair_commands_.clear();
  }
  {
    std::map<int16_t, TairMessage *>::iterator iter = tair_packets_.begin();
    for (; iter != tair_packets_.end(); ++iter)
    {
      delete iter->second;
    }
    tair_packets_.clear();
  }
  http_commands_.clear();
}

void RpcCommandFactory::RegisterTairCommand(TairRpcCommand *cmd)
{
  TairMessage *request_message    = cmd->NewRequestMessage();
  TairMessage *response_message   = cmd->NewResponseMessage();
  TBSYS_LOG(INFO, "AddTairCommand request_id:0x%X request_message:%s response_id:0x%X response_message:%s",
      cmd->request_id(), request_message->GetTypeName().c_str(),
      cmd->response_id(), response_message->GetTypeName().c_str());
  assert(tair_commands_.find(cmd->request_id()) == tair_commands_.end());
  tair_commands_[cmd->request_id()] = cmd;
  RegisterTairPacket(cmd->request_id(), request_message);
  RegisterTairPacket(cmd->response_id(), response_message);
}

void RpcCommandFactory::RegisterCommand(RpcCommand *cmd)
{
  google::protobuf::Message *request_message    = cmd->NewRequestMessage();
  google::protobuf::Message *response_message   = cmd->NewResponseMessage();
  TBSYS_LOG(INFO, "AddCommand request_id:0x%X request_message:%s response_id:0x%X response_message:%s",
      cmd->request_id(), request_message->GetTypeName().c_str(),
      cmd->response_id(), response_message->GetTypeName().c_str());
  assert(commands_.find(cmd->request_id()) == commands_.end());
  commands_[cmd->request_id()] = cmd;
  RegisterPacket(cmd->request_id(), request_message);
  RegisterPacket(cmd->response_id(), response_message);
}

void RpcCommandFactory::RegisterTairPacket(int16_t pid, TairMessage *message)
{
  if (tair_packets_.find(pid) == tair_packets_.end())
  {
    tair_packets_[pid] = message;
  }
  else
  {
    assert(tair_packets_.find(pid)->second->GetTypeName() == message->GetTypeName());
    delete message;
  }
}

void RpcCommandFactory::RegisterPacket(int16_t pid, google::protobuf::Message *message)
{
  if (packets_.find(pid) == packets_.end())
  {
    packets_[pid] = message;
  }
  else
  {
    assert(packets_.find(pid)->second->GetTypeName() == message->GetTypeName());
    delete message;
  }
}

void RpcCommandFactory::RegisterHttpCommand(const RpcHttpConsoleCommand &cmd)
{
  const std::string uri = cmd.uri();
  TBSYS_LOG(INFO, "AddHttpCommand uri:%s", uri.c_str());
  http_commands_[uri] = cmd.GetCommand();
}

}
}

