#include <iostream>
#include <unistd.h>
#include <mysql.h>
#include <getopt.h>

#include "tair_rpc.h"
#include "administrator_client.h"
#include "administrator_handler.h"
#include "administrator_initializer.h"
#include "notify_configuration_command.h"
#include "grab_group_command.h"
#include "time_event_scheduler.h"
#include "sync_worker.h"
#include "http_set_config.h"
#include "http_get_config.h"
#include "http_del_config.h"
#include "http_check_config.h"
#include "http_check_task.h"
#include "http_gettair_config.h"
#include "http_cluster_manager.h"

//TODO: add log
using namespace std;
using namespace tair::rpc;
using namespace tair::admin;

volatile bool stop = false;

RpcContext* s_context = NULL;

void SigHandler(int sig)
{
  switch (sig)
  {
    case SIGALRM:
      {
        tzset();
        time_t t = time(NULL) - timezone;
        if (abs(t % 86400 - 86340) < 5) { //~ rotate ahead by 1min around
          TBSYS_LOG(INFO, "rotating log file...done");
          TBSYS_LOGGER.rotateLog(NULL, "%d"); //~ rotate by day
        }
        alarm(5);
        break;
      }
    case 41:
    case 42:
      {
        if (sig == 41) 
        {
          TBSYS_LOGGER._level++;
        } 
        else 
        {
          TBSYS_LOGGER._level--;
        }
        TBSYS_LOG(ERROR, "log level changed to %d.", TBSYS_LOGGER._level);
        break;
      }
    case SIGTERM:
    case SIGINT:
      {
        if (s_context != NULL)
        {
          stop = true;
          s_context->Stop();
          s_context = NULL;
        }
        break;
      }
  }
}

void print_usage(char *prog_name) 
{
  fprintf(stderr, "%s -f config_file\n"
      "    -f, --config_file  config file\n"
      "    -h, --help         show this message\n"
      "    -V, --version      show version\n\n",
      prog_name);
}

char *parse_cmd_line(int argc, char *const argv[]) 
{
  int opt; 
  const char *optstring = "hVf:";
  struct option longopts[] = 
  {
    {"config_file", 1, NULL, 'f'},
    {"help", 0, NULL, 'h'},
    {"version", 0, NULL, 'V'},
    {0, 0, 0, 0}
  };  

  char *configFile = NULL; 
  while ((opt = getopt_long(argc, argv, optstring, longopts, NULL)) != -1) 
  {
    switch (opt) 
    {
    case 'f':
      configFile = optarg;
      break;
    case 'V':
      fprintf(stderr, "BUILD_TIME: %s %s\n\n", __DATE__, __TIME__);
      exit (1);
    case 'h':
      print_usage(argv[0]);
      exit (1);
    }
  }
  return configFile;
}

int main(int argc, char *argv[])
{
  const char *configfile = parse_cmd_line(argc, argv);
  if (configfile == NULL) 
  {
    print_usage(argv[0]);
    return 1;
  }
  tbsys::CConfig config;
  if (config.load(configfile)) 
  {
      fprintf(stderr, "load ConfigFile %s failed.\n", configfile);
      return 1;
  }
  const char *pidFile = config.getString("public", "pid_file", "tair_administrator.pid");
  const char *logFile = config.getString("public", "log_file", "tair_administrator.log");

  char *p = NULL, dirpath[256];
  snprintf(dirpath, sizeof(dirpath), "%s", pidFile);
  p = strrchr(dirpath, '/');
  if (p != NULL) *p = '\0';
  if (p != NULL && !tbsys::CFileUtil::mkdirs(dirpath)) 
  {
    fprintf(stderr, "mkdirs %s failed.\n", dirpath);
    return -1;
  }
  snprintf(dirpath, sizeof(dirpath), "%s", logFile);
  p = strrchr(dirpath, '/');
  if (p != NULL) *p = '\0';
  if (p != NULL && !tbsys::CFileUtil::mkdirs(dirpath)) 
  {
    fprintf(stderr, "mkdirs %s failed.\n", dirpath);
    return -1;
  }
  int pid;
  if ((pid = tbsys::CProcess::existPid(pidFile))) 
  {
    fprintf(stderr, "process already running, pid:%d\n", pid);
    return -1;
  }
  if (tbsys::CProcess::startDaemon(pidFile, logFile)) 
  {
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
  } 
  const char *logLevel      = config.getString("public", "log_level", "info");
  const char *db_addr       = config.getString("public", "db_addr", "localhost");
  const char *db_user       = config.getString("public", "db_user", "root");
  const char *db_pasw       = config.getString("public", "db_pasw", "123456");
  const char *db_name       = config.getString("public", "db_name", "tair_administrator");
  int         db_port       = config.getInt("public", "db_port", 3306); 
  int  http_port            = config.getInt("public", "http_port", 8080);
  int  http_thread_count    = config.getInt("public", "http_thread_count", 4);
  int  rpc_port             = config.getInt("public", "rpc_port", 7191);
  int  rpc_thread_count     = config.getInt("public", "rpc_thread_count", 4);
  int  worker_thread_count  = config.getInt("public", "worker_thread_count", 8);
  TBSYS_LOGGER.setLogLevel(logLevel);

  AdministratorHandler handler;
  // init handler.ready
  handler.ready = false;
  RpcDispatcherEIO dispatcher;
  RpcContext::Properties prop; 
  prop.io_count           = rpc_thread_count;
  prop.rpc_port           = rpc_port;
  prop.http_port          = http_port;
  prop.http_thread_count  = http_thread_count;
  prop.server_dispatcher  = &dispatcher;
  prop.user_handler       = &handler;
  // construct http command
  HttpSetConfig		   http_set_config;
  HttpGetConfig		   http_get_config;
  HttpDelConfig		   http_del_config;
  HttpCheckConfig      http_check_config;
  HttpCheckTask        http_check_task;
  HttpGetTairConfig	   http_gettair_config;
  HttpAddCluster       http_add_cluster;
  // construct context
  RpcContext *context = new RpcContext();
  context->GetRpcCommandFactoryBuilder()
         ->RegisterTairCommand(new NotifyConfigurationCommand())
         ->RegisterTairCommand(new GrabGroupCommand())
         ->RegisterHttpCommand(http_set_config)
         ->RegisterHttpCommand(http_del_config)
         ->RegisterHttpCommand(http_get_config)
         ->RegisterHttpCommand(http_check_config)
         ->RegisterHttpCommand(http_check_task)
         ->RegisterHttpCommand(http_gettair_config)
         ->RegisterHttpCommand(http_add_cluster)
         ->Build();

  if (context->Startup(prop) == false)
  {
    delete context;
    exit(-1);
  }
  handler.client().set_rpc_client_context(context->client_context());
  handler.set_mysql(db_addr, db_user, db_pasw, db_name, db_port);

  AdministratorInitializer initializer;
  Status status = initializer.InitializeHandler(&handler);
  if (status.ok() == false)
  {
    TBSYS_LOG(ERROR, "Initialize Failed %s", status.message().c_str());
    return -1;
  }
  handler.SetAdminInitializer(&initializer);
  SyncWorker          sync_worker;
  sync_worker.set_handler(&handler);
  sync_worker.set_worker_count(worker_thread_count);
  sync_worker.Start();
  // init time event scheduler
  TimeEventScheduler time_event_scheduler;
  time_event_scheduler.set_handler(&handler);
  TairClusterIterator tair_iter     = handler.tair_manager().begin();
  TairClusterIterator tair_iter_end = handler.tair_manager().end();

  time_t interval = 2;
  for (; tair_iter != tair_iter_end; ++tair_iter)
  {
    time_event_scheduler.PushEvent(&(tair_iter->second), time(NULL) + interval);
    interval += 5;
  }
  handler.SetTimeEventScheduler(&time_event_scheduler);
  time_event_scheduler.Start();

  // begin service
  handler.ready = true;
  signal(SIGPIPE, SIG_IGN);
  signal(SIGHUP, SIG_IGN);
  signal(SIGINT, SigHandler);
  signal(SIGTERM, SigHandler);
  signal(SIGALRM, SigHandler);
  signal(41, SigHandler);
  signal(42, SigHandler);
  alarm(5);
  s_context = context;
  // wait
  context->Wait();
  sync_worker.Stop();
  sync_worker.Wait();
  time_event_scheduler.Stop();
  time_event_scheduler.Wait();

  context->Shutdown();

  delete context;
  google::protobuf::ShutdownProtobufLibrary();
  mysql_library_end();
  return 0;
}

