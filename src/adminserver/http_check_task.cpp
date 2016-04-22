/*=============================================================================
#     FileName: http_check_task.cpp
#         Desc: 
#       Author: yexiang.ych
#        Email: yexiang.ych@taobao.com
#     HomePage: 
#      Version: 0.0.1
#   LastChange: 2014-03-28 15:25:27
#      History:
=============================================================================*/
#include "http_check_task.h"
#include "http_config_helper.h"
#include "administrator_handler.h"

namespace tair
{
namespace admin
{

int HttpCheckTask::DoCommand(void *h, struct mg_connection *conn)
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
  params[HttpConfigHelper::kUUIDName]    = "";
  Status status = HttpConfigHelper::GetParameterValue(conn, params);
  if (status.ok())
  {
    std::string task_status;
    status = handler->GetTaskStatus(params[HttpConfigHelper::kUUIDName], task_status);
    if (status.ok())
    {
      params["TaskStatus"] = task_status;
    }
    
    HttpConfigHelper::OKRequest(conn, "checktask", params, int_params, status);
  }
  else
  {
    HttpConfigHelper::BadRequest(conn, status);
  }
  return 1;
}

}
}
