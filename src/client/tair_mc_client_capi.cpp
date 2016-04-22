#include "tair_mc_client_capi.hpp"
#include "tair_mc_client_api.hpp"

using tair::tair_mc_client_api;
using tair::data_entry;

void tair_mc_set_log_level(const char* level)
{
  TBSYS_LOGGER.setLogLevel(level);
}

void tair_mc_set_log_file(const char* log_file)
{
  TBSYS_LOGGER.setFileName(log_file);
}

tair_mc_handler tair_mc_init()
{
  return new tair_mc_client_api();
}

void tair_mc_set_net_device_name(tair_mc_handler handler, const char* net_device_name)
{
  tair_mc_client_api* tc = (tair_mc_client_api*)handler;
  tc->set_net_device_name(net_device_name);
}

void tair_mc_set_timeout(tair_mc_handler handler, int timeout)
{
  tair_mc_client_api* tc = (tair_mc_client_api*)handler;
  tc->set_timeout(timeout);
}

int tair_mc_begin(tair_mc_handler handler, const char* config_id)
{
  if (NULL == handler || NULL == config_id)
    return TAIR_RETURN_FAILED;
  tair_mc_client_api* tc = (tair_mc_client_api*)handler;
  return tc->startup(config_id) ? TAIR_RETURN_SUCCESS : TAIR_RETURN_FAILED;
}

void tair_mc_deinit(tair_mc_handler handler)
{
  tair_mc_client_api* tc = (tair_mc_client_api*)handler;
  tc->close();
  delete tc;
}

int tair_mc_put(tair_mc_handler handler, int area, tair_data_pair* key, tair_data_pair* data, int expire, int version)
{
  return tair_mc_put_with_cache(handler, area, key, data, expire, version, true);
}

int tair_mc_put_with_cache(tair_mc_handler handler, int area, tair_data_pair* key, tair_data_pair* data,
                        int expire, int version, bool fill_cache)
{
  if (handler == 0)
    return TAIR_RETURN_INVALID_ARGUMENT;
  data_entry _key(key->data, key->len, false);
  data_entry _data(data->data, data->len, false);

  tair_mc_client_api* tc = (tair_mc_client_api*)handler;
  return tc->put(area, _key, _data, expire, version, fill_cache);
}

int tair_mc_get(tair_mc_handler handler, int area, tair_data_pair* key, tair_data_pair* data)
{
   if (handler == 0)
      return TAIR_RETURN_INVALID_ARGUMENT;

   data_entry _key(key->data, key->len, false);
   data_entry* _data = NULL;

   tair_mc_client_api* tc = (tair_mc_client_api*)handler;
   int ret = tc->get(area, _key, _data);
   if (ret< 0)
     return ret;

   data->len = _data->get_size();
   data->data = (char*)malloc(data->len + 1);
   memcpy(data->data, _data->get_data(), data->len );
   data->data[data->len] = '\0';

   delete _data;
   _data = NULL;
   return TAIR_RETURN_SUCCESS;
}

int tair_mc_remove(tair_mc_handler handler, int area, tair_data_pair* key)
{
  if (handler == 0)
    return TAIR_RETURN_INVALID_ARGUMENT;

   data_entry _key(key->data, key->len, false);
   tair_mc_client_api* tc = (tair_mc_client_api*)handler;

   return tc->remove(area, _key);
}

int tair_mc_invalidate(tair_mc_handler handler, int area, tair_data_pair* key, bool is_sync)
{
  if (handler == NULL)
    return TAIR_RETURN_INVALID_ARGUMENT;

  data_entry _key(key->data, key->len, false);
  tair_mc_client_api* tc = (tair_mc_client_api*)handler;

  return tc->invalidate(area, _key, is_sync);
}

int tair_mc_incr(tair_mc_handler handler, int area, tair_data_pair* key, int count, int* ret_count, int init_value, int expire)
{
   if (handler == 0)
      return TAIR_RETURN_INVALID_ARGUMENT;

   data_entry _key(key->data, key->len, false);

   tair_mc_client_api* tc = (tair_mc_client_api*)handler;

   return tc->incr(area, _key, count, ret_count, init_value, expire);
}

int tair_mc_decr(tair_mc_handler handler, int area, tair_data_pair* key, int count, int* ret_count, int init_value, int expire)
{
   if (handler == 0)
      return TAIR_RETURN_INVALID_ARGUMENT;

   data_entry _key(key->data, key->len, false);
   tair_mc_client_api* tc = (tair_mc_client_api*)handler;

   return tc->decr(area, _key, count, ret_count, init_value, expire);
}

const char* get_error_msg(tair_mc_handler handler, int ret)
{
  tair_mc_client_api* tc = (tair_mc_client_api*)handler;
  return  tc->get_error_msg(ret);
}
