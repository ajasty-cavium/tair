/*
 * (C) 2007-2010 Alibaba Group Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id: tair_mc_client_capi.hpp 2829 2014-08-11 17:57:21Z yunhen $
 *
 * Authors:
 *   yunhen <huan.wanghuan@alibaba-inc.com>
 *
 */
#ifndef TAIR_MC_CLIENT_CAPI_HPP_
#define TAIR_MC_CLIENT_CAPI_HPP_

#include <stdbool.h>
#include <stddef.h>

#include "capi_define.hpp"

#if __cplusplus
extern "C"
{
#endif

  typedef void* tair_mc_handler;

  /**
   * @brief set log level
   *
   * @param level    "DEBUG"|"WARN"|"INFO"|"ERROR"
   */
  void tair_mc_set_log_level(const char* level);

    /**
     * @brief set log file
     *
     * @param log_file  the filename of log file
     */
  void tair_mc_set_log_file(const char* log_file);

  /**
   * @brief   alloc a tair_mc_handler, and need to free it with tair_mc_deinit() after use.
   */
  tair_mc_handler tair_mc_init();

  /**
   * @brief   Specify which network interface to use to get the diamond dataid context.
   *          If not invoked, it auto try all interface expect the "lo",
   *          and take the one which first match the route rule in dimond dataid-GROUP.
   *
   * @param net_device_name    the network interface, such as "eth0", "bond0".
   */
  void tair_mc_set_net_device_name(tair_mc_handler handler, const char* net_device_name);

  /**
   * @brief set timout of each method, ms
   *
   * @param timeout
   */
  void tair_mc_set_timeout(tair_mc_handler hander, int timeout);

  /**
   * @brief   connect to tair cluster
   *
   * @param handler     tair_mc_handler
   * @param config_id   diamond config_id of the tair cluster which you will connect to
   *
   * @return 0 -- success, otherwise -- fail
   */
  int tair_mc_begin(tair_mc_handler handler, const char* config_id);

  /*
   * @brief   free the tair_mc_handler
   */
  void tair_mc_deinit(tair_mc_handler handler);

  /**
   * @brief  put data to tair
   *
   * @param area     namespace
   * @param key      key
   * @param data     data
   * @param expire   expire time,realtive time
   * @param version  version,if you don't care the version,set it to 0
   *
   * @return 0 -- success, otherwise fail, you can use get_error_msg(ret) to get more information.
   */
  int tair_mc_put(tair_mc_handler handler, int area, tair_data_pair* key, tair_data_pair* data, int expire, int version);

  /**
   * @brief put data to tair
   *
   * @param area     namespace
   * @param key      key
   * @param data     data
   * @param expire   expire time,realtive time
   * @param version  version,if you don't care the version,set it to 0
   * @param fill_cache whether fill cache when put(only meaningful when server support embedded cache).
   *
   * @return 0 -- success, otherwise fail,you can use get_error_msg(ret) to get more information.
   */
  int tair_mc_put_with_cache(tair_mc_handler handler, int area, tair_data_pair* key, tair_data_pair* data,
                             int expire, int version, bool fill_cache);

  /**
   * @brief get data from tair cluster
   *
   * @param area    namespace
   * @param key     key
   * @param data    if success, data, pointer to the value get, the caller must release the memory. free(data.data)
   *
   * @return 0 -- success, otherwise fail, you can use get_error_msg(ret) to get more information.
   */
  int tair_mc_get(tair_mc_handler handler, int area, tair_data_pair* key, tair_data_pair* data);

  /**
   * @brief  remove data from tair cluster
   *
   * @param area    namespace
   * @param key     key
   *
   * @return  0 -- success, otherwise fail, you can use get_error_msg(ret) to get more information.
   */
  int tair_mc_remove(tair_mc_handler handler, int area, tair_data_pair* key);

  /**
   * @brief delete data from tair cluster, if there is no invalserver, it acts as remove().
   *
   * @param area    namespace
   * @param key     key
   *
   * @return 0 -- success, otherwise fail, you can use get_error_msg(ret) to get more information.
   */
  int tair_mc_invalidate(tair_mc_handler handler, int area, tair_data_pair* key, bool is_sync);

  /**
   * @brief incr
   *
   * @param area            namespace
   * @param key             key
   * @param count           add (count) to original value.
   * @param retCount        the latest value
   * @param expire          expire time
   * @param initValue       initialize value, if the key doesn't exist, set this initializer value, and then add count
   *
   * @return 0 -- success, otherwise fail, you can use get_error_msg(ret) to get more information.
   */
  int tair_mc_incr(tair_mc_handler handler, int area, tair_data_pair* key, int count, int* ret_count, int init_value, int expire);

  /**
   * @brief incr
   *
   * @param area            namespace
   * @param key             key
   * @param count           add (count) to original value.
   * @param retCount        the latest value
   * @param expire          expire time
   * @param initValue       initialize value, if the key doesn't exist, set this initializer value, and then add count
   *
   * @return 0 -- success, otherwise fail, you can use get_error_msg(ret) to get more information.
   */
  int tair_mc_decr(tair_mc_handler handler, int area, tair_data_pair* key, int count, int* ret_count, int init_value, int expire);

  /*
   * @brief   return message info of api ret code
   */
  const char* get_error_msg(tair_mc_handler hander, int ret);

#if __cplusplus
}
#endif
#endif
