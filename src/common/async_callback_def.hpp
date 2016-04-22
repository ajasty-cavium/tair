#ifndef _ASYNC_CALLBACK_DEF_HPP_
#define _ASYNC_CALLBACK_DEF_HPP_

#include "data_entry.hpp"

namespace tair { class response_mc_ops; }
namespace tair { namespace common {

typedef void (*TAIRCALLBACKFUNC)(int retcode, void* parg);
typedef void (*TAIRCALLBACKFUNC_EX)(int recode, const key_code_map_t *key_code_map, void *parg);
typedef void (*callback_get_pt)(int retcode, const data_entry *key, const data_entry *data, void *parg);
typedef void (*callback_mc_ops_pt)(int retcode, response_mc_ops *resp, void *args);
typedef TAIRCALLBACKFUNC callback_put_pt;
typedef TAIRCALLBACKFUNC callback_remove_pt;
typedef TAIRCALLBACKFUNC_EX callback_mreturn_pt;

}} // tair

#endif
