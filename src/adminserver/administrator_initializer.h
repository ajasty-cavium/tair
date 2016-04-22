/*=============================================================================
#     FileName: administrator_initializer.h
#         Desc: 
#       Author: yexiang.ych
#        Email: yexiang.ych@taobao.com
#     HomePage: 
#      Version: 0.0.1
#   LastChange: 2014-03-17 10:50:40
#      History:
=============================================================================*/
#ifndef ADMIN_ADMINISTRATOR_INITIALIZER_H_
#define ADMIN_ADMINISTRATOR_INITIALIZER_H_

#include "tair_rpc.h"
#include "status.h"
#include "administrator_handler.h"

namespace tair
{
namespace admin
{

class AdministratorClient;
class TairManager;
class TairInfo;

class AdministratorInitializer : public tair::rpc::RpcThread<void *>
{
 public:
  AdministratorInitializer();

  virtual ~AdministratorInitializer();

  Status InitializeHandler(AdministratorHandler *handler);

  void MaybeSyncConfiguration(AdministratorClient &client, TairInfo *tair_info);
 protected:
  virtual void Run(void *&arg, int32_t index);

  virtual std::string thread_name() const { return "admin-initializer"; }

 private:
  AdministratorHandler    *handler_;
  std::vector<TairInfo *> tair_info_queue_;
  int32_t                 queue_index_;

  const static std::string kSelectTairsSQL;
  const static std::string kSelectConfigsSQL;
};

}
}

#endif

