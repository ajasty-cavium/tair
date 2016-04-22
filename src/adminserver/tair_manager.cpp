/*============================================================================= configurations_;
#     FileName: tair_manager.cpp
#         Desc: 
#       Author: yexiang.ych
#        Email: yexiang.ych@taobao.com
#     HomePage: 
#      Version: 0.0.1
#   LastChange: 2014-03-13 22:40:57
#      History:
=============================================================================*/
#include "tair_manager.h"
#include "murmurhash2.h"

namespace tair
{
namespace admin
{

TairManager::TairManager()
{
  pthread_rwlock_init(&tairclusters_rwlock_, NULL);
}

TairManager::~TairManager()
{
  pthread_rwlock_destroy(&tairclusters_rwlock_);
}

/*void TairManager::RegisterTairInfo(const std::string &tid,
                                   const std::vector<std::string> &cids,
                                   const easy_addr_t &master_addr,
                                   const easy_addr_t &slave_addr,
                                   const std::string &group_name)
{ // Has not been called
  TairInfo &ti = tairclusters_[tid];
  ti.set_config_server_addr(master_addr, slave_addr, group_name);
  ti.set_tid(tid);
  TBSYS_LOG(INFO, "Register TairInfo:%s", ti.ToAddressString().c_str());
  for(size_t i = 0; i < cids.size(); ++i)
  {
    ti.RegisterTairConfiguration(cids[i]);
  }
}*/

void TairManager::RegisterTairInfo(const std::string &tid,
                                   const std::vector<TairConfiguration> &cids,
                                   TairInfo &ti)
{
  TBSYS_LOG(INFO, "Register TairInfo:%s", ti.ToAddressString().c_str());
  for(size_t i = 0; i < cids.size(); ++i)
  {
    ti.RegisterTairConfiguration(cids[i]);
  }
  Wlock();
  tairclusters_[tid] = ti;
  Unlock();
}

void TairManager::RegisterTairInfo(const std::string &tid,
                                   const std::vector<TairConfiguration> &cids,
                                   const easy_addr_t &master_addr,
                                   const easy_addr_t &slave_addr,
                                   const std::string &group_name)
{
  TairInfo &ti = tairclusters_[tid];
  ti.set_config_server_addr(master_addr, slave_addr, group_name);
  ti.set_tid(tid);
  TBSYS_LOG(INFO, "Register TairInfo:%s", ti.ToAddressString().c_str());
  for(size_t i = 0; i < cids.size(); ++i)
  {
    ti.RegisterTairConfiguration(cids[i]);
  }
}

TairInfo* TairManager::GetTairInfo(const std::string &tid)
{
  Rlock();
  std::map<std::string, TairInfo>::iterator iter = tairclusters_.find(tid);
  if (iter != tairclusters_.end())
  {
    Unlock();
    return &(iter->second);
  }
  Unlock();
  return NULL;
}

std::string TairConfiguration::ToString()
{
  std::stringstream ss;
  ss << "{\"Content\":{";
  Lock();
  std::map<std::string, std::string>::iterator iter = entries_.begin();
  bool first = true;
  for (; iter != entries_.end(); ++iter)
  {
    if (!first)
      ss << ",";
    ss << "\"" << iter->first << "\":\"" << iter->second << "\"";
    first = false;
  }
  ss << "},\"ConfigurationVersion\":" << version() << "}";
  Unlock();
  return ss.str();
}

TairConfiguration* TairManager::GetTairConfiguration(const std::string& tid, const std::string& cid)
{
  TairInfo *tair_info = GetTairInfo(tid);
  if (tair_info != NULL)
  {
    return (*tair_info)[cid];
  }
  return NULL;
}

std::string TairInfo::ToAddressString() const
{
  std::stringstream ss;
  ss << "{\"Tid\":\"" << tid_ << "\",\"MasterAddress\":\"";
  tair::rpc::RpcContext::Cast2String(master_addr_, ss);
  ss << "\",\"SlaveAddress\":\"";
  tair::rpc::RpcContext::Cast2String(slave_addr_, ss);
  ss << "\",\"GroupName\":\"" << group_name_str_;
  Rlock();
  ss << "\",\"ServerTableVersion\":\""  << version_;
  ss << "\",\"Servers\":[";
  for (size_t i = 0; i < ds_addrs_.size(); ++i)
  {
    if (i != 0)
      ss << ", ";
    ss << "\"";
    tair::rpc::RpcContext::Cast2String(ds_addrs_[i], ss);
    ss << "\"";
  }
  Unlock();
  ss << "]";
  ss << "}";
  return ss.str();
}

std::string TairInfo::ToString(const std::string &cid)
{
  std::stringstream ss;
  ss << "{\"Tid\":\"" << tid_ << "\",\"MasterAddress\":\"";
  tair::rpc::RpcContext::Cast2String(master_addr_, ss);
  ss << "\",\"SlaveAddress\":\"";
  tair::rpc::RpcContext::Cast2String(slave_addr_, ss);
  ss << "\",\"GroupName\":\""    << group_name_str_;
  Rlock();
  ss << "\",\"ServerTableVersion\":\""  << version_;
  ss << "\",\"Servers\":[";
  for (size_t i = 0; i < ds_addrs_.size(); ++i)
  {
    if (i != 0)
      ss << ", ";
    ss << "\"";
    tair::rpc::RpcContext::Cast2String(ds_addrs_[i], ss);
    ss << "\"";
  }
  Unlock();
  ss << "]";
  ss << ",\"Configurations\":{";
  if (cid == "")
  {
    TairConfigurationIterator iter     = begin();
    TairConfigurationIterator iter_end = end();
    bool first = true;
    for(; iter != iter_end; ++iter)
    {
      if (!first)
        ss << ",";
      ss << "\"" << iter->first << "\":" << iter->second.ToString() << "";
      first = false;
    }
  }
  else
  {
    TairConfiguration *configuration = (*this)[cid];
    if (configuration != NULL)
    {
      ss << "\"" << cid << "\":" << configuration->ToString() << "";
    }
  }
  ss << "}";
  ss << "}";
  return ss.str();
}

void TairInfo::GrabAddresses(std::vector<easy_addr_t> &addrs) const
{
  Rlock();
  addrs.assign(ds_addrs_.begin(), ds_addrs_.end());
  Unlock();
  return ;
}

bool CompAddr(const easy_addr_t& i, const easy_addr_t& j)
{
  return memcmp(&i, &j, sizeof(easy_addr_t));
}

bool EqualAddr(const easy_addr_t& i, const easy_addr_t& j)
{
    return memcmp(&i, &j, sizeof(easy_addr_t)) == 0;
}

bool TairInfo::MaybeDataServerChanged(int64_t version, std::vector<easy_addr_t> &addrs)
{
  if (TBSYS_LOGGER._level >= TBSYS_LOG_LEVEL_DEBUG)
  {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < addrs.size(); ++i)
    {
      if (i != 0)
        ss << ", ";
      tair::rpc::RpcContext::Cast2String(addrs[i], ss);
    }
    ss << "]";
    std::string new_addrs = ss.str();
    TBSYS_LOG(DEBUG, "Grab DataServers NewVersion:%ld NewServers:%s TairInfo:%s",
              version, new_addrs.c_str(), ToAddressString().c_str());
  }
  bool changed = false;
  std::sort(addrs.begin(), addrs.end(), CompAddr);
  Wlock();
  if (version != version_ || addrs.size() != this->ds_addrs_.size() ||
      std::equal(addrs.begin(), addrs.end(), this->ds_addrs_.begin(), EqualAddr) == false)
  {
    this->version_     = version;
    this->ds_addrs_    = addrs;
    changed            = true;
  }
  Unlock();
  if (changed)
  {
    TBSYS_LOG(INFO, "Has changed DataServers TairInfo:%s",
              ToAddressString().c_str());
  }
  return changed;
}

TairInfo::TairInfo()
    : version_(0)
{
  pthread_rwlock_init(&tair_info_rwlock_, NULL);
}

TairInfo::~TairInfo()
{
  pthread_rwlock_destroy(&tair_info_rwlock_);
}

void TairInfo::set_config_server_addr(const easy_addr_t &master,
                                      const easy_addr_t &slave,
                                      const std::string &group_name)
{
  assert(group_name.length() > 0);
  this->master_addr_ = master;
  this->slave_addr_  = slave;
  this->group_name_  = group_name; 
  if (*(group_name.rbegin()) != '\0')
  {
    this->group_name_.push_back('\0');
    this->group_name_str_ = group_name;
  }
  else
  {
    this->group_name_str_.assign(group_name.begin(), group_name.end() - 1);
  }
  this->version_     = 0;
}

TairConfiguration* TairInfo::GetTairConfiguration(const std::string& cid)
{
  std::map<std::string, TairConfiguration>::iterator iter = configurations_.find(cid);
  if (iter != configurations_.end())
  {
    return &iter->second;
  }
  return NULL;
}

Status TairConfiguration::BatchSet(const std::map<std::string, std::string> &configs,
                                   int64_t *last_version, int64_t *next_version)
{
  Status s = Status::OK();
  std::map<std::string, std::string>::const_iterator iter     = configs.begin();
  std::map<std::string, std::string>::const_iterator iter_end = configs.end();
  for(; iter != iter_end; ++iter)
  {
    entries_[iter->first] = iter->second;
    size_ += iter->first.length() + iter->second.length();
  }
  UpdateVersion(last_version, next_version);
  return s;
}

void TairConfiguration::Set(const std::string &key, const std::string &val, 
                              int64_t *last_version, int64_t *next_version)
{
  std::map<std::string, std::string>::iterator iter = entries_.find(key);
  if (iter == entries_.end())
  {
    size_ += key.length() + val.length();
    entries_[key] = val;
    UpdateVersion(last_version, next_version);
  }
  else if (iter->second != val)
  {
    size_ += val.length() - iter->second.length();
    iter->second = val;
    UpdateVersion(last_version, next_version);
  }
  else
  {
    *last_version = *next_version = version_;
  }
  assert(size_ > 0);
}

Status TairConfiguration::Del(const std::string &key, int64_t *last_version, int64_t *next_version)
{
  Status s = Status::OK();
  std::map<std::string, std::string>::iterator iter = entries_.find(key);
  if (iter != entries_.end())
  {
    size_ -= key.length() + iter->second.length();
    entries_.erase(iter);
    UpdateVersion(last_version, next_version);
  }
  else
  {
    s = Status::NotFound();
    s.mutable_message() << "key:" << key << " not found key ";
  }
  return s;
}

Status TairConfiguration::Get(const std::string &key, std::string &val, int64_t &version)
{
  Status s = Status::OK();
  std::map<std::string, std::string>::iterator iter = entries_.find(key);
  if (iter != entries_.end())
  {
    val = iter->second;
    version = version_;
  }
  else
  {
    s = Status::NotFound();
    s.mutable_message() << "key:" << key << " not found key ";
  }
  return s;
}

void TairConfiguration::UpdateVersion(int64_t *last_version, int64_t *next_version)
{
  CMurmurHash2A hasher;
  hasher.Begin();
  std::map<std::string, std::string>::iterator iter = entries_.begin();
  for (; iter != entries_.end(); ++iter)
  {
    hasher.Add(reinterpret_cast<const unsigned char *>(iter->first.c_str()), 
               iter->first.length());
    hasher.Add(reinterpret_cast<const unsigned char *>(iter->second.c_str()), 
               iter->second.length());
  }
  *last_version = version_;
  *next_version = version_ = hasher.End();
}

int64_t TairConfiguration::version() 
{ 
  return version_; 
}

TairConfiguration::TairConfiguration() 
    : version_(0), size_(0), last_check_time_(0)
{
  pthread_mutex_init(&mutex_, NULL);
}

TairConfiguration::~TairConfiguration()
{
  pthread_mutex_destroy(&mutex_);
}


}
}

