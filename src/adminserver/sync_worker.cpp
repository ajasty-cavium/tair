#include <sys/prctl.h>
#include <iostream>
#include "sync_worker.h"

namespace tair
{
namespace admin
{

SyncWorker::SyncWorker() 
    : handler_(NULL)
{
}

void SyncWorker::Stop()
{
  handler_->task_scheduler().Interrupt();
  tair::rpc::RpcThread<void *>::Stop();
}

/*int64_t MaybeSyncConfiguration(TaskRequest *task_request, AdministratorClient *client)
{
  int64_t version = client->NotifyConfiguration(task_request->addr, 
                                                task_request->message->cid, 
                                                AdministratorClient::kEmpty, 
                                                0, 0, 
                                                kCheck, 200);
  if (version != -1 && version != task_request->message->last_version)
  {
    TBSYS_LOG(INFO, "Version not equal, sync all configurations TairVersion:%ld, AdminVersion:%ld",
              version, task_request->message->last_version);
    version = client->NotifyConfiguration(task_request->addr,
                                          task_request->message->cid,
                                          task_request->message->configs,
                                          0, 
                                          task_request->message->next_version,
                                          kSync, 2000);
  }
  return version;
}*/

void SyncWorker::Run(void *&arg, int32_t index)
{
  while (IsStop() == false)
  {
    TaskRequest *task_request = handler_->task_scheduler().PopTaskRequest();
    if (task_request == NULL) break;
    if (TBSYS_LOGGER._level >= TBSYS_LOG_LEVEL_INFO)
    {
      std::stringstream ss;
      tair::rpc::RpcContext::Cast2String(task_request->addr, ss);
      TBSYS_LOG(INFO, "Do worker request Address:%s Operation:%d CID:%s", ss.str().c_str(),
                task_request->message->opt, task_request->message->cid.c_str());
    }
    int64_t version = handler_->client().NotifyConfiguration(task_request->addr, 
                                                   task_request->message->cid, 
                                                   task_request->message->opt == kCheck ? 
                                                       AdministratorClient::kEmpty : task_request->message->configs, 
                                                   task_request->message->last_version,
                                                   task_request->message->next_version,
                                                   task_request->message->opt, 500);
    if (task_request->message->opt == kCheck &&
        version != -1 && 
        version != task_request->message->next_version)
    {
      std::stringstream ss;
      tair::rpc::RpcContext::Cast2String(task_request->addr, ss);
      TBSYS_LOG(INFO, "Configuration not match, sync all Address:%s Operation:%d CID:%s Version:%ld NextVersion:%ld", ss.str().c_str(),
                task_request->message->opt, task_request->message->cid.c_str(), version, task_request->message->next_version);
      version = handler_->client().NotifyConfiguration(task_request->addr,
                                             task_request->message->cid,
                                             task_request->message->configs,
                                             0, task_request->message->next_version,
                                             kSync, 2000);
    }

    if (version == -1)
    {
      __sync_add_and_fetch(&task_request->message->failed_count, 1); 
      std::stringstream ss;
      tair::rpc::RpcContext::Cast2String(task_request->addr, ss);
      TBSYS_LOG(ERROR, "Failed request Address:%s Operation:%d CID:%s Version:%ld", ss.str().c_str(),
                task_request->message->opt, task_request->message->cid.c_str(), version);
    }
    else if (TBSYS_LOGGER._level >= TBSYS_LOG_LEVEL_INFO)
    {
      std::stringstream ss;
      tair::rpc::RpcContext::Cast2String(task_request->addr, ss);
      TBSYS_LOG(INFO, "Successed request Address:%s Operation:%d CID:%s Version:%ld NextVersion:%ld", ss.str().c_str(),
                task_request->message->opt, task_request->message->cid.c_str(), version, task_request->message->next_version);
    }
    // OnTaskRequestFinished may be free message
    std::string uuid = task_request->message->uuid;
    int failed_count = __sync_add_and_fetch(&task_request->message->failed_count, 0); 
    if (handler_->task_scheduler().OnTaskRequestFinished(task_request) && uuid != "")
    {
      if (failed_count != 0)
      {
        TBSYS_LOG(ERROR, "Task partitial successed, failed count:%d", failed_count); 
        handler_->UpdateOplog(uuid, "FAILED"); 
      }
      else
        handler_->UpdateOplog(uuid, "SUCCESSED"); 
    }
  }
}

}
}

