/*=============================================================================
#     FileName: tair_manager.h
#         Desc: 
#       Author: yexiang.ych
#        Email: yexiang.ych@taobao.com
#     HomePage: 
#      Version: 0.0.1
#   LastChange: 2014-03-13 22:48:54
#      History:
=============================================================================*/
#ifndef ADMIN_TAIR_MANAGER_H_
#define ADMIN_TAIR_MANAGER_H_

#include <pthread.h>
#include <map>
#include <vector>
#include "tair_rpc.h"
#include "status.h"

namespace tair
{
namespace admin
{

class TairConfiguration;
class TairInfo;

typedef std::map<std::string, TairConfiguration>::iterator TairConfigurationIterator;
typedef std::map<std::string, TairInfo>::iterator TairClusterIterator;

class TairConfiguration
{
 public:
  TairConfiguration();

/*  TairConfiguration(const TairConfiguration &rhs)
  {
    rhs.Lock();
    this->entries_  = rhs.entries_;
    this->version_  = rhs.version_;
    this->size_     = rhs.size_;
    this->cid_      = rhs.cid_;
    this->last_check_time_ = rhs.last_check_time_;
    rhs.Unlock();
    pthread_mutex_init(&mutex_, NULL);
  }*/

/*  TairConfiguration& operator=(const TairConfiguration &rhs)
  {
    rhs.Lock();
    this->entries_  = rhs.entries_;
    this->version_  = rhs.version_;
    this->size_     = rhs.size_;
    this->cid_      = rhs.cid_;
    this->last_check_time_ = rhs.last_check_time_;
    rhs.Unlock();
    pthread_mutex_init(&mutex_, NULL);
    return *this;
  }*/

  ~TairConfiguration();
  /**
   * @return: Set Del return prev version; Get return current version
   * @key: configuration key
   * @val: configuration val
   */
  void Set(const std::string &key, const std::string &val, 
             int64_t *last_version, int64_t *next_version);

  Status BatchSet(const std::map<std::string, std::string> &configs, 
                  int64_t *last_version, int64_t *next_version);

  Status Del(const std::string &key, int64_t *last_version, int64_t *next_version);

  Status Get(const std::string &key, std::string &val, int64_t &version);

  void Lock() { pthread_mutex_lock(&mutex_); }
  void Unlock() { pthread_mutex_unlock(&mutex_); }

  int64_t version();

  time_t last_check_time() { return last_check_time_; }

  void set_last_check_time(time_t t) { last_check_time_ = t; }
  
  const std::map<std::string, std::string>& entries() { return entries_; }

  std::string ToString();

  void set_cid(const std::string& cid)
  {
    this->cid_ = cid;
  }

  const std::string& cid() const 
  { 
    return this->cid_; 
  }

 private:
  void UpdateVersion(int64_t *last_version, int64_t *next_version);

 private:
  // MAP<Key, Val>
  std::map<std::string, std::string>  entries_;
  int64_t                             version_;
  int32_t                             size_;
  time_t                              last_check_time_;
  std::string                         cid_;
  mutable pthread_mutex_t             mutex_;
};

class TairInfo
{
 public:
  TairInfo();

  ~TairInfo();

  void set_tid(const std::string& tid)
  {
    this->tid_ = tid;
  }

  const std::string& tid() const
  {
    return this->tid_;
  }

  void set_config_server_addr(const easy_addr_t &master, 
                              const easy_addr_t &slave, 
                              const std::string &group_name);

  void GrabAddresses(std::vector<easy_addr_t> &addrs) const;

  bool MaybeDataServerChanged(int64_t version, std::vector<easy_addr_t> &addrs); 

  const easy_addr_t &master_address() { return master_addr_; }

  const easy_addr_t &slave_address() { return slave_addr_; }

  const std::string &group_name() { return group_name_; }

  TairConfiguration* operator[](const std::string &cid)
  {
    return GetTairConfiguration(cid);
  }

  /*void RegisterTairConfiguration(const std::string& cid)
  {
    configurations_[cid].set_cid(cid);
  }*/

  void RegisterTairConfiguration(const TairConfiguration& config)
  {
    configurations_[config.cid()] = config;
  }

  TairConfigurationIterator begin()
  {
    return configurations_.begin();
  }

  TairConfigurationIterator end()
  {
    return configurations_.end();
  }

  std::string ToAddressString() const;

  std::string ToString(const std::string &cid = "");

  void Wlock() const { pthread_rwlock_wrlock(&tair_info_rwlock_); }

  void Rlock() const { pthread_rwlock_rdlock(&tair_info_rwlock_); }

  void Unlock() const { pthread_rwlock_unlock(&tair_info_rwlock_); }

 private:
  TairConfiguration* GetTairConfiguration(const std::string& cid);

 private:
  int64_t                   version_;
  easy_addr_t               master_addr_;
  easy_addr_t               slave_addr_;
  std::string               group_name_;
  std::string               group_name_str_;
  std::string               tid_;
  std::vector<easy_addr_t>  ds_addrs_;
  mutable pthread_rwlock_t  tair_info_rwlock_;

  std::map<std::string, TairConfiguration> configurations_;
};

class TairManager
{
 public:
  TairManager();

  ~TairManager();

  void RegisterTairInfo(const std::string &tid,
                        const std::vector<TairConfiguration> &cids,
                        const easy_addr_t &master_addr,
                        const easy_addr_t &slave_addr,
                        const std::string &group_name);

  void RegisterTairInfo(const std::string &tid,
                        const std::vector<std::string> &cids,
                        const easy_addr_t &master_addr,
                        const easy_addr_t &slave_addr,
                        const std::string &group_name);

  void RegisterTairInfo(const std::string &tid,
                        const std::vector<TairConfiguration> &cids,
                        TairInfo &ti);

  TairInfo* operator[](const std::string &tid)
  {
    return GetTairInfo(tid);
  }

  TairConfiguration* GetTairConfiguration(const std::string &tid,
                                          const std::string &cid);

  void tairclusters(std::map<std::string, TairInfo> &tcs)
  {
    Rlock();
    tcs = tairclusters_;
    Unlock();
  }

  TairClusterIterator begin()
  {
    return tairclusters_.begin();
  }

  TairClusterIterator end()
  {
    return tairclusters_.end();
  }

  void Wlock() const { pthread_rwlock_wrlock(&tairclusters_rwlock_); }

  void Rlock() const { pthread_rwlock_rdlock(&tairclusters_rwlock_); }

  void Unlock() const { pthread_rwlock_unlock(&tairclusters_rwlock_); }

 private:
  TairInfo* GetTairInfo(const std::string &tid);

 private:
  mutable pthread_rwlock_t        tairclusters_rwlock_;
  std::map<std::string, TairInfo> tairclusters_;
};

}
}

#endif
