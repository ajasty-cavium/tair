/*=============================================================================
#     FileName: grab_group_command.h
#         Desc:
#       Author: yexiang.ych
#        Email: yexiang.ych@taobao.com
#     HomePage:
#      Version: 0.0.1
#   LastChange: 2014-03-18 17:08:49
#      History:
=============================================================================*/
#ifndef ADMIN_GRAB_GROUP_COMMAND_H_
#define ADMIN_GRAB_GROUP_COMMAND_H_

#include <byteswap.h>
#include <string>
#include <string.h>
#include "tair_rpc.h"
#include <base_packet.hpp>

namespace tair
{
namespace admin
{

class GrabGroupRequest : public tair::rpc::TairMessage
{
 public:
  GrabGroupRequest();

  virtual tair::rpc::TairMessage* New();

  virtual bool ParseFromArray(char *buf, int32_t len);

  virtual void SerializeToArray(char *buf, int32_t len);

  virtual const std::string& GetTypeName() const;

  virtual int32_t ByteSize() const;

  void set_group_name(const std::string& group_name);

 protected:
  std::string                group_name_;
  uint32_t                   group_version_;
  int32_t                    byte_size_;
  const static std::string   s_type_name;
};

class GrabGroupResponse : public tair::rpc::TairMessage
{
 public:
  GrabGroupResponse();

  virtual tair::rpc::TairMessage* New();

  virtual bool ParseFromArray(char *buf, int32_t len);

  virtual void SerializeToArray(char *buf, int32_t len);

  virtual const std::string& GetTypeName() const;

  virtual int32_t ByteSize() const;

  const std::vector<easy_addr_t>& addrs() const
  {
    return addrs_;
  }

  int64_t version() const
  {
    return version_;
  }
 private:
  int32_t                             byte_size_;
  std::map<std::string, std::string>  config_map_;
  std::vector<easy_addr_t>            addrs_;
  int32_t                             version_;
  const static std::string s_type_name;
};

class GrabGroupCommand : public tair::rpc::TairRpcCommandAbstract
  <tair::TAIR_REQ_GET_GROUP_NON_DOWN_DATASERVER_PACKET, GrabGroupRequest,
  tair::TAIR_RESP_GET_GROUP_NON_DOWN_DATASERVER_PACKET, GrabGroupResponse>
{
 public:
  virtual void DoCommand(void *handler, tair::rpc::RpcSession *session) {}
};

}
}
#endif
