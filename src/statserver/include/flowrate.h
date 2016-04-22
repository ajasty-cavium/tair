#ifndef FLOW_RATE_H
#define FLOW_RATE_H

#include <cstddef>
#include <stdint.h>

namespace tair
{
namespace stat
{

enum FlowType
{
  IN = 0, OUT = 1, OPS = 2
};

enum FlowStatus
{
  DOWN = 0, KEEP = 1, UP = 2
};

struct FlowLimit
{
  int64_t lower;
  int64_t upper;
};

struct AllFlowLimit
{
  FlowLimit in;
  FlowLimit out;
  FlowLimit ops;
};

struct Flowrate
{
  int64_t in;
  FlowStatus in_status;
  int64_t out;
  FlowStatus out_status;
  int64_t ops;
  FlowStatus ops_status;

  FlowStatus status;
};

}
}

#endif

