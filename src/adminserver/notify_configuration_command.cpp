/*=============================================================================
#     FileName: notify_configuration_command.cpp
#         Desc: 
#       Author: yexiang.ych
#        Email: yexiang.ych@taobao.com
#     HomePage: 
#      Version: 0.0.1
#   LastChange: 2014-03-24 23:08:38
#      History:
=============================================================================*/
#include "notify_configuration_command.h"
#include <byteswap.h>
#include <string>
#include <map>
#include <string.h>
#include "tair_rpc.h"

namespace tair
{
namespace admin
{

template<class T>
void WriteType(char *buf, const T t)
{
  memcpy(buf, reinterpret_cast<const char *>(&t), sizeof(t));
}

const std::string NotifyConfigurationRequest::s_type_name = "NotifyConfigurationRequest";

NotifyConfigurationRequest::NotifyConfigurationRequest() 
    : byte_size_(26) 
    , last_version_(0), next_version_(0)
    , opt_(0), flag_(0), data_size_(0)
{}

tair::rpc::TairMessage* NotifyConfigurationRequest::New() 
{
  return new NotifyConfigurationRequest();
}

void NotifyConfigurationRequest::set_version(int64_t last_version, int64_t next_version)
{
  this->last_version_ = last_version;
  this->next_version_ = next_version;
  return;
}

void NotifyConfigurationRequest::set_opt(TaskOperation opt)
{
  this->opt_ = opt;
  this->byte_size_ -= this->optstr_.length();
  switch(opt_)
  {
  case kSet:
    this->optstr_ = "SET";
    break;
  case kDel:
    this->optstr_ = "DEL";
    break;
   case kCheck:
    this->optstr_ = "CHECK";
    break; 
   case kSync:
    this->optstr_ = "SYNC";
    break;
   case kGet:
    this->optstr_ = "GET";
    break;
   default:
    assert(false);
    break;
  }
  this->byte_size_ += this->optstr_.length();
}


void NotifyConfigurationRequest::set_cid(const std::string& cid)
{
  byte_size_ += cid.length() - this->cid_.length();
  this->cid_ = cid;
}

void NotifyConfigurationRequest::set_configs(const std::map<std::string, std::string> &configs)
{
  this->configs_    = configs;
  this->data_size_  = 0;
  std::map<std::string, std::string>::iterator iter       = configs_.begin();
  std::map<std::string, std::string>::iterator iter_end   = configs_.end();
  for (; iter != iter_end; ++iter)
  {
    this->data_size_ += 4 + iter->first.length() + iter->second.length();
  }
  this->byte_size_ += this->data_size_;
}

bool NotifyConfigurationRequest::ParseFromArray(char *buf, int32_t len) 
{
  return false;
}

void NotifyConfigurationRequest::SerializeToArray(char *buf, int32_t len)
{
  MessageBuffer message_buffer(buf, len);
  if (message_buffer.WriteType(bswap_64(last_version_)) == false)
    assert(false);
  if (message_buffer.WriteType(bswap_64(next_version_)) == false) 
    assert(false);
  if (message_buffer.WriteType(bswap_16(flag_)) == false)
    assert(false);
  if (message_buffer.WriteLittleString(optstr_) == false)
    assert(false);
  if (message_buffer.WriteLittleString(cid_) == false)
    assert(false);
  //TODO: compressed support
  if (message_buffer.WriteType(bswap_32(configs_.size())) == false)
    assert(false);
  std::map<std::string, std::string>::iterator iter       = configs_.begin();
  std::map<std::string, std::string>::iterator iter_end   = configs_.end();
  for (; iter != iter_end; ++iter)
  {
    if (message_buffer.WriteLittleString(iter->first) == false)
      assert(false);
    if (message_buffer.WriteLittleString(iter->second) == false)
      assert(false);
  }
}

const std::string& NotifyConfigurationRequest::GetTypeName() const
{
  return s_type_name;
}

int32_t NotifyConfigurationRequest::ByteSize() const
{
  return byte_size_;
}

const std::string NotifyConfigurationResponse::s_type_name = "NotifyConfigurationResponse";

NotifyConfigurationResponse::NotifyConfigurationResponse() : byte_size_(0), current_version_(0), flag_(0)
{
}

tair::rpc::TairMessage* NotifyConfigurationResponse::New() 
{
  return new NotifyConfigurationResponse();
}

bool NotifyConfigurationResponse::ParseFromArray(char *buf, int32_t len) 
{
  byte_size_ = 8 + 2 + 4;
  MessageBuffer message_buffer(buf, len);
  message_buffer.ReadType(current_version_);
  current_version_ = bswap_64(current_version_);
  message_buffer.ReadType(flag_);
  flag_ = bswap_16(flag_);
  int32_t size = 0;
  message_buffer.ReadType(size);
  size  = bswap_32(size);
  for(int32_t i = 0; i < size; ++i)
  {
    std::string key; 
    message_buffer.ReadLittleString(key);
    std::string val; 
    message_buffer.ReadLittleString(val);
    byte_size_ += 4 + key.length() + val.length();
    configs_[key] = val;
  }
  return true;
}

void NotifyConfigurationResponse::SerializeToArray(char *buf, int32_t len)
{
  return ;
}

const std::string& NotifyConfigurationResponse::GetTypeName() const
{
  return s_type_name;
}

int32_t NotifyConfigurationResponse::ByteSize() const
{
  return byte_size_;
}

int64_t NotifyConfigurationResponse::current_version() const
{ 
  return current_version_; 
}

}
}

