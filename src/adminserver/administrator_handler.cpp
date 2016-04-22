#include "administrator_handler.h"
#include "administrator_client.h"
#include "administrator_initializer.h"
#include "time_event_scheduler.h"
#include <pthread.h>

namespace tair
{
namespace admin
{

void ToTaskOperationString(TaskOperation opt, std::stringstream &ss)
{
  switch (opt)
  {
   case kSet:
    ss << "SET";
    break;
   case kDel:
    ss << "DEL";
    break;
   case kCheck:
    ss << "CHECK";
    break;
   case kSync:
    ss << "SYNC";
    break;
   default:
    ss << "UNKNOW";
    break;
  }
  return ;
}

AdministratorHandler::~AdministratorHandler()
{
  while (mysql_conn_.empty() == false)
  {
    MYSQL *conn = mysql_conn_.front();
    mysql_conn_.pop();
    mysql_close(conn);
  }
  mysql_library_end();
  pthread_mutex_destroy(&mutex_);
}

Status AdministratorHandler::PopMysqlConn(MYSQL **conn)
{
  *conn = NULL;
  pthread_mutex_lock(&mutex_);
  if (mysql_conn_.empty() == false)
  {
    *conn = mysql_conn_.front();
    mysql_conn_.pop();
  }
  pthread_mutex_unlock(&mutex_);
  return *conn != NULL ? Status::OK() : ConnectMysql(conn);
}
void AdministratorHandler::PushMysqlConn(MYSQL* conn)
{
  assert(conn != NULL);
  pthread_mutex_lock(&mutex_);
  if (mysql_conn_.size() < 16)
  {
    mysql_conn_.push(conn);
    conn = NULL;
  }
  pthread_mutex_unlock(&mutex_);
  if (conn != NULL)
  {
    mysql_close(conn);
  }
}

void AdministratorHandler::set_mysql(const std::string &host,
                                     const std::string &user,
                                     const std::string &pasw,
                                     const std::string &dbname, int16_t port)
{
  this->mysql_host_ = host;
  this->mysql_user_ = user;
  this->mysql_pasw_ = pasw;
  this->mysql_port_ = port;
  this->mysql_dbname_ = dbname;
}

Status AdministratorHandler::ConnectMysql(MYSQL **conn)
{
  *conn = mysql_init(NULL);
  if (mysql_real_connect(*conn,
                         this->mysql_host_.c_str(),
                         this->mysql_user_.c_str(),
                         this->mysql_pasw_.c_str(),
                         this->mysql_dbname_.c_str(),
                         this->mysql_port_, 0,0) == NULL)
  {
    Status status = Status::MysqlError();
    status.mutable_message() << "connect error:" << mysql_error(*conn);
    mysql_close(*conn);
    *conn = NULL;
    return status;
  }
  char value = 1;
  mysql_options(*conn, MYSQL_OPT_RECONNECT, (char *)&value);
  return Status::OK();
}

Status AdministratorHandler::ExecuteSQL(const std::string &sql, MYSQL_RES **res)
{
  MYSQL *conn = NULL;
  Status status = PopMysqlConn(&conn);
  if (status.ok())
  {
    if (mysql_query(conn, sql.c_str()) != 0)
    {
      TBSYS_LOG(ERROR, "mysql error sql:%s error:%s", sql.c_str(), mysql_error(conn));
      status = Status::MysqlError();
      status.mutable_message() << "sql:" << sql << " error:" << mysql_error(conn);
    }
    else if (res != NULL)
    {
      *res = mysql_store_result(conn);
      if (*res == NULL)
        status = Status::NotFound();
    }
    PushMysqlConn(conn);
  }
  return status;
}

Status AdministratorHandler::GetTaskStatus(const std::string& uuid, std::string &task_status)
{
  const std::string sql = "SELECT `op_status` from `oplog` WHERE `op_uuid`='" + uuid + "';";
  MYSQL_RES *result = NULL;
  Status status = ExecuteSQL(sql, &result);
  if (status.ok())
  {
    MYSQL_ROW row = NULL;
    int n = mysql_num_rows(result);
    if (n != 1)
    {
      status = Status::NotFound();
      status.mutable_message() << "found too much or not found tasks, task number:" << n;
    }
    else if ((row = mysql_fetch_row(result)) != NULL)
    {
      task_status = row[0];
    }
    mysql_free_result(result);
  }
  return status;
}

Status AdministratorHandler::LoadConfiguration(const std::string& tid, TairConfiguration &tair_conf)
{
  const std::string sql = "SELECT `conf_key`, `conf_val` FROM `configuration` WHERE `deleted`=0 AND "
                          "`tair_id`='" + tid + "' AND `conf_id`='" + tair_conf.cid() + "';";
  MYSQL_RES *result = NULL;
  Status status = ExecuteSQL(sql, &result);
  if (status.ok())
  {
    std::map<std::string, std::string> kvs;
    MYSQL_ROW row = NULL;
    while ((row = mysql_fetch_row(result)) != NULL)
    {
      std::string key(row[0]);
      std::string val(row[1]);
      kvs[key] = val;
      TBSYS_LOG(INFO, "%s Tid:%s Cid:%s Key:%s Val:%s", __FUNCTION__,
          tid.c_str(), tair_conf.cid().c_str(), key.c_str(), val.c_str());
    }
    int64_t last_version = 0;
    int64_t next_version = 0;
    tair_conf.BatchSet(kvs, &last_version, &next_version);
    mysql_free_result(result);
  }
  else if (status.IsNotFound())
    status = Status::OK();
  return status;
}

void AdministratorHandler::SplitCidList(const std::string &tid, char *list, std::vector<TairConfiguration> &configs)
{
  Status status        = Status::OK();
  char *saveptr = NULL;
  for(char *p = strtok_r(list, " ,", &saveptr); p != NULL; p = strtok_r(NULL, " , ", &saveptr))
  {
    std::string cid(p);
    TairConfiguration tair_conf;
    tair_conf.set_cid(cid);
    status = LoadConfiguration(tid, tair_conf);
    if (status.ok())
    {
      configs.push_back(tair_conf);
    }
    else
    {
      TBSYS_LOG(ERROR, "Load Tair Configuration %s Failed error:%s", cid.c_str(), status.message().c_str());
    }
  }
}

Status AdministratorHandler::AddCluster(const std::string &tid,
                                        const std::string &master_addr,
                                        const std::string &slave_addr,
                                        const std::string &group,
                                        const std::string &cid_list)
{
  Status status        = Status::OK();
  easy_addr_t master   = easy_inet_str_to_addr(master_addr.c_str(), 0);
  easy_addr_t slave    = easy_inet_str_to_addr(slave_addr.c_str(), 0);
  std::vector<TairConfiguration> configs;
  SplitCidList(tid, const_cast<char *>(cid_list.c_str()), configs);

  std::vector<easy_addr_t> addrs;
  TairInfo ti;
  ti.set_config_server_addr(master, slave, group);
  ti.set_tid(tid);
  status = UpdateDataServers(&ti, addrs);// Update version and addrs
  if (!status.ok())
    return status;
  tair_manager_.RegisterTairInfo(tid, configs, ti);

  TBSYS_LOG(INFO, "Initialized TairInfo:%s", ti.ToString().c_str());
  admin_initializer_->MaybeSyncConfiguration(client_, &ti);

  time_event_scheduler_->PushEvent(tair_manager_[tid], time(NULL) + 5);// Get tair_info address from global tairclusters_
  return status;
}

Status AdministratorHandler::LoadTairManager()
{
  const static std::string kSelectTairsSQL = "SELECT `tair_id`, `master`, `slave`, `groupname`, `cid_list` FROM `cluster` WHERE `online`=true";
  MYSQL_RES *result = NULL;
  Status status = ExecuteSQL(kSelectTairsSQL, &result);
  if (status.ok())
  {
    MYSQL_ROW row = NULL;
    while ((row = mysql_fetch_row(result)) != NULL && status.ok())
    {
      std::string tid(row[0]);
      easy_addr_t master = easy_inet_str_to_addr(row[1], 0);
      easy_addr_t slave  = easy_inet_str_to_addr(row[2], 0);
      std::string group(row[3]);
      std::vector<TairConfiguration> configs;
      SplitCidList(tid, row[4], configs);
      tair_manager_.RegisterTairInfo(tid, configs, master, slave, group);
    }
    mysql_free_result(result);
  }
  return status;
}


Status AdministratorHandler::DelConfig(const std::string &tid, const std::string &cid,
                                       const std::string &key,
                                       int64_t *last_version, int64_t *next_version, std::string &uuid)
{
  assert(last_version != NULL);
  assert(next_version != NULL);
  Status status = GenerateUUID(uuid);
  if (status.ok() == false)
    return status;

  TairInfo *tair_info = tair_manager_[tid];
  if (tair_info == NULL)
  {
    status = Status::NotFound();
    status.mutable_message() << "not found tid:" << tid;
    return status;
  }
  TairConfiguration *tair_conf = (*tair_info)[cid];
  if (tair_conf == NULL)
  {
    status = Status::NotFound();
    status.mutable_message() << "not found cid:" << cid;
    return status;
  }

  std::map<std::string, std::string> kvs;
  kvs[key] = "";
  std::vector<easy_addr_t> addrs;
  tair_info->GrabAddresses(addrs);

  tair_conf->Lock();
  status = DelConfigToMysql(uuid, tid, cid, key);
  bool pushed = false;
  if (status.ok())
  {
    tair_conf->Del(key, last_version, next_version);
    if (*last_version == *next_version)
    {
      status = Status::NotChanged();
      status.mutable_message() << "configuration is modified by del but not changed";
      TBSYS_LOG(INFO, "modified but not changed Tid:%s Cid:%s Key:%s Version:%ld",
                      tid.c_str(), cid.c_str(), key.c_str(), *last_version);
    }
    else if (addrs.size() == 0)
    {
      status.mutable_message() << "warn no alive server found";
    }
    else
    {
      pushed = true;
      task_scheduler_.PushTask(uuid, cid, kvs, *last_version, *next_version, addrs, kDel);
    }
  }
  tair_conf->Unlock();
  if (pushed && status.ok())
  {
    status = AddOplog(uuid, tid, cid,  key, "", *last_version, *next_version, kDel);
  }
  return status;
}

Status AdministratorHandler::SetConfig(const std::string &tid, const std::string &cid,
                                       const std::string &key, const std::string &val,
                                       int64_t *last_version, int64_t *next_version, std::string &uuid)
{
  assert(last_version != NULL);
  assert(next_version != NULL);
  Status status = GenerateUUID(uuid);
  if (status.ok() == false)
    return status;

  TairInfo *tair_info = tair_manager_[tid];
  if (tair_info == NULL)
  {
    status = Status::NotFound();
    status.mutable_message() << "not found tid:" << tid;
    return status;
  }
  TairConfiguration *tair_conf = (*tair_info)[cid];
  if (tair_conf == NULL)
  {
    status = Status::NotFound();
    status.mutable_message() << "not found cid:" << cid;
    return status;
  }

  std::map<std::string, std::string> kvs;
  kvs[key] = val;
  std::vector<easy_addr_t> addrs;
  tair_info->GrabAddresses(addrs);

  tair_conf->Lock();
  status = SetConfigToMysql(uuid, tid, cid, key, val);
  bool pushed = false;
  if (status.ok())
  {
    tair_conf->Set(key, val, last_version, next_version);
    if (*last_version == *next_version)
    {
      status = Status::NotChanged();
      status.mutable_message() << "configuration is modified by set but not changed";
      TBSYS_LOG(INFO, "modified but not changed Tid:%s Cid:%s Key:%s Val:%s Version:%ld",
                      tid.c_str(), cid.c_str(), key.c_str(), val.c_str(), *last_version);
    }
    else if (addrs.size() == 0)
    {
      status.mutable_message() << "warn no alive server found";
    }
    else
    {
      pushed = true;
      task_scheduler_.PushTask(uuid, cid, kvs, *last_version, *next_version, addrs, kSet);
    }
  }
  tair_conf->Unlock();
  if (pushed && status.ok())
  {
    status = AddOplog(uuid, tid, cid,  key, val, *last_version, *next_version, kSet);
  }
  return status;
}

Status AdministratorHandler::AddOplog(const std::string &uuid,
                                      const std::string &tid, const std::string &cid,
                                      const std::string &key, const std::string &val,
                                      int64_t last_version, int64_t next_version,
                                      TaskOperation opt)
{
  std::stringstream sql_stream;
  sql_stream << "INSERT INTO oplog(`op_uuid`, `tair_id`, `conf_id`, `conf_key`, `conf_val`, "
                "`last_version`, `next_version`, `op_type`, `op_status`) VALUES(";
  sql_stream << "'"
             << uuid << "', '"
             << tid  << "', '"
             << cid  << "', '"
             << key  << "', '"
             << val  << "', '"
             << last_version << "', '"
             << next_version << "', '";
  ToTaskOperationString(opt, sql_stream);
  sql_stream << "', 'SYNCING')";
  return ExecuteSQL(sql_stream.str(), NULL);
}

Status AdministratorHandler::UpdateOplog(const std::string &uuid, const std::string &status)
{
  std::string sql = "UPDATE `oplog` SET `op_status`='" + status + "' WHERE `op_uuid`='" + uuid + "';";
  return ExecuteSQL(sql, NULL);
}

Status AdministratorHandler::DelConfigToMysql(const std::string &uuid,
                                              const std::string &tid,
                                              const std::string &cid,
                                              const std::string &key)
{
  std::string sql = "INSERT INTO configuration(`tair_id`, `conf_id`, `conf_key`, `conf_val`, `op_uuid`) "
                    "VALUES('" + tid + "', '" + cid + "', '" + key + "', '', '" + uuid + "')"
                    "ON DUPLICATE KEY UPDATE `deleted`=1;";
  return ExecuteSQL(sql, NULL);
}

Status AdministratorHandler::SetConfigToMysql(const std::string &uuid,
                                              const std::string &tid, const std::string &cid,
                                              const std::string &key, const std::string &val)
{
  std::string sql = "INSERT INTO configuration(`tair_id`, `conf_id`, `conf_key`, `conf_val`, `op_uuid`) "
                    "VALUES('" + tid + "', '" + cid + "', '" + key + "', '" + val + "', '" + uuid + "')"
                    "ON DUPLICATE KEY UPDATE `deleted`=0, `conf_val` = '" + val + "';";
  return ExecuteSQL(sql, NULL);
}

void AdministratorHandler::GrabTairInfos(std::vector<TairInfo *> &vec)
{
  TairClusterIterator iter      = tair_manager_.begin();
  TairClusterIterator iter_end  = tair_manager_.end();
  for (; iter != iter_end; ++iter)
  {
    vec.push_back(&(iter->second));
  }
  return ;
}

Status AdministratorHandler::UpdateDataServers(TairInfo *tair_info, std::vector<easy_addr_t> &addr)
{
  Status status = Status::OK();
  int64_t version = 0;
  version = client_.GrabDataServers(tair_info->master_address(),
      tair_info->group_name(), addr, 500);
  if (version == -1)
  {
    addr.clear();
    version = client_.GrabDataServers(tair_info->slave_address(),
        tair_info->group_name(), addr, 500);
  }
  if (version == -1)
  {
    TBSYS_LOG(ERROR, "Update DataServers Failed TairInfo:%s",
        tair_info->ToAddressString().c_str());
    status = Status::TairError();
  }
  else if (tair_info->MaybeDataServerChanged(version, addr) == false)
  {
    status = Status::NotFound();
  }
  return status;
}

Status AdministratorHandler::GenerateUUID(std::string &uuid)
{
  const static std::string sql = "SELECT HEX(uuid_short());";
  MYSQL_RES *result = NULL;
  Status status = ExecuteSQL(sql, &result);
  if (status.ok())
  {
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row != NULL)
    {
      uuid = row[0];
    }
    else
    {
      status = Status::MysqlError();
      status.mutable_message() << "no uuid generate";
    }
    mysql_free_result(result);
  }
  return status;
}


}
}

