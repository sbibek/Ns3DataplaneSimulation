#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"
#include "probe-payload.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ProbePayload");

NS_OBJECT_ENSURE_REGISTERED (ProbePayload);

ProbePayload::ProbePayload ()
  : m_swid (0), m_portId(0), m_maxQueueDepth(0)
{
  NS_LOG_FUNCTION (this);
}


  void ProbePayload::SetSwid(swid_t swid){
     NS_LOG_FUNCTION (this << swid);
     m_swid = swid;

  }
  swid_t ProbePayload::GetSwid(void) const {
     NS_LOG_FUNCTION (this);
      return m_swid;
  }  

  void ProbePayload::SetPortId(portid_t pid) {
     NS_LOG_FUNCTION (this << pid);
     m_portId = pid;
  }

  portid_t ProbePayload::GetPortId(void) const {
     NS_LOG_FUNCTION (this);
     return m_portId;
  }

  void ProbePayload::SetMaxQueueDepth(maxqueue_t maxq) {
    NS_LOG_FUNCTION(this << maxq);
    m_maxQueueDepth = maxq;
  }

  maxqueue_t ProbePayload::GetMaxQueueDepth(void) const {
    return m_maxQueueDepth;
  }


TypeId
ProbePayload::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ProbePayload")
    .SetParent<Header> ()
    .SetGroupName("Applications")
    .AddConstructor<ProbePayload> ()
  ;
  return tid;
}
TypeId
ProbePayload::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void
ProbePayload::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "Swid: " << m_swid << " PortId: " << m_portId;
}
uint32_t
ProbePayload::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return 6;
}

void
ProbePayload::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  i.WriteHtonU16 (m_swid);
  i.WriteHtonU16(m_portId);
  i.WriteHtonU16(m_maxQueueDepth);
}
uint32_t
ProbePayload::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  m_swid = i.ReadNtohU16 ();
  m_portId = i.ReadNtohU16();
  m_maxQueueDepth = i.ReadNtohU16();
  return GetSerializedSize ();
}

} // namespace ns3
