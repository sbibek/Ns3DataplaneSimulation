#ifndef SWITCH_NODE_H
#define SWITCH_NODE_H

#include "ns3/application.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/system-mutex.h"
#include "switch-net-device.h"
#include <unordered_map>

namespace ns3 {

class Packet;

typedef struct
{
  uint64_t timestamp_ms;
  uint64_t totalPackets;
  uint64_t totalBytes;
  uint64_t totalQueue;
  uint64_t maxQueue;
  uint64_t minQueue;
} QueueStats;

class SwitchNode : public Node
{

public:
  static TypeId GetTypeId (void);
  SwitchNode ();
  SwitchNode (uint16_t probe_port, uint64_t period_ms);

  void process (Ptr<NetDevice> incomingPort, Ptr<NetDevice> outgoingPort, Packet *packet,
                uint16_t protocol, Address const &src, Address const &dst, __stats &stats);

  void setSwid (uint32_t swid);
  uint32_t getSwid ();

  void onProbeReceived (Packet *packet, uint32_t swid, uint8_t portId, QueueStats *stats,
                 uint64_t current_ts);

private:
  std::unordered_map<uint64_t, QueueStats *> queueStats;
  SystemMutex mutex;
  uint16_t probePort = 9999;
  uint64_t period_ms = 100;
  uint32_t swid;
};

} /* namespace ns3 */

#endif /* SWITCH_NODE_H */