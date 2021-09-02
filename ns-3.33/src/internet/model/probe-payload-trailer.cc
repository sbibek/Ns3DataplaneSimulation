#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"
#include "probe-payload-trailer.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ProbePayloadTrailer");

NS_OBJECT_ENSURE_REGISTERED (ProbePayloadTrailer);

ProbePayloadTrailer::ProbePayloadTrailer ()
  : m_swid (0), m_totalPackets(0)
{
  NS_LOG_FUNCTION (this);
}


  void ProbePayloadTrailer::SetSwid(uint16_t swid){
     NS_LOG_FUNCTION (this << swid);
     m_swid = swid;

  }
  uint16_t ProbePayloadTrailer::GetSwid(void) const {
     NS_LOG_FUNCTION (this);
      return m_swid;
  }  

  void ProbePayloadTrailer::SetTotalPackets(uint32_t totalPackets) {
     NS_LOG_FUNCTION (this << totalPackets);
     m_totalPackets = totalPackets;
  }
  uint32_t ProbePayloadTrailer::GetTotalPackets(void) const {
     NS_LOG_FUNCTION (this);
     return m_totalPackets;
  }

  void ProbePayloadTrailer::SetGlobalHopLatency(uint32_t latency) {
      m_global_hop_latency = latency;
  }

  uint32_t ProbePayloadTrailer::GetGlobalHopLatency(void) const {
    return m_global_hop_latency;
  }

  void ProbePayloadTrailer::SetGlobalQueueDepth(uint16_t qdepth) {
    m_global_qdepth = qdepth;
  }

  uint16_t ProbePayloadTrailer::GetGlobalQueueDepth(void) const {
    return m_global_qdepth;
  }

TypeId
ProbePayloadTrailer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ProbePayloadTrailer")
    .SetParent<Header> ()
    .SetGroupName("Applications")
    .AddConstructor<ProbePayloadTrailer> ()
  ;
  return tid;
}
TypeId
ProbePayloadTrailer::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void
ProbePayloadTrailer::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "Swid: " << m_swid << " Total packets: " << m_totalPackets;
}
uint32_t
ProbePayloadTrailer::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return 2+4+2+8;
}

void
ProbePayloadTrailer::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  i.WriteHtonU16 (m_swid);
  i.WriteHtonU32(m_global_hop_latency);
  i.WriteHtonU16(m_global_qdepth);
  i.WriteHtonU64(m_totalPackets);
}
uint32_t
ProbePayloadTrailer::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  m_swid = i.ReadNtohU16 ();
  m_global_hop_latency = i.ReadNtohU32();
  m_global_qdepth = i.ReadNtohU16();
  m_totalPackets = i.ReadNtohU64();
  return GetSerializedSize ();
}

} // namespace ns3
