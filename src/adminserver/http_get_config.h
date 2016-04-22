/*=============================================================================
#     FileName: http_get_config.h
#         Desc: http://localhost:8080/get/?tid=tair_TEST&cid=config_TEST&key=ns
#       Author: yexiang.ych
#        Email: yexiang.ych@taobao.com
#     HomePage: 
#      Version: 0.0.1
#   LastChange: 2014-03-12 16:53:43
#      History:
=============================================================================*/
#ifndef ADMIN_HTTP_GET_CONFIG_H_
#define ADMIN_HTTP_GET_CONFIG_H_

#include "tair_rpc.h"
#include <string.h>

namespace tair
{
namespace admin
{

class HttpGetConfig: public tair::rpc::RpcHttpConsoleCommand
{
 public: 
  virtual const Command GetCommand() const
  {
    return HttpGetConfig::DoCommand;
  }

  virtual const std::string uri() const
  {
    return "/get/";
  }

 private:
  static int DoCommand(void *handler, struct mg_connection *conn);
};

}
}

#endif

