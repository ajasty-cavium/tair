#include "server_list_manager.hpp"
#include "area_map_manager.hpp"
#include "query_info_packet.hpp"
#include <algorithm>
namespace tair
{
namespace admin
{
ServerListManager::ServerListManager()
{
  version_changed_beacon_.TurnOn();
  atomic_set(&server_list_version_, 0);
  server_list_lock_ = 0;
}

ServerListManager::~ServerListManager()
{
}

void ServerListManager::SetAreaMapManager(AreaMapManager* area_map_manager)
{
  area_map_manager_ = area_map_manager;
}

void ServerListManager::SetEIOInstance(easy_io_t* eio)
{
  eio_ptr_ = eio;
}

bool ServerListManager::Initialize()
{
  bool ret = load_config_server_list("ServerListManager", configserver_list_);
  const char* group_name_cstr = TBSYS_CONFIG.getString(TAIRPUBLIC_SECTION, ADMINSERVER_GROUP_NAME, NULL);
  if (group_name_cstr == NULL)
  {
    log_error("miss group name");
    return false;
  }
  group_name_ = std::string(group_name_cstr);
  if (ret)
  {
    query_version_packet.server_id_ = local_server_ip::ip;
    strcpy(query_version_packet.group_name_, group_name_.c_str());
    memset(&eio_handler_, 0, sizeof(eio_handler_));
    easy_helper::init_handler(&eio_handler_, this);
    eio_handler_.process = ServerListManager::EasyIOHandlerProcess;
  }
  return ret;
}
bool ServerListManager::Start()
{
  log_info("ServerListManager Start.");
  start();
  return true;
}

void ServerListManager::Stop()
{
  log_info("ServerListManager Stop.");
  stop();
}

void ServerListManager::PullServerList()
{
  //pull server list from configserver, using sync RPC.
  for (uint32_t i = 0; i < configserver_list_.size(); ++i)
  {
    if ((configserver_list_[i] & TAIR_FLAG_CFG_DOWN)) continue;
    request_query_info *req = new request_query_info();
    req->query_type = request_query_info::Q_DATA_SERVER_INFO;
    req->server_id = local_server_ip::ip;
    req->group_name = group_name_;
    if (easy_helper::easy_async_send(eio_ptr_, configserver_list_[i],
          req, (void*)((long)i), &eio_handler_) == EASY_ERROR)
    {
      log_error("pull server list from %s, group: %s failed.",
          tbsys::CNetUtil::addrToString(configserver_list_[i]).c_str(), group_name_.c_str());
      delete req;
    }
    else
    {
      log_info("pull server list from %s, group: %s success.",
          tbsys::CNetUtil::addrToString(configserver_list_[i]).c_str(), group_name_.c_str());
    }
  }
}
void ServerListManager::AsyncQueryVersion()
{
  //query version
  for (uint32_t i = 0; i < configserver_list_.size(); ++i)
  {
    if ((configserver_list_[i] & TAIR_FLAG_CFG_DOWN)) continue;
    request_query_version *req = new request_query_version(query_version_packet);
    if (easy_helper::easy_async_send(eio_ptr_, configserver_list_[i],
          req, (void*)((long)i), &eio_handler_) == EASY_ERROR)
    {
    log_error("query version from %s, group: %s failed.",
        tbsys::CNetUtil::addrToString(configserver_list_[i]).c_str(), group_name_.c_str());
      delete req;
    }
    log_debug("query version from %s, group: %s sucess.",
        tbsys::CNetUtil::addrToString(configserver_list_[i]).c_str(), group_name_.c_str());
  }
}
void ServerListManager::run(tbsys::CThread *thread, void *arg)
{
  easy_helper::set_thread_name("ServerListManager");
  //periodic work
  while (!_stop)
  {
    //periodic check version
    AsyncQueryVersion();

    //if version changed, pull the server list, and update `AreaManager's server list.
    //in the `Async RPC's Callback, we TURN OFF the `version_changed_beacon_, if we 
    //has got the yong server list. otherwise, we retry to query server list.
    if (version_changed_beacon_.IsOn())
    {
      PullServerList();
    }

    TAIR_SLEEP(_stop, 2);
  };
}

//invoked by `PullServerList
void ServerListManager::CollectServer(const std::map<std::string, std::string>& kvmap)
{
  //parse the server id
  std::set<uint64_t> server_list;
  for (std::map<std::string, std::string>::const_iterator it = kvmap.begin(); it != kvmap.end(); ++it)
  {
    const std::string& addr_str = it->first;
    const std::string& status = it->second;
    log_info("got server map from config server, server: %s , status: %s", addr_str.c_str(), status.c_str());
    if (status == "alive")
    {
      uint64_t id = tbsys::CNetUtil::strToAddr(addr_str.c_str(), 0);
      if (id != 0)
      {
        server_list.insert(id);
      }
      else
      {
        log_error("connvert string: %s to ip:port failed", addr_str.c_str());
      }
    }
  }
  //update the `AreaMapManager's server list
  easy_spin_lock(&server_list_lock_);
  {
    server_list_ = server_list;
    area_map_manager_->UpdateServerList(server_list_);
  }
  easy_spin_unlock(&server_list_lock_);
}

int ServerListManager::HandleRequest(easy_request_t *r)
{
  base_packet *packet = (base_packet*)r->ipacket;
  if (packet == NULL)
  {
    log_error("ServerListManager connect to %s timeout", easy_helper::easy_connection_to_str(r->ms->c).c_str());
    easy_session_destroy(r->ms);
    return EASY_ERROR; //~ destroy this connection
  }

  if (_stop)
  {
    log_warn("thread is stop, but receive packet. pcode :%d", packet->getPCode());
  }
  else
  {
    int pcode = packet->getPCode();
    int server_index = (int)((long)r->args);
    if (server_index == 1 && configserver_list_.size() == 2U && (configserver_list_[0] & TAIR_FLAG_CFG_DOWN) == 0)
    {
      //drop the response;
      easy_session_destroy(r->ms);
      return EASY_OK;
    }

    if (pcode == TAIR_RESP_QUERY_VERSION_PACKET)
    {
      response_query_version* resp = dynamic_cast<response_query_version*>(packet);
      log_debug("admin server receives query version response from: [%d, %s], current version: %d, version: %d",
          server_index, easy_helper::easy_connection_to_str(r->ms->c).c_str(),
          atomic_read(&server_list_version_), resp->version_);

      if (atomic_read(&server_list_version_) != resp->version_)
      {
        log_info("admin server receives query version response from: [%d, %s], version: [%d, %d], version changed.",
            server_index, easy_helper::easy_connection_to_str(r->ms->c).c_str(),
            atomic_read(&server_list_version_), resp->version_);
        //the version of the server list was changed.
        //TURN ON the `version_changed_beacon_;
        atomic_set(&server_list_version_, resp->version_);
        version_changed_beacon_.TurnOn();
      }
    }
    else if (pcode == TAIR_RESP_QUERY_INFO_PACKET)
    {

      response_query_info* resp = dynamic_cast<response_query_info*>(packet);
      if (resp != NULL)
      {
        if (version_changed_beacon_.IsOn())
        {
          log_info("admin server receives query server list response from: [%d, %s], current version: %d",
              server_index, easy_helper::easy_connection_to_str(r->ms->c).c_str(), atomic_read(&server_list_version_));
          //TURN OFF the `version_changed_beacon_.
          version_changed_beacon_.TurnOff();
          std::map<std::string, std::string>& kvmap = resp->map_k_v;
          CollectServer(kvmap);
        }
      }
      else
      {
        log_error("admin server receives query server list response from: [%d, %s], current version: %d, failed.",
            server_index, easy_helper::easy_connection_to_str(r->ms->c).c_str(), atomic_read(&server_list_version_));
      }
    }
    else
    {
      log_warn("admin server receives the unknown packet from: %s, pcode: %d",
          easy_helper::easy_connection_to_str(r->ms->c).c_str(), pcode);
    }
  }
  easy_session_destroy(r->ms);
  return EASY_OK;
}

}
}
