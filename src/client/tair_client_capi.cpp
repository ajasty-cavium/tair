/*
 * (C) 2007-2010 Alibaba Group Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id: tair_client_capi.cpp 1961 2013-11-20 09:57:21Z dutor $
 *
 * Authors:
 *   MaoQi <maoqi@taobao.com>
 *
 */
#include "tair_client_api.hpp"
#include "tair_client_capi.hpp"
#include "query_info_packet.hpp"
#include "tair_client.hpp"
#include "tair_client_api_impl.hpp"

using namespace tair;

tair_handler tair_init()
{
   return new tair_client_api();
}

void tair_set_ioth_count(tair_handler handler, int count)
{
  tair_client_api *tair = (tair_client_api*) handler;
  tair->set_thread_count(count);
}

void tair_set_light_mode(tair_handler handler)
{
  tair_client_api *tair = (tair_client_api*) handler;
  tair->set_light_mode();
}

void tair_deinit(tair_handler handler)
{
   tair_client_api* tc = (tair_client_api*)handler;
   tc->close();
   delete tc;
}

void tair_set_loglevel(const char* level )
{
   TBSYS_LOGGER.setLogLevel((char*)level);
}

int tair_begin(tair_handler handler, const char *master_addr,const char *slave_addr, const char *group_name)
{
   if (handler == 0
        || master_addr <= 0
        || group_name == 0)
   {
      return EXIT_FAILURE;
   }
   tair_client_api* tc = (tair_client_api*)handler;
   return tc->startup(master_addr, slave_addr, group_name) ? EXIT_SUCCESS : EXIT_FAILURE;
}

int tair_put(tair_handler handler, int area, tair_data_pair* key, tair_data_pair* data, int expire, int version)
{
  return tair_put_with_cache(handler, area, key, data, expire, version, true);
}

int tair_put_with_cache(tair_handler handler, int area, tair_data_pair* key, tair_data_pair* data, int expire, int version, bool fill_cache)
{
   if (handler == 0)
      return EXIT_FAILURE;

   data_entry _key(key->data, key->len, true);
   data_entry _data(data->data, data->len, true);

   tair_client_api* tc = (tair_client_api*)handler;
   return tc->put(area, _key, _data, expire, version, fill_cache);
}

int tair_get(tair_handler handler, int area, tair_data_pair* key, tair_data_pair* data)
{
   if (handler == 0)
      return EXIT_FAILURE;

   data_entry _key(key->data, key->len, true);
   data_entry* _data = 0;

   tair_client_api* tc = (tair_client_api*)handler;
   int ret = EXIT_FAILURE;
   if ( (ret = tc->get(area, _key, _data)) < 0 )
      return ret;



   data->len = _data->get_size();
   data->data = (char*)malloc(data->len + 1);
   memset(data->data, 0, data->len + 1 );
   memcpy(data->data, _data->get_data(), data->len );


   delete _data;
   _data = 0;
   return EXIT_SUCCESS;
}

int tair_remove(tair_handler handler, int area, tair_data_pair* key)
{
   if (handler == 0)
      return EXIT_FAILURE;

   data_entry _key(key->data, key->len, true);
   tair_client_api* tc = (tair_client_api*)handler;

   return tc->remove(area, _key);
}
/*
  int tairRemoveArea(tair_handler handler, int area, int timeout)
  {
  if ( handler == 0 )
  return EXIT_FAILURE;

  tair_client_api* tc = (tair_client_api*)handler;

  return tc->removeArea(area, timeout);
  }*/


int tair_incr(tair_handler handler, int area, tair_data_pair* key, int count, int* ret_count, int init_value, int expire)
{
   if (handler == 0)
      return EXIT_FAILURE;

   data_entry _key(key->data, key->len, true);

   tair_client_api* tc = (tair_client_api*)handler;
    
   return tc->incr(area, _key, count, ret_count, init_value, expire);
}

int tair_decr(tair_handler handler, int area, tair_data_pair* key, int count, int* ret_count, int init_value, int expire)
{
   if (handler == 0)
      return EXIT_FAILURE;

   data_entry _key(key->data, key->len, true);

   tair_client_api* tc = (tair_client_api*)handler;
    
   return tc->decr(area, _key, count, ret_count, init_value, expire);
}

int tair_set_flow_limit(tair_handler handler, int area, int lower, int upper, int type)
{
  tair_client_api *tair = (tair_client_api*) handler;
  return tair->set_flow_limit_bound(area, lower, upper, (tair::stat::FlowType)type);
}

int tair_alloc_area_quota(tair_handler handler, long quota, int* new_area)
{
   if (handler == 0)
      return EXIT_FAILURE;
  
   tair_client_api* tc = (tair_client_api*)handler;
   return tc->alloc_area_quota(quota, new_area);
}

int tair_modify_area_quota(tair_handler handler, int area, long quota)
{
   if (handler == 0)
      return EXIT_FAILURE;
  
   tair_client_api* tc = (tair_client_api*)handler;
   return tc->modify_area_quota(area, quota);
}

int tair_delete_area_quota(tair_handler handler, int area)
{
  return tair_modify_area_quota(handler, area, 0);
}

int tair_clear_area(tair_handler handler, int area)
{
   if (handler == 0)
      return EXIT_FAILURE;

   tair_client_api* tc = (tair_client_api*)handler;
   return tc->clear_area(area);
}

int tair_get_flow(tair_handler handler, int64_t *inflow, int64_t *outflow, int area)
{
  if (handler == NULL) {
    return -1;
  }
  tair_client_api *tair = reinterpret_cast<tair_client_api*> (handler);
  if (tair == NULL) {
    return -1;
  }
  set<uint64_t> servers;
  tair->get_servers(servers);
  *inflow = 0;
  *outflow = 0;
  for (set<uint64_t>::const_iterator iter = servers.begin(); iter != servers.end(); ++iter) {
    uint64_t addr = *iter;
    tair::stat::Flowrate rate;
    int ret = tair->get_flow(addr, area, rate);
    if (ret == 0) {
        *inflow += rate.in;
        *outflow += rate.out;
    } else {
        fprintf(stderr, "fail %s \terr:%d\n", tbsys::CNetUtil::addrToString(addr).c_str(), ret);
        return -1;
    }
  }
  return 0;
}

char** tair_fetch_statistics(tair_handler handler, const char* group, int area)
{
  static const int max_items = 16;
  static const int buffer_size_per_item = 64;
  if (handler == NULL) {
    return NULL;
  }
  tair_client_api *tair = reinterpret_cast<tair_client_api*> (handler);
  if (tair == NULL) {
    return NULL;
  }
  std::map<std::string, std::string> result_map;
  tair->query_from_configserver(tair::request_query_info::Q_STAT_INFO, group, result_map, 0);
  if (result_map.empty()) {
    return NULL;
  }
  std::map<std::string, uint64_t> stat_of_area;
  {
    std::map<std::string, std::string>::iterator itr = result_map.begin();
    while (itr != result_map.end()) {
      int ns;
      char stat_name[32];
      uint64_t stat_value;
      sscanf(itr->first.c_str(), "%d %s", &ns, stat_name);
      if (ns != area) { //~ filter out other areas'
        ++itr;
        continue;
      }
      stat_value = atoll(itr->second.c_str());
      stat_of_area[stat_name] = stat_value;
      ++itr;
    }
  }

  if (stat_of_area.empty()) {
    return NULL;
  }
  char **stats = (char**) malloc(max_items * sizeof(char*));
  int index = 0;

  {
    std::map<std::string, uint64_t>::iterator itr = stat_of_area.begin();
    while (itr != stat_of_area.end()) {
      char *item = (char*) malloc(buffer_size_per_item);
      snprintf(item, buffer_size_per_item, "%s: %lu", itr->first.c_str(), itr->second);
      stats[index++] = item;
      ++itr;
    }
  }

  tair->query_from_configserver(tair::request_query_info::Q_AREA_CAPACITY, group, result_map, 0);
  {
    std::map<std::string, std::string>::iterator itr = result_map.begin();
    uint64_t quota = 0;
    while (itr != result_map.end()) {
      int ns;
      sscanf(itr->first.c_str(), "area(%d)", &ns);
      if (ns == area) {
        quota = atoll(itr->second.c_str());
        break;
      }
      ++itr;
    }
    char *item = (char*) malloc(buffer_size_per_item);
    snprintf(item, buffer_size_per_item, "quota: %lu", quota);
    stats[index++] = item;
  }
  stats[index] = NULL; //~ null-terminated
  return stats;
}

void tair_free_statistics(tair_handler handler, char **stats)
{
  if (stats == NULL) {
    return ;
  }
  char **itr = stats;
  while (*itr != NULL) {
    free(*itr);
    ++itr;
  }
  free(stats);
}
