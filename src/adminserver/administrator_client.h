/*=============================================================================
#     FileName: administrator_client.h
#         Desc: 
#       Author: yexiang.ych
#        Email: yexiang.ych@taobao.com
#     HomePage: 
#      Version: 0.0.1
#   LastChange: 2014-03-10 14:22:56
#      History:
=============================================================================*/
#ifndef ADMIN_ADMINISTRATOR_CLIENT_H_
#define ADMIN_ADMINISTRATOR_CLIENT_H_

#include <string>
#include "tair_rpc.h"
#include "task_scheduler.h"

namespace tair
{
namespace rpc
{
class RpcClientContext;
}

namespace admin
{

class AdministratorClient
{
 public:
  AdministratorClient();

  ~AdministratorClient();

  int64_t Set(easy_addr_t addr, 
           const std::string& cid, 
           const std::string& key, 
           const std::string& val, 
           int64_t last_version, int64_t current_version, long timeout_ms);

  int64_t GrabDataServers(const easy_addr_t &cs_addr,
                          const std::string &group_name,
                          std::vector<easy_addr_t> &addrs,
                          int64_t timeout_ms);

  int64_t NotifyConfiguration(const easy_addr_t &addr, 
                              const std::string &cid, 
                              const std::map<std::string, std::string> &configs,
                              int64_t last_version, int64_t next_version, 
                              TaskOperation opt, int64_t timeout_ms);

  int64_t GrabConfiguration(const easy_addr_t &addr, 
                            const std::string &cid, 
                            std::map<std::string, std::string> &configs,
                            int64_t timeout_ms);

  const static std::map<std::string, std::string> kEmpty;

  void set_rpc_client_context(tair::rpc::RpcClientContext *context)
  {
    this->context_ = context;
  }

 private:
  tair::rpc::RpcClientContext *context_;
};

}
}

#endif

