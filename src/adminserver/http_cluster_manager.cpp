/*=============================================================================
#     FileName: http_get_config.cpp
#         Desc:
#       Author: yexiang.ych
#        Email: yexiang.ych@taobao.com
#     HomePage:
#      Version: 0.0.1
#   LastChange: 2014-03-12 16:54:01
#      History:
=============================================================================*/
#include "http_cluster_manager.h"
#include "http_config_helper.h"
#include "administrator_handler.h"

namespace tair
{
namespace admin
{

int HttpAddCluster::DoCommand(void *h, struct mg_connection *conn)
{
  AdministratorHandler *handler = reinterpret_cast<AdministratorHandler *>(h);
  if (handler->ready == false)
  {
    HttpConfigHelper::ServiceUnavailable(conn);
    return 1;
  }
  std::map<std::string, std::string> params;
  std::map<std::string, int64_t> int_params;
  params[HttpConfigHelper::kTidName]    = "";
  params[HttpConfigHelper::kCidName]    = "";
  params[HttpConfigHelper::kMasterName] = "";
  params[HttpConfigHelper::kSlaveName]  = "";
  params[HttpConfigHelper::kGroupName]  = "";
  Status status = HttpConfigHelper::GetParameterValue(conn, params);
  if (!status.ok())
  {
    HttpConfigHelper::BadRequest(conn, status);
    return 1;
  }
  std::string sql = "INSERT INTO `cluster`(`tair_id`, `master`, `slave`, `groupname`, `cid_list`, `online`) VALUES"
      "('" + params[HttpConfigHelper::kTidName] + "', '" + params[HttpConfigHelper::kMasterName] +
      "', '" + params[HttpConfigHelper::kSlaveName] + "', '" + params[HttpConfigHelper::kGroupName] +
      "', '" + params[HttpConfigHelper::kCidName] + "', 1)";
  TBSYS_LOG(DEBUG, "insert sql:%s", sql.c_str());
  status = handler->ExecuteSQL(sql, NULL);
  if (!status.ok())
  {
    HttpConfigHelper::OKRequest(conn, "Add", params, int_params, status);
    return 1;
  }
  status = handler->AddCluster(params[HttpConfigHelper::kTidName],
                               params[HttpConfigHelper::kMasterName],
                               params[HttpConfigHelper::kSlaveName],
                               params[HttpConfigHelper::kGroupName],
                               params[HttpConfigHelper::kCidName]);
  if (status.ok())
  {
    HttpConfigHelper::OKRequest(conn, "{\"Message\": \"Add Cluster success!\"}");
  }
  else
    HttpConfigHelper::OKRequest(conn, "Add Cluster error!");
  return 1;
}

}
}
