#ifndef ADMIN_HTTP_CLUSTER_MANAGER_H_
#define ADMIN_HTTP_CLUSTER_MANAGER_H_

#include "tair_rpc.h"
#include <string.h>

namespace tair
{
namespace admin
{

class HttpAddCluster: public tair::rpc::RpcHttpConsoleCommand
{
 public:
  virtual const Command GetCommand() const
  {
    return HttpAddCluster::DoCommand;
  }

  virtual const std::string uri() const
  {
    return "/add_cluster/";
  }

 private:
  static int DoCommand(void *handler, struct mg_connection *conn);
};

}
}

#endif

