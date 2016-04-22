/*
 * (C) 2007-2010 Alibaba Group Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id: tair_client_capi.hpp 2870 2014-08-14 05:16:31Z yunhen $
 *
 * Authors:
 *   MaoQi <maoqi@taobao.com>
 *
 */
#ifndef CTAIR_CLIENT_API_H
#define CTAIR_CLIENT_API_H

#include <stdbool.h>
#include <stddef.h>

#include "capi_define.hpp"

#if __cplusplus
extern "C"
{
#endif

   typedef void* tair_handler;

   tair_handler tair_init();
   void tair_set_ioth_count(tair_handler handler, int count);
   void tair_set_light_mode(tair_handler handler);
   void tair_deinit(tair_handler handler);
   void tair_set_loglevel(const char* level);
   int tair_begin(tair_handler handler, const char *master_addr,const char *slave_addr, const char *group_name);
   int tair_put(tair_handler handler, int area, tair_data_pair* key, tair_data_pair* data, int expire, int version);
   int tair_put_with_cache(tair_handler handler, int area, tair_data_pair* key, tair_data_pair* data, int expire, int version, bool fill_cache);
   int tair_get(tair_handler handler, int area, tair_data_pair* key, tair_data_pair* data);
   int tair_remove(tair_handler handler, int area, tair_data_pair* key);
//int tairRemoveArea(tair_handler handler, int area, int timeout);
   int tair_incr(tair_handler handler, int area, tair_data_pair* key, int count, int* ret_count, int init_value, int expire);
   int tair_decr(tair_handler handler, int area, tair_data_pair* key, int count, int* ret_count, int init_value, int expire);
   /*
    * type: 0 for IN, 1 for OUT, 2 for QPS
    */
   int tair_set_flow_limit(tair_handler handler, int area, int lower, int upper, int type);
   int tair_alloc_area_quota(tair_handler handler, long quota, int* new_area);
   int tair_modify_area_quota(tair_handler handler, int area, long quota);
   int tair_delete_area_quota(tair_handler handler, int area);
   int tair_clear_area(tair_handler handler, int area);
   /*
    * @brief: query statistics from config server, by area
    * @format: stats[0]: "getCount: 1024", stats[1]: "putCount: 4096", ...
    * @items: getCount, putCount, removeCount, hitCount, evictCount, itemCount, dataSize, useSize, quota
    * @return: null-terminated char*[]
    */
   char** tair_fetch_statistics(tair_handler handler, const char *group, int area);

   int tair_get_flow(tair_handler handler, int64_t *inflow, int64_t *outflow, int area);
   /*
    * @brief: free memory of stats returned by tair_fetch_statistics
    */
   void tair_free_statistics(tair_handler handler, char **stats);

#if __cplusplus
}
#endif
#endif
