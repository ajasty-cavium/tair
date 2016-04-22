#include <string>
#include <iostream>
#include <getopt.h>
#include <tbsys.h>
#include <tair_rpc.h>

#include "echo_command.hpp"
#include "http_demo_command.h"

using namespace std;
using namespace tair::rpc;


easy_io_t eio;

void SigHandler(int sig)
{
  switch (sig)
  {
    case SIGTERM:
    case SIGINT:
      easy_io_stop(&eio);
      break; 
  }
}

/**
 * Construct
 * eio        -> ServerContext
 * Dispatcher -> ServerContext
 * Handler    -> ServerContext
 *
 * Start Service
 * Dispathcer.Start and Handler.Start, before ServerContext.Start
 *
 * Stop Service
 * First eio.Stop, Second ServerContext.Stop, Last dispatcher.Stop, Handler.Stop
 *
 * Destruct
 * Delete ServerContext 
 * Delete Dispatcher and Handler
 * Delete eio
 */
int main(int argc, char *argv[]) 
{

  int iocount     = 4;
  int workercount = 4;

  memset(&eio, 0, sizeof(eio));
  if (easy_io_create(&eio, iocount) == 0) 
  {
    cerr << "eio crate failed" << endl;
  }
  eio.do_signal = 0;
  eio.no_redispatch = 0;
  eio.no_reuseport = 1;
  eio.listen_all = 1;
  eio.tcp_nodelay = 1;
  eio.tcp_cork = 0;
  eio.affinity_enable = 1; 

  signal(SIGINT, SigHandler);
  signal(SIGTERM, SigHandler);

  RpcServerContext::Initialize();

  RpcCommandFactory *factory = RpcCommandFactory::Instance();
  factory->RegisterCommand(new EchoCommand());
  RpcHttpDemoCommand http_cmd;
  factory->RegisterHttpCommand(http_cmd);

  RpcDispatcherDefault *dispatcher = new RpcDispatcherDefault(workercount);
  RpcHttpConsole *console          = new RpcHttpConsole(8080, 1, factory);
  RpcServerContext *server_context = new RpcServerContext(&eio, dispatcher, NULL);
  server_context->SetupHttpConsole(console);
  server_context->set_port(8818);

  dispatcher->Start();  
  console->Start();
  server_context->Run();

  server_context->Wait();

  console->Stop();
  console->Join();
  dispatcher->Stop();
  dispatcher->Join();
  // Destroy
  delete server_context;
  delete console;
  delete dispatcher;
  easy_io_destroy(&eio);
  RpcServerContext::Destroy();
  return 0;
}

