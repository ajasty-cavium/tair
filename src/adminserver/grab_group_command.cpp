#include "grab_group_command.h"

namespace tair
{
namespace admin
{

const std::string GrabGroupRequest::s_type_name = "GrabGroupRequest";

GrabGroupRequest::GrabGroupRequest()
    : group_version_(0), byte_size_(0)
{
}

void GrabGroupRequest::set_group_name(const std::string& group_name)
{
  this->group_name_ = group_name;
  byte_size_ = 4 + 4 + group_name_.length();
}

tair::rpc::TairMessage* GrabGroupRequest::New()
{
  return new GrabGroupRequest();
}

bool GrabGroupRequest::ParseFromArray(char *buf, int32_t len)
{
  return false;
}

void GrabGroupRequest::SerializeToArray(char *buf, int32_t len)
{
  uint32_t bint = bswap_32(group_version_);
  WriteType(buf, bint);
  bint = bswap_32(group_name_.length());
  WriteType(buf + 4, bint);
  memcpy(buf + 8, group_name_.c_str(), group_name_.length());
  return;
}

const std::string& GrabGroupRequest::GetTypeName() const
{
  return s_type_name;
}

int32_t GrabGroupRequest::ByteSize() const
{
  return byte_size_;
}

const std::string GrabGroupResponse::s_type_name = "GrabGroupResponse";

GrabGroupResponse::GrabGroupResponse()
    : byte_size_(0), version_(0)
{
}

tair::rpc::TairMessage* GrabGroupResponse::New()
{
  return new GrabGroupResponse();
}

bool GrabGroupResponse::ParseFromArray(char *buf, int32_t len)
{
#define READ_32TYPE(x) if (!message_buffer.ReadType(x)) \
                         return false;                 \
                       (x) = bswap_32(x);

#define READ_SIZE(x) READ_32TYPE(x);                   \
                     if ((x) < 0) return false;

  MessageBuffer message_buffer(buf, len);

  READ_32TYPE(version_);
  int32_t addrs_size = 0;
  READ_SIZE(addrs_size);
  addrs_.clear();
  for (int32_t i = 0; i < addrs_size; ++i)
  {
    uint64_t addr = 0;
    if (!message_buffer.ReadType(addr))
      return false;
    addr = bswap_64(addr);
    addrs_.push_back(tair::rpc::RpcContext::Cast2Easy(addr));
  }
  byte_size_ = len;
  return true;
}

void GrabGroupResponse::SerializeToArray(char *buf, int32_t len)
{
  return;
}

const std::string& GrabGroupResponse::GetTypeName() const
{
  return s_type_name;
}

int32_t GrabGroupResponse::ByteSize() const
{
  return byte_size_;
}


}
}
