#ifndef TAIR_PROXY_MEMCAHCED_UNORDERED_H
#define TAIR_PROXY_MEMCAHCED_UNORDERED_H

#if __cplusplus > 199711L || defined(__GXX_EXPERIMENTAL_CXX0X__) || defined(_MSC_VER)
#include <unordered_map>
#else
#include <tr1/unordered_map>
namespace std
{
  using tr1::hash;
  using tr1::unordered_map;
}
#endif

#endif

