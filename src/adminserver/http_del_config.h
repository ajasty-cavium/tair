/*=============================================================================
#     FileName: http_del_config.h
#         Desc: http://localhost:8080/del/?tid=tair_TEST&cid=config_TEST&key=ns
#       Author: yexiang.ych
#        Email: yexiang.ych@taobao.com
#     HomePage: 
#      Version: 0.0.1
#   LastChange: 2014-03-28 16:39:42
#      History:
=============================================================================*/
#ifndef ADMIN_HTTP_DEL_CONFIG_H_
#define ADMIN_HTTP_DEL_CONFIG_H_

#include "tair_rpc.h"
#include <string.h>

namespace tair
{
namespace admin
{

class HttpDelConfig: public tair::rpc::RpcHttpConsoleCommand
{
 public: 
  virtual const Command GetCommand() const
  {
    return HttpDelConfig::DoCommand;
  }

  virtual const std::string uri() const
  {
    return "/del/";
  }

 private:
  static int DoCommand(void *handler, struct mg_connection *conn);
};

}
}

#endif

