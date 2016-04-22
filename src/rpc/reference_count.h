#ifndef TAIR_RPC_REFERENCE_COUNT_H_
#define TAIR_RPC_REFERENCE_COUNT_H_

#include <stdint.h>
#include <stdio.h>
#include <cassert>

#ifndef LIKELY
#define LIKELY(x) (__builtin_expect(!!(x),1))
#endif

#ifndef UNLIKELY
#define UNLIKELY(x) (__builtin_expect(!!(x),0))
#endif

namespace tair
{
namespace rpc
{

class ReferenceCount 
{
 public:
  ReferenceCount() : refcnt_(0) {}

  inline void Ref() { __sync_add_and_fetch(&refcnt_, 1); }

  inline bool UnRef() 
  {  
    assert(refcnt_ > 0);
    int ref = __sync_sub_and_fetch(&refcnt_, 1); 
    if (ref != 0) return false;
    delete this; 
    return true;
  }

  inline void set_ref(int32_t i) { refcnt_ = i; }

  virtual ~ReferenceCount() = 0;
 protected:
  int32_t       refcnt_;
};

}
}


#endif
