#ifndef TAIR_ADMIN_SERVER_LIST_MANAGER_H
#define TAIR_ADMIN_SERVER_LIST_MANAGER_H
#include <pthread.h>
#include <tbsys.h>
#include <easy_io.h>
#include <easy_io_struct.h>
#include "packet_factory.hpp"
#include <vector>
#include <set>
#include <string>
#include "query_version_packet.hpp"
#include "area_map.hpp"
namespace tair
{
namespace admin
{
class AreaMapManager;
class ServerListManager : public tbsys::CDefaultRunnable
{
public:
  ServerListManager();
  ~ServerListManager();

  void SetAreaMapManager(AreaMapManager* area_map_manager);
  void SetEIOInstance(easy_io_t* eio);

  bool Initialize();

  bool Start();

  void Stop();

  void run(tbsys::CThread *thread, void *arg);
private:
  //eio
  easy_io_t* eio_ptr_;
  easy_io_handler_pt eio_handler_;

  //config server
  std::vector<uint64_t> configserver_list_;

  //area map manager
  AreaMapManager *area_map_manager_;
  request_query_version query_version_packet;
  std::string group_name_;

  //server list
  easy_atomic_t server_list_lock_;
  atomic_t server_list_version_;
  std::set<uint64_t> server_list_;
  Beacon version_changed_beacon_;
private:
  void AsyncQueryVersion();
  void PullServerList();
  void CollectServer(const std::map<std::string, std::string>& kvmap);
  int HandleRequest(easy_request_t *r);

  static int EasyIOHandlerProcess(easy_request_t *r)
  {
    ServerListManager *inst = static_cast<ServerListManager *>(r->ms->c->handler->user_data);
    return inst->HandleRequest(r);
  }
};

} // end of namespace tair
} // end of namespace admin
#endif
