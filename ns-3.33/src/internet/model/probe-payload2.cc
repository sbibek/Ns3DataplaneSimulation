#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"
#include "probe-payload2.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ProbePayload2");

NS_OBJECT_ENSURE_REGISTERED (ProbePayload2);

ProbePayload2::ProbePayload2 ()
  : m_portId(0), m_maxQueueDepth(0)
{
  NS_LOG_FUNCTION (this);
}

  void ProbePayload2::SetPortId(portid_t pid) {
     NS_LOG_FUNCTION (this << pid);
     m_portId = pid;
  }

  portid_t ProbePayload2::GetPortId(void) const {
     NS_LOG_FUNCTION (this);
     return m_portId;
  }

  void ProbePayload2::SetMaxQueueDepth(maxqueue_t maxq) {
    NS_LOG_FUNCTION(this << maxq);
    m_maxQueueDepth = maxq;
  }

  maxqueue_t ProbePayload2::GetMaxQueueDepth(void) const {
    return m_maxQueueDepth;
  }


TypeId
ProbePayload2::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ProbePayload2")
    .SetParent<Header> ()
    .SetGroupName("Applications")
    .AddConstructor<ProbePayload2> ()
  ;
  return tid;
}
TypeId
ProbePayload2::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void
ProbePayload2::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << " PortId: " << m_portId;
}
uint32_t
ProbePayload2::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return 4;
}

void
ProbePayload2::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  i.WriteHtonU16(m_portId);
  i.WriteHtonU16(m_maxQueueDepth);
}
uint32_t
ProbePayload2::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  m_portId = i.ReadNtohU16();
  m_maxQueueDepth = i.ReadNtohU16();
  return GetSerializedSize ();
}

} // namespace ns3
