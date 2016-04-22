#ifndef ADMIN_TASK_SECHDULER_H
#define ADMIN_TASK_SECHDULER_H

#include <queue>
#include <string>
#include <map>
#include "tair_rpc.h"

namespace tair
{
namespace admin
{
 
class TairInfo;

enum TaskOperation
{
  kSet = 0, kDel = 1, kCheck = 2, kSync = 3, kGet = 4
};

struct TaskMessage : public tair::rpc::ReferenceCount
{
  std::string                cid;
  int64_t                    last_version;
  int64_t                    next_version;
  int32_t                    failed_count;
  TaskOperation              opt;
  std::vector<easy_addr_t>   addrs;
  std::string                uuid;
  std::map<std::string, std::string> configs;
};

struct TaskRequest
{
  TaskMessage *message;
  easy_addr_t addr;
};

class TaskScheduler
{
 public:
  TaskScheduler();
  ~TaskScheduler();

  TaskRequest* PopTaskRequest();

  bool OnTaskRequestFinished(TaskRequest *); 

  void Interrupt();

  int32_t pending_task_count() 
  { 
    return task_message_queue_size_;
  }

  int32_t doing_task_count() 
  { 
    return request_doing_count_;
  }

  void PushTask(const std::string &uuid, const std::string &cid, 
                const std::map<std::string, std::string> &configs,
                int64_t last_version, 
                int64_t next_version, 
                const std::vector<easy_addr_t> &addrs,
                TaskOperation opt);

 private:
  std::queue<TaskMessage *>   task_message_queue_;
  int32_t                     task_message_queue_size_;
  std::queue<TaskRequest *>   task_request_queue_;
  int32_t                     task_request_queue_size_;
  int32_t                     request_doing_count_;
  pthread_mutex_t             mutex_; 
  pthread_cond_t              cond_;
  volatile bool               interrupted_;
};
 
}
}

#endif
