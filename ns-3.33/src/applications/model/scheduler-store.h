#ifndef SCH_STORE_H
#define SCH_STORE_H

#include <unordered_map>
#include "ns3/probe-header2.h"
#include "ns3/probe-payload2.h"
#include "scheduler-topo.h"
#include <stdlib.h>

using namespace ns3;
typedef struct
{
  int maxQueueOccupancy;
} portEntry;

typedef struct
{
  std::unordered_map<int, portEntry> ports;
} switchEntry;

class SchedulerStore
{
public:
  void onSwitchUpdate (ProbeHeader2 header, std::vector<ProbePayload2> payload);
  void log(void);

private:
  std::unordered_map<int, std::unordered_map<int, portEntry>> switches;
  SchedulerTopo topo;
};

#endif