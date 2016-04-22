/*=============================================================================
#     FileName: http_check_config.cpp
#         Desc:
#       Author: yexiang.ych
#        Email: yexiang.ych@taobao.com
#     HomePage:
#      Version: 0.0.1
#   LastChange: 2014-03-20 14:04:20
#      History:
=============================================================================*/
#include "http_check_config.h"
#include "http_config_helper.h"
#include "administrator_handler.h"

namespace tair
{
namespace admin
{

int HttpCheckConfig::DoCommand(void *h, struct mg_connection *conn)
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
    TairConfiguration *configuration = NULL;
    TairInfo          *tair_info = (handler->tair_manager())[params[HttpConfigHelper::kTidName]];
    if (tair_info != NULL)
    {
      configuration = (*tair_info)[params[HttpConfigHelper::kCidName]];
    }
    if (configuration != NULL)
    {
      std::stringstream ss;
      HttpConfigHelper::JsonFormatBegin(ss);
      HttpConfigHelper::JsonFormatString(ss, params);
      ss << "\"TairInfo\":" << tair_info->ToString(params[HttpConfigHelper::kCidName]) << ",";
      ss << "\"ConfigurationVersions\":{";
      std::vector<easy_addr_t> addrs;
      tair_info->GrabAddresses(addrs);
      for (size_t i = 0; i < addrs.size(); ++i)
      {
        int64_t version = handler->client().NotifyConfiguration(
                                              addrs[i], configuration->cid(),
                                              AdministratorClient::kEmpty, 0, 0, kCheck, 200);
        if (version == -1)
        {
          status = Status::TairError();
        }
        if (i != 0) ss << ",";
        ss << "\"";
        tair::rpc::RpcContext::Cast2String(addrs[i], ss);
        ss << "\":" << version;
      }
      ss << "},";
      HttpConfigHelper::JsonFormatEnd(ss, "checkconf", status);
      HttpConfigHelper::OKRequest(conn, ss.str());
    }
    else
    {
      status = Status::NotFound();
      status.mutable_message() << "tid or cid not found";
      std::map<std::string, int64_t> int_params;
      HttpConfigHelper::OKRequest(conn, "checkconf", params, int_params, status);
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

