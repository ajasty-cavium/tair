#include "rpc_http_console.h"

#include <sstream>
#include <pthread.h>
#include <sys/prctl.h>
#include "rpc_command_factory.h"
#include "mongoose.h"

namespace tair
{
namespace rpc
{

RpcHttpConsole::RpcHttpConsole(int port, int count, RpcCommandFactory *cmd_factory)
    : context_(NULL), port_(port), worker_count_(count), stop_(false), cmd_factory_(cmd_factory)
{
}

RpcHttpConsole::~RpcHttpConsole()
{
  if (worker_infos_ == NULL) return ;
  for (int i = 0; i < worker_count_; ++i)
  {
    WorkerInfo *wi  = mutable_worker_info(i);
    if (i != 0)
      mg_set_listening_socket(wi->http_server, -1);
    mg_destroy_server(&(wi->http_server));
    wi->console     = NULL;
    wi->http_server = NULL;
  }
  delete[] worker_infos_;
}

void* RpcHttpConsole::ThreadFunc(void *arg)
{
  prctl(PR_SET_NAME, "rpc-http-console");
  WorkerInfo *wi = reinterpret_cast<WorkerInfo *>(arg);
  wi->console->Run(wi);
  return NULL;
}

void RpcHttpConsole::Stop()
{
  this->stop_ = true;
}

void RpcHttpConsole::Start()
{
  std::stringstream ss;
  ss << port_;
  std::string sport = ss.str();
  worker_infos_ = new WorkerInfo[worker_count_];
  int sockfd = -1;
  for (int i = 0; i < worker_count_; ++i)
  {
    WorkerInfo *wi    = mutable_worker_info(i);
    wi->console       = this;
    wi->http_server   = mg_create_server(context_->command_handler_);
    wi->pid           = 0;
    const std::unordered_map<std::string, RpcHttpConsoleCommand::Command> &http_cmd 
                  = cmd_factory_->http_commands(); 
    std::unordered_map<std::string, RpcHttpConsoleCommand::Command>::const_iterator iter 
                  = http_cmd.begin();
    for (; iter != http_cmd.end(); ++iter)
    {
      mg_add_uri_handler(wi->http_server, iter->first.c_str(), iter->second);
    }
    if (i == 0)
    {
      mg_set_option(wi->http_server, "listening_port", sport.c_str());
      sockfd = mg_get_listening_socket(wi->http_server);
    }
    else
      mg_set_listening_socket(wi->http_server, sockfd);
  }

  for (int i = 0; i < worker_count_; ++i)
  {
    WorkerInfo *wi = mutable_worker_info(i);
    pthread_create(&wi->pid, NULL, RpcHttpConsole::ThreadFunc, wi); 
  }
}

void RpcHttpConsole::Run(WorkerInfo *wi)
{
  while (this->stop_ == false)
  {
    mg_poll_server(wi->http_server, 1000);
  }
}

void RpcHttpConsole::Join()
{
  for (int i = 0; i < worker_count_; ++i)
  {
    WorkerInfo *wi = mutable_worker_info(i);
    pthread_join(wi->pid, NULL);
  }
}

}
}

