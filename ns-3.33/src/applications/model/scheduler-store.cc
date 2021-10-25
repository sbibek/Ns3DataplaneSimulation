#include "scheduler-store.h"
#include <algorithm>

bool
sortfn (const std::tuple<int, int, int> &a, const std::tuple<int, int, int> &b)
{
  return (std::get<1> (a) < std::get<1> (b));
}

void
SchedulerStore::onSwitchUpdate (ProbeHeader2 header, std::vector<ProbePayload2> payload)
{
  for (ProbePayload2 p : payload)
    {
      switches[header.GetSwid ()][p.GetPortId ()].maxQueueOccupancy = p.GetMaxQueueDepth ();
    }
//#define SHOW_OUT
#ifdef SHOW_OUT
    int swid = header.GetSwid();
    for(auto p: switches[swid]) {
      // just for test purposes, we just log the specific switch port occupancy
      NS_LOG_UNCOND("PROBE_STAT "  << swid << " " << p.first << " " << p.second.maxQueueOccupancy << std::endl);
    }
#endif
}

void
SchedulerStore::log (uint16_t swid)
{
  std::cout << "Switch: " << (int) swid << "\n";
  for (auto p : switches[swid])
    {
      std::cout << "		Port = " << p.first << " Q = " << p.second.maxQueueOccupancy
                << std::endl;
    }
}

std::unordered_map<int, int>
SchedulerStore::tracePath (uint16_t a)
{
  std::unordered_map<int, int> effectiveQs;
  int q = 0;
  for (int i = topo.minNodeId; i <= topo.maxNodeId; i++)
    {
      if (i == a)
        continue;

      q = 0;
      for (auto path : topo.topoPath.at (a).at (i).path)
        {
          q += switches[path.swid][path.portid].maxQueueOccupancy;
        }
      effectiveQs[i] = q;
    }

  //  for(auto x: effectiveQs) {
  // 	 if(x.second > 0)
  // 	 std::cout << x.first << "=" << x.second << std::endl;
  //  }
  return effectiveQs;
}

std::vector<std::tuple<int, int>>
SchedulerStore::tracePathWithTuple (uint16_t a, uint16_t selectionStragety)
{
//#define SHOW_OUT
#ifdef SHOW_OUT
    for(auto p: switches[12]) {
      // just for test purposes, we just log the specific switch port occupancy
      NS_LOG_UNCOND("PROBE_STAT " << 12 << " " << p.first << " " << p.second.maxQueueOccupancy << std::endl);
    }
#endif

  std::vector<std::tuple<int, int>> qs;
  for (int i = topo.minNodeId; i <= topo.maxNodeId; i++)
    {
      if (i == a)
        continue;

      int q = 0;
      if (selectionStragety != 2)
        { // random selection doesnt require anything
          for (auto path : topo.topoPath.at (a).at (i).path)
            {
              if (selectionStragety == 0)
                { // optimal selection will take in queue
                  q += switches[path.swid][path.portid].maxQueueOccupancy;
                }
              else if (selectionStragety == 1)
                { // near selection so we just count the hops
                  q++;
                }
            }
        }

      qs.push_back (std::tuple<int, int>{q, i});
    }

  //  for(auto x: effectiveQs) {
  // 	 if(x.second > 0)
  // 	 std::cout << x.first << "=" << x.second << std::endl;
  //  }
  std::sort (qs.begin (), qs.end ());
  return qs;
}

void
SchedulerStore::log (void)
{
  for (auto s : switches)
    {
      std::cout << "swid: " << s.first << "\n";
      for (auto p : s.second)
        {
          std::cout << "		Port = " << p.first << " Q = " << p.second.maxQueueOccupancy
                    << std::endl;
        }
    }
  std::cout << std::endl;
}