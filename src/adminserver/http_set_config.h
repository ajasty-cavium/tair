/*=============================================================================
#     FileName: http_set_config.h
#         Desc: http://localhost:8080/set/?tid=tair_TEST&cid=config_TEST&key=ns&val=v1,v2
#       Author: yexiang.ych
#        Email: yexiang.ych@taobao.com
#     HomePage: 
#      Version: 0.0.1
#   LastChange: 2014-03-11 14:07:01
#      History:
=============================================================================*/
#ifndef ADMIN_HTTP_SET_CONFIG_H_
#define ADMIN_HTTP_SET_CONFIG_H_

#include "tair_rpc.h"
#include <string.h>

namespace tair
{
namespace admin
{

class HttpSetConfig: public tair::rpc::RpcHttpConsoleCommand
{
 public: 
  virtual const Command GetCommand() const
  {
    return HttpSetConfig::DoCommand;
  }

  virtual const std::string uri() const
  {
    return "/set/";
  }

 private:
  static int DoCommand(void *handler, struct mg_connection *conn);
};

}
}

#endif

