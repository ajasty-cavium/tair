/*=============================================================================
#     FileName: notify_configuration_command.h
#         Desc: 
#       Author: yexiang.ych
#        Email: yexiang.ych@taobao.com
#     HomePage: 
#      Version: 0.0.1
#   LastChange: 2014-03-24 23:07:49
#      History:
=============================================================================*/
#ifndef ADMIN_NOTIFY_CONFIGURATION_COMMAND_H_
#define ADMIN_NOTIFY_CONFIGURATION_COMMAND_H_

#include <byteswap.h>
#include <string>
#include <string.h>
#include "tair_rpc.h"
#include "task_scheduler.h"

namespace tair
{
namespace admin
{

class NotifyConfigurationRequest : public tair::rpc::TairMessage
{
 public:
  NotifyConfigurationRequest();

  virtual tair::rpc::TairMessage* New();

  virtual bool ParseFromArray(char *buf, int32_t len);

  virtual void SerializeToArray(char *buf, int32_t len);

  virtual const std::string& GetTypeName() const;

  virtual int32_t ByteSize() const;

  void set_version(int64_t last_version, int64_t next_version);

  void set_opt(TaskOperation opt);

  void set_cid(const std::string& cid);
  
  void set_configs(const std::map<std::string, std::string> &configs);

 protected:
  int32_t byte_size_;
  const static std::string s_type_name;
 
 private:
  int64_t last_version_;
  int64_t next_version_;
  int16_t opt_;
  int16_t flag_;
  int32_t data_size_;
  std::string cid_;
  std::string optstr_;
  std::map<std::string, std::string> configs_;
};

class NotifyConfigurationResponse : public tair::rpc::TairMessage
{
 public:
  NotifyConfigurationResponse();

  virtual tair::rpc::TairMessage* New();

  virtual bool ParseFromArray(char *buf, int32_t len);

  virtual void SerializeToArray(char *buf, int32_t len);
  
  virtual const std::string& GetTypeName() const;
  
  virtual int32_t ByteSize() const;
  
  int64_t current_version() const;

  const std::map<std::string, std::string> &configs() const { return configs_; }
  
 protected:
  int32_t byte_size_;
  const static std::string s_type_name;

 private:
  int64_t current_version_;
  int16_t flag_;
  std::map<std::string, std::string> configs_;
};

class NotifyConfigurationCommand : public tair::rpc::TairRpcCommandAbstract
                                        <0x03f3, NotifyConfigurationRequest,
                                         0x0457, NotifyConfigurationResponse>
{
 public:
  virtual void DoCommand(void *handler, tair::rpc::RpcSession *session) {}
};

}
}
#endif

