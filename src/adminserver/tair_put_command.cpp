/*=============================================================================
#     FileName: sync_configuration_command.cpp
#         Desc: 
#       Author: yexiang.ych
#        Email: yexiang.ych@taobao.com
#     HomePage: 
#      Version: 0.0.1
#   LastChange: 2014-03-10 19:27:10
#      History:
=============================================================================*/
#include "sync_configuration_command.h"
#include <byteswap.h>
#include <string>
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

const std::string SyncConfigurationRequest::s_type_name = "SyncConfigurationRequest";

SyncConfigurationRequest::SyncConfigurationRequest() : byte_size_(0) {}

tair::rpc::TairMessage* SyncConfigurationRequest::New() 
{
  return new SyncConfigurationRequest();
}

bool SyncConfigurationRequest::ParseFromArray(char *buf, int32_t len) 
{
  return false;
}

void SyncConfigurationRequest::SerializeToArray(char *buf, int32_t len)
{
  buf[0] = 0;                           // 0
  int16_t bint16 = bswap_16(area);
  WriteType(buf + 1, bint16);           // ns
  bint16 = bswap_16(0);
  WriteType(buf + 3, bint16);           // version
  int32_t bint32 = bswap_32(0);
  WriteType(buf + 5, bint32);           // expired
  // Add Key
  bzero(buf + 9, 36);                   // meta
  bint32 = bswap_32(key.length());
  WriteType(buf + 45, bint32);          // key length 
  memcpy(buf + 49, key.c_str(), key.length()); // key content
  // Add Val
  bzero(buf + 49 + key.length(), 36);   // meta
  bint32 = bswap_32(val.length());
  WriteType(buf + 85 + key.length(), bint32);                 // val length
  memcpy(buf + 89 + key.length(), val.c_str(), val.length()); // key content
}

const std::string& SyncConfigurationRequest::GetTypeName() const
{
  return s_type_name;
}

int32_t SyncConfigurationRequest::ByteSize() const
{
  return byte_size_;
}

void SyncConfigurationRequest::SetKeyValue(int16_t area, const std::string& key, const std::string& val)
{
  this->area = area;
  this->key  = key;
  this->val  = val;
  this->byte_size_ = key.length() + val.length() + 89;
}

const std::string SyncConfigurationResponse::s_type_name = "SyncConfigurationResponse";

SyncConfigurationResponse::SyncConfigurationResponse() : byte_size_(0)
{
}

tair::rpc::TairMessage* SyncConfigurationResponse::New() 
{
  return new SyncConfigurationResponse();
}

bool SyncConfigurationResponse::ParseFromArray(char *buf, int32_t len) 
{
  if (len < 12) 
    return false;
  code_ = *reinterpret_cast<int32_t *>(buf + 4);
  code_ = bswap_32(code_);
  byte_size_ = len;
  return true;
}

void SyncConfigurationResponse::SerializeToArray(char *buf, int32_t len)
{
  return ;
}

const std::string& SyncConfigurationResponse::GetTypeName() const
{
  return s_type_name;
}

int32_t SyncConfigurationResponse::ByteSize() const
{
  return byte_size_;
}

int32_t SyncConfigurationResponse::code() const
{
  return code_;
}

}
}

