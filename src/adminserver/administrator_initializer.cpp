#include "administrator_initializer.h"
#include <stdint.h>
#include <tbsys.h>
#include <string.h>
#include "administrator_client.h"

namespace tair
{
namespace admin
{

AdministratorInitializer::AdministratorInitializer()
    : handler_(NULL), queue_index_(0)
{
}

AdministratorInitializer::~AdministratorInitializer()
{
}

Status AdministratorInitializer::InitializeHandler(AdministratorHandler *handler)
{
  this->handler_ = handler;
  Status status = handler->LoadTairManager();
  if (status.ok() == false)
    return status;
  handler_->GrabTairInfos(tair_info_queue_);
  std::vector<void *> args(16, NULL);
  InitializeWorker(16, args);
  this->Start();
  this->Wait();
  return Status::OK();
}

void AdministratorInitializer::MaybeSyncConfiguration(AdministratorClient &client, TairInfo *tair_info)
{
  std::vector<easy_addr_t> ds_addrs;
  tair_info->GrabAddresses(ds_addrs);
  TairConfigurationIterator iter      = tair_info->begin();
  TairConfigurationIterator iter_end  = tair_info->end();
  for (; iter != iter_end; ++iter)
  {
    for (size_t i = 0; i < ds_addrs.size(); ++i)
    {
      int64_t version = client.NotifyConfiguration(ds_addrs[i], iter->first,
                                                             AdministratorClient::kEmpty,
                                                             0, 0, kCheck, 500);
      TBSYS_LOG(INFO, "Checked TairID:%s TairVersion:%ld AdminVersion:%ld",
                iter->first.c_str(),
                version, iter->second.version());
      if (version == -1 || version == iter->second.version())
        continue;
      version = client.NotifyConfiguration(ds_addrs[i], iter->first,
                                                     iter->second.entries(),
                                                     0, iter->second.version(), kSync, 2000);
      TBSYS_LOG(INFO, "Synced TairID:%s TairVersion:%ld AdminVersion:%ld",
                iter->first.c_str(),
                version, iter->second.version());
    }
  }
}

void AdministratorInitializer::Run(void *&, int32_t)
{
  while (IsStop() == false)
  {
    int32_t current_index = __sync_fetch_and_add(&queue_index_, 1);
    if (current_index >= (int32_t)tair_info_queue_.size())
    {
      break;
    }
    TairInfo *tair_info = tair_info_queue_[current_index];
    TBSYS_LOG(INFO, "Ready to initialize TairInfo:%s", tair_info->ToString().c_str());
    std::vector<easy_addr_t> addrs;
    handler_->UpdateDataServers(tair_info, addrs);
    TBSYS_LOG(INFO, "Initialized TairInfo:%s", tair_info->ToString().c_str());
    MaybeSyncConfiguration(handler_->client(), tair_info);
  }
}

}
}

