#include "reference_count.h"

namespace tair
{
namespace rpc
{

ReferenceCount::~ReferenceCount()
{
  assert(refcnt_ == 0);
}

}
}

