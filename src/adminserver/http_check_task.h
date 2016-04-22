/*=============================================================================
#     FileName: http_check_task.h
#         Desc: http://localhost:8080/set/?tid=tair_TEST&cid=config_TEST&key=ns&val=v1,v2
#       Author: yexiang.ych
#        Email: yexiang.ych@taobao.com
#     HomePage: 
#      Version: 0.0.1
#   LastChange: 2014-03-28 15:20:35
#      History:
=============================================================================*/
#ifndef ADMIN_HTTP_CHECK_TASK_H_
#define ADMIN_HTTP_CHECK_TASK_H_

#include "tair_rpc.h"
#include <string.h>

namespace tair
{
namespace admin
{

class HttpCheckTask : public tair::rpc::RpcHttpConsoleCommand
{
 public: 
  virtual const Command GetCommand() const
  {
    return HttpCheckTask::DoCommand;
  }

  virtual const std::string uri() const
  {
    return "/checktask/";
  }

 private:
  static int DoCommand(void *handler, struct mg_connection *conn);
};

}
}

#endif

