#include "switch-node.h"
// #include "ns3/ppp-header.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-header.h"
#include "ns3/udp-header.h"
#include "ns3/query-header.h"
#include "ns3/probe-header2.h"
#include "ns3/probe-payload2.h"
#include "ns3/probe-payload-trailer.h"

namespace ns3 {

TypeId
SwitchNode::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SwitchNode").SetParent<Node> ().AddConstructor<SwitchNode> ();
  return tid;
}

SwitchNode::SwitchNode ()
{
}

SwitchNode::SwitchNode (uint16_t probe_port, uint64_t period_ms)
{
  this->probePort = probe_port;
  this->period_ms = period_ms;
}

void
SwitchNode::setSwid (uint32_t swid)
{
  this->swid = swid;
  // std::cout << "[conf][SwitchNode = " << swid << "] ProbePort = " << probePort
            // << " Period(ms) = " << period_ms << std::endl;
}

uint32_t
SwitchNode::getSwid ()
{
  return this->swid;
}

#define V2_PROCESS
#ifndef V2_PROCESS
void
onProbeReceived (Packet *packet, uint32_t swid, uint8_t portId, QueueStats *stats,
                 uint64_t current_ts)
{
  // if(swid == 1) return;
  // std::cout << "Swid:" << swid << " current ts (microsecs): " << current_ts << " Total packets: " << stats->totalPackets << " Total bytes: " << stats->totalBytes <<
  //                         // "\npps:" << st->totalPackets*1000000/(diff_miroseconds*1.0) << " BW(Mbit/s): " << st->totalBytes*1000000*8/(diff_miroseconds*1024.0*1024.0) <<
  //                         " Queue(avg, max, min)=(" << stats->totalQueue/(stats->totalPackets*1.0)<<","<< stats->maxQueue << "," << stats->minQueue << ")\n\n";
  ProbeHeader ph;
  packet->RemoveHeader (ph);
// # define TRAILER
#ifndef TRAILER
  std::vector<ProbePayload> payloads;
  for (uint32_t i = 0; i < ph.GetCount (); i++)
    {
      ProbePayload p;
      packet->RemoveHeader (p);
      payloads.push_back (p);
    }

  ProbePayload newp;
  newp.SetSwid ((swid_t) swid);
  newp.SetPortId ((portid_t) portId);
  newp.SetMaxQueueDepth ((maxqueue_t) stats->maxQueue);
  payloads.push_back (newp);

  for (int i = payloads.size () - 1; i >= 0; i--)
    {
      packet->AddHeader (payloads.at (i));
    }
#else
// not working yet
// ProbePayloadTrailer newp;
// newp.SetSwid(swid);
// newp.SetTotalPackets(stats->totalPackets);
// newp.SetGlobalQueueDepth((uint16_t)stats->maxQueue);
// packet->AddTrailer(newp);
#endif

  ph.SetCount (ph.GetCount () + 1);
  packet->AddHeader (ph);
}
#else
void
SwitchNode::onProbeReceived (Packet *packet, uint32_t swid, uint8_t outPortId, QueueStats *stats,
                 uint64_t current_ts)
{
  #define SWID_TRACE
  #ifdef SWID_TRACE
  std::cout  << swid << ":" << (int)outPortId << std::endl ;
  #endif

  ProbeHeader2 ph;
  packet->RemoveHeader (ph);

  if(ph.GetSwid() == 0) {
    ph.SetSwid(swid);
    int count = 0;
    for(auto& it: queueStats) {
      ProbePayload2 payload;
      payload.SetPortId(it.first);
      payload.SetMaxQueueDepth(it.second->maxQueue);
      packet->AddHeader(payload);
      count++;
    }

    ph.SetCount(count);
  }

  packet->AddHeader(ph);
}
#endif


void onQueryPacketReceived(Packet *packet, uint32_t swid, uint8_t outPortId, QueueStats *stats,
                 uint64_t current_ts) {
      QueryHeader qheader;
      packet->RemoveHeader(qheader);

      if(qheader.GetSwid() == 0) {
        std::cout << "Query set swid " << swid << std::endl;
        // means query doesnt have the swid yet, so lets put it in there
        qheader.SetSwid(swid);
      } 

      packet->AddHeader(qheader);
}

void
SwitchNode::process (Ptr<NetDevice> incomingPort, Ptr<NetDevice> outgoingPort, Packet *packet,
                     uint16_t protocol, Address const &src, Address const &dst, __stats &stats)
{

  // CriticalSection cs(mutex);
  mutex.Lock ();

#define STATS_COLLECTOR
#ifdef STATS_COLLECTOR
  uint64_t current_ts = ns3::Simulator::Now ().GetMicroSeconds ();

  // uint64_t outPortId = (uint64_t)PeekPointer(outgoingPort);
  // uint32_t inPortId = incomingPort->getGenericId ();
  uint32_t outPortId = outgoingPort->getGenericId ();
  //  = (uint64_t)&outgoingPort;
  QueueStats *st = queueStats[outPortId];
  if (st == NULL)
    {
      // std::cout << "Is Null " << outPortId << ns3::Simulator::Now() << std::endl << std::flush;
      st = new QueueStats ();
      st->timestamp_ms = current_ts;
      st->totalPackets++;
      st->totalBytes += packet->GetSize ();
      st->totalQueue += stats.queueOccupancy;
      st->minQueue = st->maxQueue = stats.queueOccupancy;
      queueStats[outPortId] = st;
    }
  else
    {
      st->totalPackets++;
      st->totalBytes += packet->GetSize ();
      st->totalQueue += stats.queueOccupancy;
      st->maxQueue = stats.queueOccupancy > st->maxQueue ? stats.queueOccupancy : st->maxQueue;
      st->minQueue = stats.queueOccupancy < st->minQueue ? stats.queueOccupancy : st->minQueue;

      uint64_t diff_miroseconds = (current_ts - st->timestamp_ms);
      if (diff_miroseconds >= period_ms * 1000)
        {
// so we have to now rollup
#if 0 
              std::cout << "Swid:" << swid<<" Port ID: " << outPortId << " current ts (microsecs): " << current_ts << "\nTotal time(ms): " << diff_miroseconds/1000.0 << " Total packets: " << st->totalPackets << " Total bytes: " << st->totalBytes <<
                            "\npps:" << st->totalPackets*1000000/(diff_miroseconds*1.0) << " BW(Mbit/s): " << st->totalBytes*1000000*8/(diff_miroseconds*1024.0*1024.0) <<
                            " Queue(avg, max, min)=(" << st->totalQueue/(st->totalPackets*1.0)<<","<< st->maxQueue << "," << st->minQueue << ")\n\n";
#endif
          // reset everything
          st->timestamp_ms = current_ts;
          st->totalPackets = 1;
          st->totalBytes = packet->GetSize ();
          st->totalQueue = stats.queueOccupancy;
          st->minQueue = st->maxQueue = stats.queueOccupancy;
        }
    }
#endif

#define PROCESS_PROBE
#ifdef PROCESS_PROBE

  if (protocol == 2048)
    {
      Ipv4Header iph;
      packet->RemoveHeader (iph);

      if ((int) iph.GetProtocol () == 17)
        {
          UdpHeader udph;
          packet->RemoveHeader (udph);

          if (udph.GetDestinationPort () == probePort)
            {
              // std::cout << "@probe swid:" << swid << " ports: " << inPortId << " -> " << outPortId
              // << " UDP Port: " << udph.GetDestinationPort () << std::endl;
              // std:: cout << "PROBE packet @switch=" << swid<< std::endl;
              onProbeReceived (packet, swid, outPortId, st, current_ts);
            }
            else if(udph.GetDestinationPort() == probePort+1) {
              // query packet
              onQueryPacketReceived(packet, swid, outPortId, st, current_ts);
            } else if(udph.GetDestinationPort() == probePort+2) {
              // this means we just want to dump the path taken by the packet
              std::cout << "PATH " << swid << " " << outPortId << std::endl;
            }
          packet->AddHeader (udph);
        }

      packet->AddHeader (iph);
    }

#endif

  // std::cout << "[SwitchNode] protocol " << protocol << std::endl;
  // Packet* copy = const_cast<Packet*>(GetPointer(packet));
  // std::cout << "[Q]" << stats.queueOccupancy << " " << stats.timestampMs << std::endl;
  // std::cout << "hey!! \n";
  // #if 1
  //       Packet* copy = packet;
  //       //  Ptr<Packet> p =  ConstCast(copy);
  //         // Packet* p = cop y->GetPointer();
  //         // Ptr<Packet> packet = copy->Ref();
  //         // copy->RemoveHeader(ppp);
  //         // PacketMetadata::ItemIterator metadataIterator = copy->BeginItem();
  //         // PacketMetadata::Item item;
  //         // if(metadataIterator.HasNext()) {
  //         //   item = metadataIterator.Next();
  //         //   std::cout << "well we have some " << item.tid.GetName() << std::endl;
  //         // }
  //         // std::cout << ppp.GetProtocol() << std::endl;
  //         if(protocol == 2048) {
  //           Ipv4Header ip;
  //           copy->RemoveHeader(ip);
  //           if((int)ip.GetProtocol() == 17) {
  //             UdpHeader udp;
  //             copy->RemoveHeader(udp);

  //             if(udp.GetDestinationPort() == 9999) {
  //               // extract probe packet
  //               ProbeHeader ph;
  //               copy->RemoveHeader(ph);
  //               std::vector<ProbePayload> payloads;
  //               for(uint32_t i=0;i<ph.GetCount();i++) {
  //                 ProbePayload p;
  //                 copy->RemoveHeader(p);
  //                 payloads.push_back(p);
  //                 // p.Print(std::cout);
  //               }
  //               ProbePayload newp;
  //               newp.SetSwid(18);
  //               newp.SetTotalPackets(99);
  //               payloads.push_back(newp);
  //               std:: cout << swid << "writing " << payloads.size() << std::endl;
  //               for(ProbePayload p: payloads) {
  //                 copy->AddHeader(p);
  //               }
  //               std:: cout << swid << " done\n";
  //               ph.SetCount(ph.GetCount()+1);
  //                copy->AddHeader(ph);
  //             }
  //              copy->AddHeader(udp);
  //           }
  //            copy->AddHeader(ip);
  //         }
  // #endif
  mutex.Unlock ();
}

} // namespace ns3