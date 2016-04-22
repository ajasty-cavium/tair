/*=============================================================================
#     FileName: http_config_helper.h
#         Desc: 
#       Author: yexiang.ych
#        Email: yexiang.ych@taobao.com
#     HomePage: 
#      Version: 0.0.1
#   LastChange: 2014-03-12 13:31:34
#      History:
=============================================================================*/
#ifndef ADMIN_HTTP_CONFIG_HELPER_H_
#define ADMIN_HTTP_CONFIG_HELPER_H_

#include <string>
#include <map>
#include "tair_rpc.h"
#include "status.h"

namespace tair
{
namespace admin
{

class HttpConfigHelper
{
 public:
  static const std::string kTidName;
  static const std::string kCidName;
  static const std::string kKeyName;
  static const std::string kValName;
  static const std::string kUUIDName;
  static const std::string kMasterName;
  static const std::string kSlaveName;
  static const std::string kGroupName;

  static const size_t kParameterLen;

  static Status GetParameterValue(struct mg_connection *conn,
                                  std::map<std::string, std::string> &params);
  static void JsonFormatEnd(std::stringstream &ss,
                            const std::string &method,
                            const Status &status);
  static void JsonFormatString(std::stringstream &ss,
                               const std::map<std::string, std::string> &params);
  static void JsonFormatInt(std::stringstream &ss,
                            const std::map<std::string, int64_t> &params);

  static void JsonFormatBegin(std::stringstream &ss);

  static void BadRequest(struct mg_connection *conn, const Status &s);

  static void OKRequest(struct mg_connection *conn, 
                        const std::string &content);
  static void OKRequest(struct mg_connection *conn, 
                        const std::string &method,
                        const std::map<std::string, std::string> &params,
                        const std::map<std::string, int64_t> &int_params,
                        const Status &status);

  static void ServiceUnavailable(struct mg_connection *conn);
};

}
}

#endif
