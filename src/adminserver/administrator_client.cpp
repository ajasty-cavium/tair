#include "administrator_client.h"
#include "tair_rpc.h"
#include "notify_configuration_command.h"
#include "grab_group_command.h"

namespace tair
{
namespace admin
{

const std::map<std::string, std::string> AdministratorClient::kEmpty;

AdministratorClient::AdministratorClient()
    :context_(NULL)
{
}

AdministratorClient::~AdministratorClient()
{
}

int64_t AdministratorClient::GrabDataServers(const easy_addr_t &cs_addr,
                                             const std::string &group_name,
                                             std::vector<easy_addr_t> &addrs,
                                             int64_t timeout_ms)
{
  int64_t version = -1;
  GrabGroupRequest *request = new GrabGroupRequest();
  request->set_group_name(group_name);
  tair::rpc::RpcPacket *packet = 
    context_->NewTairRpcPacket(GrabGroupCommand::GetRequestID(), request);
  tair::rpc::RpcFuture *future = 
    context_->Call(cs_addr, packet, timeout_ms, NULL, NULL);
  future->Wait();
  const GrabGroupResponse *response = 
    dynamic_cast<const GrabGroupResponse *>(future->tair_response());
  if (response != NULL)
  {
    version   = response->version();
    addrs     = response->addrs(); 
  }
  else
  {
    char buffer[64];
    bzero(buffer, sizeof(buffer));
    easy_inet_addr_to_str(const_cast<easy_addr_t *>(&cs_addr), buffer, sizeof(buffer));
    TBSYS_LOG(WARN, "%s Failed address:%s group:%s exception:%d", 
              __FUNCTION__, buffer, 
              group_name.c_str(), future->exception());
  }
  future->UnRef();
  return version;
}

int64_t AdministratorClient::NotifyConfiguration(const easy_addr_t &addr, 
                                                 const std::string &cid, 
                                                 const std::map<std::string, std::string> &configs,
                                                 int64_t last_version, int64_t next_version, 
                                                 TaskOperation opt, int64_t timeout_ms)
{
  NotifyConfigurationRequest *request = new NotifyConfigurationRequest();
  request->set_cid(cid);
  request->set_version(last_version, next_version);
  request->set_opt(opt);
  request->set_configs(configs);

  tair::rpc::RpcPacket *packet = 
    context_->NewTairRpcPacket(NotifyConfigurationCommand::GetRequestID(), request);
  tair::rpc::RpcFuture *future = 
    context_->Call(addr, packet, timeout_ms, NULL, NULL);
  future->Wait();
  const NotifyConfigurationResponse *response =
    dynamic_cast<const NotifyConfigurationResponse *>(future->tair_response());
  int64_t current_version = -1;
  if (response != NULL)
  {
    current_version = response->current_version();
  }
  else
  {
    char buffer[64];
    bzero(buffer, sizeof(buffer));
    easy_inet_addr_to_str(const_cast<easy_addr_t *>(&addr), buffer, sizeof(buffer));
    TBSYS_LOG(WARN, "%s Failed address:%s cid:%s exception:%d", 
              __FUNCTION__, buffer, cid.c_str(), future->exception());
  }
  future->UnRef();
  return current_version;
}
int64_t AdministratorClient::GrabConfiguration(const easy_addr_t &addr, 
                                               const std::string &cid, 
                                               std::map<std::string, std::string> &configs,
                                               int64_t timeout_ms)
{
  NotifyConfigurationRequest *request = new NotifyConfigurationRequest();
  request->set_cid(cid);
  request->set_version(0, 0);
  request->set_opt(kGet);
  request->set_configs(configs);

  tair::rpc::RpcPacket *packet = 
    context_->NewTairRpcPacket(NotifyConfigurationCommand::GetRequestID(), request);
  tair::rpc::RpcFuture *future = 
    context_->Call(addr, packet, timeout_ms, NULL, NULL);
  future->Wait();
  const NotifyConfigurationResponse *response =
    dynamic_cast<const NotifyConfigurationResponse *>(future->tair_response());
  int64_t current_version = -1;
  if (response != NULL)
  {
    current_version = response->current_version();
    configs         = response->configs();
  }
  else
  {
    char buffer[64];
    bzero(buffer, sizeof(buffer));
    easy_inet_addr_to_str(const_cast<easy_addr_t *>(&addr), buffer, sizeof(buffer));
    TBSYS_LOG(WARN, "%s Failed address:%s cid:%s exception:%d", 
              __FUNCTION__, buffer, cid.c_str(), future->exception());
  }
  future->UnRef();
  return current_version;
}

}
}

