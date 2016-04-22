#include "task_scheduler.h"

namespace tair
{
namespace admin
{

TaskScheduler::TaskScheduler() 
    : task_message_queue_size_(0), 
      task_request_queue_size_(0),
      request_doing_count_(0), 
      interrupted_(false)
{
  pthread_mutex_init(&mutex_, NULL);
  pthread_cond_init(&cond_, NULL);
}
 
TaskScheduler::~TaskScheduler()
{
  while (!task_message_queue_.empty())
  {
    task_message_queue_.front()->UnRef();
    task_message_queue_.pop();
  }
  while (!task_request_queue_.empty())
  {
    TaskRequest *request = task_request_queue_.front();
    request->message->UnRef();
    delete request;
    task_request_queue_.pop();
  }
  pthread_mutex_destroy(&mutex_);
  pthread_cond_destroy(&cond_);
}

TaskRequest* TaskScheduler::PopTaskRequest()
{
  if (interrupted_) return NULL;
  TaskRequest *request = NULL;
  pthread_mutex_lock(&mutex_);
  {
    if (task_request_queue_.empty() == true)
    {
      bool wait = false;
      do 
      {
        // wait condition tables
        // ------------------------------------------------------------------------
        // |requset_queue.isEmpty |doing>0 |message_queue.isEmpty |wait_condition |
        // ------------------------------------------------------------------------
        // |true                  |true    |false                 |Y              |
        // |true                  |true    |true                  |Y              |
        // |true                  |false   |true                  |Y              |
        // |true                  |false   |false                 |N              |
        // |false                 |false   |true                  |N              |
        // |false                 |false   |false                 |N              |
        // |false                 |true    |true                  |N              |
        // |false                 |true    |false                 |N              |
        // ------------------------------------------------------------------------
        wait = false;
        if (task_request_queue_.empty() == true
              && (request_doing_count_ > 0 || task_message_queue_.empty() == true))
        {
          wait = true;
          pthread_cond_wait(&cond_, &mutex_);
        }
      } while(wait && interrupted_ == false); 

      if (interrupted_ == true)
      {
      }
      else if (task_request_queue_.empty() == true
                && task_message_queue_.empty() == false)
      {
        TaskMessage *t = task_message_queue_.front();
        task_message_queue_.pop();
        task_message_queue_size_ -= 1;
        for (size_t i = 0; i < t->addrs.size(); ++i)
        {
          t->Ref();
          request = new TaskRequest();
          request->message = t;
          request->addr = t->addrs[i]; 
          task_request_queue_.push(request);
          request = NULL;
        }
        t->UnRef();
        task_request_queue_size_  += t->addrs.size();
        request_doing_count_      += t->addrs.size();
      }
      else
      {
        assert(task_request_queue_.empty() == false);
      }
    }
    if (interrupted_ == false)
    {
      assert(task_request_queue_.empty() == false);
      request = task_request_queue_.front();
      task_request_queue_.pop();
      task_request_queue_size_ -= 1;
    }
  }
  pthread_mutex_unlock(&mutex_); 
  return request;
}

bool TaskScheduler::OnTaskRequestFinished(TaskRequest *t)
{
  pthread_mutex_lock(&mutex_);
  request_doing_count_ -= 1;
  if (request_doing_count_ == 0)
    pthread_cond_broadcast(&cond_);
  pthread_mutex_unlock(&mutex_);
  bool deleted = t->message->UnRef();
  delete t;
  return deleted;
}

void TaskScheduler::PushTask(const std::string &uuid, const std::string &cid, 
                             const std::map<std::string, std::string> &configs,
                             int64_t last_version, 
                             int64_t next_version, 
                             const std::vector<easy_addr_t> &addrs,
                             TaskOperation opt)
{
  assert(addrs.size() > 0);
  TaskMessage *task = new TaskMessage();
  task->cid           = cid;
  task->configs       = configs;
  task->last_version  = last_version;
  task->next_version  = next_version;
  task->opt           = opt;
  task->addrs         = addrs;
  task->uuid          = uuid;
  task->failed_count  = 0;
  task->set_ref(1);

  pthread_mutex_lock(&mutex_);
  task_message_queue_.push(task);
  task_message_queue_size_ += 1;
  pthread_cond_broadcast(&cond_);
  pthread_mutex_unlock(&mutex_);
}

void TaskScheduler::Interrupt()
{
  pthread_mutex_lock(&mutex_);
  interrupted_ = true;
  pthread_cond_broadcast(&cond_);
  pthread_mutex_unlock(&mutex_);
}

}
}

