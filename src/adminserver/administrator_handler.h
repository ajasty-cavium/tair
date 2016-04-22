/*=============================================================================
#     FileName: administrator_handler.h
#         Desc: 
#       Author: yexiang.ych
#        Email: yexiang.ych@taobao.com
#     HomePage: 
#      Version: 0.0.1
#   LastChange: 2014-03-12 19:08:25
#      History:
=============================================================================*/
#ifndef ADMIN_ADMINISTRATOR_HANDLER_H_
#define ADMIN_ADMINISTRATOR_HANDLER_H_

#include <mysql.h>
#include "tair_manager.h"
#include "task_scheduler.h"
#include "administrator_client.h"

namespace tair
{
namespace admin
{

class AdministratorInitializer;
class TimeEventScheduler;

class AdministratorHandler
{
 public:
  AdministratorHandler() : ready(false), mysql_port_(0)
  {
    pthread_mutex_init(&mutex_, NULL);
  }
  virtual ~AdministratorHandler();

  volatile bool   ready;

  Status UpdateDataServers(TairInfo *tair_info, std::vector<easy_addr_t> &addr);

  void GrabTairInfos(std::vector<TairInfo *> &vec);

  Status GetTaskStatus(const std::string& uuid, std::string &task_status);

  Status DelConfig(const std::string &tid, const std::string &cid,
                   const std::string &key,
                   int64_t *last_version, int64_t *next_version,
                   std::string &uuid);

  Status SetConfig(const std::string &tid, const std::string &cid,
                   const std::string &key, const std::string &val,
                   int64_t *last_version, int64_t *next_version,
                   std::string &uuid);

  Status LoadConfiguration(const std::string& tid, TairConfiguration &tair_conf);

  Status AddCluster(const std::string &tid,
                    const std::string &master_addr,
                    const std::string &slave_addr,
                    const std::string &group,
                    const std::string &cid_list);

  Status LoadTairManager();

  TaskScheduler& task_scheduler() { return task_scheduler_; }

  AdministratorClient& client() { return client_; }
 
  TairManager& tair_manager() { return tair_manager_; }

  Status AddOplog(const std::string &uuid, 
                  const std::string &tid, const std::string &cid, 
                  const std::string &key, const std::string &val,
                  int64_t last_version, int64_t next_version,
                  TaskOperation opt);

  Status UpdateOplog(const std::string &uuid, const std::string &status);

  Status GenerateUUID(std::string &uuid);

  void set_mysql(const std::string &host, const std::string &user,
                 const std::string &pasw, const std::string &dbname, int16_t port);

  Status ExecuteSQL(const std::string &sql, MYSQL_RES **res);

  void SetAdminInitializer(AdministratorInitializer *init) { admin_initializer_ = init; }

  void SetTimeEventScheduler(TimeEventScheduler *time_event_scheduler) { time_event_scheduler_ = time_event_scheduler; }

 protected:
  Status DelConfigToMysql(const std::string &uuid, const std::string &tid,
                          const std::string &cid,  const std::string &key);

  Status SetConfigToMysql(const std::string &uuid, const std::string &tid,
                          const std::string &cid,  const std::string &key,
                          const std::string &val);

  Status ConnectMysql(MYSQL **conn);

  Status PopMysqlConn(MYSQL **conn);

  void   PushMysqlConn(MYSQL* conn);

  void SplitCidList(const std::string &tid, char *list, std::vector<TairConfiguration> &configs);

 private:
  TairManager          tair_manager_;
  TaskScheduler        task_scheduler_;
  AdministratorClient  client_;
  std::queue<MYSQL *>  mysql_conn_;
  std::string          mysql_host_;
  std::string          mysql_user_;
  std::string          mysql_pasw_;
  std::string          mysql_dbname_;
  int16_t              mysql_port_;
  pthread_mutex_t      mutex_;
  AdministratorInitializer *admin_initializer_;
  TimeEventScheduler       *time_event_scheduler_;
};

}
}
#endif

