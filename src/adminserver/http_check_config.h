/*=============================================================================
#     FileName: http_check_config.h
#         Desc: http://localhost:8080/get/?tid=tair_TEST&cid=config_TEST&key=ns
#       Author: yexiang.ych
#        Email: yexiang.ych@taobao.com
#     HomePage: 
#      Version: 0.0.1
#   LastChange: 2014-03-20 14:03:02
#      History:
=============================================================================*/
#ifndef ADMIN_HTTP_CHECK_CONFIG_H_
#define ADMIN_HTTP_CHECK_CONFIG_H_

#include "tair_rpc.h"
#include <string.h>

namespace tair
{
namespace admin
{

class HttpCheckConfig : public tair::rpc::RpcHttpConsoleCommand
{
 public: 
  virtual const Command GetCommand() const
  {
    return HttpCheckConfig::DoCommand;
  }

  virtual const std::string uri() const
  {
    return "/checkconf/";
  }

 private:
  static int DoCommand(void *handler, struct mg_connection *conn);
};

}
}

#endif

