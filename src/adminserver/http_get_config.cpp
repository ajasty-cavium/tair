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
#include "http_get_config.h"
#include "http_config_helper.h"
#include "administrator_handler.h"

namespace tair
{
namespace admin
{

int HttpGetConfig::DoCommand(void *h, struct mg_connection *conn)
{
  AdministratorHandler *handler = reinterpret_cast<AdministratorHandler *>(h);
  if (handler->ready == false)
  {
    HttpConfigHelper::ServiceUnavailable(conn);
    return 1;
  }
  std::map<std::string, std::string> params;
  std::map<std::string, int64_t> int_params;
  params[HttpConfigHelper::kTidName] = "";
  params[HttpConfigHelper::kCidName] = "";
  params[HttpConfigHelper::kKeyName] = "";
  Status status = HttpConfigHelper::GetParameterValue(conn, params);
  if (status.ok())
  {
    int64_t       curt_version = 0;
    std::string   value;
    TairConfiguration *configuration = NULL;
    TairInfo          *tair_info = (handler->tair_manager())[params[HttpConfigHelper::kTidName]];
    if (tair_info != NULL)
    {
      configuration = (*tair_info)[params[HttpConfigHelper::kCidName]];
    }
    if (configuration != NULL)
    {
      std::string   &keyname = params[HttpConfigHelper::kKeyName];
      configuration->Lock();
      status = configuration->Get(keyname, value, curt_version);
      if (status.ok())
      {
        params[HttpConfigHelper::kValName] = value;
        int_params["CurtVersion"] = curt_version;
      }
      configuration->Unlock();
      std::stringstream ss;
      HttpConfigHelper::JsonFormatBegin(ss);
      HttpConfigHelper::JsonFormatString(ss, params);
      HttpConfigHelper::JsonFormatInt(ss, int_params);
      ss << "\"ServerConfigurations\":[";
      std::vector<easy_addr_t> addrs;
      tair_info->GrabAddresses(addrs);
      for (size_t i = 0; i < addrs.size(); ++i)
      {
        std::map<std::string, std::string> configs;
        configs[keyname] = "";
        int64_t version = handler->client().GrabConfiguration(addrs[i], configuration->cid(),
                                                              configs, 200);
        if (version == -1)
          status = Status::TairError();
        if (i != 0) ss << ",";
        ss << "{\"IP\":\"";
        tair::rpc::RpcContext::Cast2String(addrs[i], ss);
        ss << "\",";
        ss << "\"" << HttpConfigHelper::kValName << "\":\"" << configs[keyname] << "\",";
        ss << "\"ConfigurationVersion\":" << version << "}";
      }
      ss << "],";
      HttpConfigHelper::JsonFormatEnd(ss, "get", status);
      HttpConfigHelper::OKRequest(conn, ss.str());
    }
    else
    {
      status = Status::NotFound();
      status.mutable_message() << "tid or cid not found";
      HttpConfigHelper::OKRequest(conn, "get", params, int_params, status);
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
