/*=============================================================================
#     FileName: http_set_config.cpp
#         Desc: 
#       Author: yexiang.ych
#        Email: yexiang.ych@taobao.com
#     HomePage: 
#      Version: 0.0.1
#   LastChange: 2014-03-12 13:19:20
#      History:
=============================================================================*/
#include "http_set_config.h"
#include "http_config_helper.h"
#include "administrator_handler.h"

namespace tair
{
namespace admin
{

int HttpSetConfig::DoCommand(void *h, struct mg_connection *conn)
{
  AdministratorHandler *handler = reinterpret_cast<AdministratorHandler *>(h);
  if (handler->ready == false)
  {
    HttpConfigHelper::ServiceUnavailable(conn);
    return 1;
  }
  std::map<std::string, std::string> params;
  std::map<std::string, int64_t> int_params;
  int_params["PendingTask"] = handler->task_scheduler().pending_task_count();
  int_params["DoingTask"]   = handler->task_scheduler().doing_task_count();
  if (int_params["PendingTask"] >= 1024)
  {
    Status status = Status::Busy();
    status.mutable_message() << "task scheduler busy";
    HttpConfigHelper::OKRequest(conn, "set", params, int_params, status);
    return 1;
  }
  params[HttpConfigHelper::kTidName] = "";
  params[HttpConfigHelper::kCidName] = "";
  params[HttpConfigHelper::kKeyName] = "";
  params[HttpConfigHelper::kValName] = "";
  Status status = HttpConfigHelper::GetParameterValue(conn, params);
  if (status.ok())
  {
    int64_t last_version = 0;
    int64_t next_version = 0;
    std::string uuid;
    status = handler->SetConfig(params[HttpConfigHelper::kTidName],
                                params[HttpConfigHelper::kCidName],
                                params[HttpConfigHelper::kKeyName],
                                params[HttpConfigHelper::kValName],
                                &last_version, &next_version, uuid);
    if (status.ok())
    {
      int_params["LastVersion"] = last_version;
      int_params["NextVersion"] = next_version; 
      params["UUID"] = uuid;
    }
    HttpConfigHelper::OKRequest(conn, "set", params, int_params, status);
  }
  else
  {
    HttpConfigHelper::BadRequest(conn, status);
  }
  return 1;
}

}
}
