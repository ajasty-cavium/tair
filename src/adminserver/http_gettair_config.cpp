/*=============================================================================
#     FileName: http_getall_config.cpp
#         Desc: 
#       Author: yexiang.ych
#        Email: yexiang.ych@taobao.com
#     HomePage: 
#      Version: 0.0.1
#   LastChange: 2014-03-20 10:26:19
#      History:
=============================================================================*/
#include "http_gettair_config.h"
#include "http_config_helper.h"
#include "administrator_handler.h"

namespace tair
{
namespace admin
{

int HttpGetTairConfig::DoCommand(void *h, struct mg_connection *conn)
{
  AdministratorHandler *handler = reinterpret_cast<AdministratorHandler *>(h);
  if (handler->ready == false)
  {
    HttpConfigHelper::ServiceUnavailable(conn);
    return 1;
  }
  std::map<std::string, std::string> params;
  params[HttpConfigHelper::kTidName] = "";
  params[HttpConfigHelper::kCidName] = "";
  Status status = HttpConfigHelper::GetParameterValue(conn, params);
  if (status.ok())
  {
    TairInfo *tair_info = (handler->tair_manager())[params[HttpConfigHelper::kTidName]];
    if (tair_info != NULL && (*tair_info)[params[HttpConfigHelper::kCidName]] != NULL)
    {
      std::stringstream ss;
      HttpConfigHelper::JsonFormatBegin(ss);
      HttpConfigHelper::JsonFormatString(ss, params);
      ss << "\"TairInfo\":" << tair_info->ToString(params[HttpConfigHelper::kCidName]) << ",";
      HttpConfigHelper::JsonFormatEnd(ss, "gettair", status);
      HttpConfigHelper::OKRequest(conn, ss.str());
    }
    else
    {
      status = Status::NotFound(); 
      status.mutable_message() << "tid not found";
      std::map<std::string, int64_t> int_params;
      HttpConfigHelper::OKRequest(conn, "gettair", params, int_params, status);
    }
  }
  else
  {
    HttpConfigHelper::BadRequest(conn, status);
  }
  return 1;
}

}
}
