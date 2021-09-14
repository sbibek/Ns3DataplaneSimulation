#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"
#include "query-value.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QueryValue");

NS_OBJECT_ENSURE_REGISTERED (QueryValue);

QueryValue::QueryValue () : m_nodeId (0), m_value (0)
{
  NS_LOG_FUNCTION (this);
}

void
QueryValue::SetNodeId (uint16_t swid)
{
  NS_LOG_FUNCTION (this << swid);
  m_nodeId = swid;
}
uint16_t
QueryValue::GetNodeId (void) const
{
  NS_LOG_FUNCTION (this);
  return m_nodeId;
}

void
QueryValue::SetValue (uint32_t value)
{
  m_value = value;
}

uint32_t
QueryValue::GetValue (void) const
{
  return m_value;
}

TypeId
QueryValue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::QueryValue")
                          .SetParent<Header> ()
                          .SetGroupName ("Applications")
                          .AddConstructor<QueryValue> ();
  return tid;
}
TypeId
QueryValue::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void
QueryValue::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "Swid: " << m_nodeId << " Value: " << m_value;
}

uint32_t
QueryValue::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return 2 + 4;
}

void
QueryValue::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  i.WriteHtonU16 (m_nodeId);
  i.WriteHtonU32(m_value);
}
uint32_t
QueryValue::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  m_nodeId = i.ReadNtohU16 ();
  m_value = i.ReadNtohU32();
  return GetSerializedSize ();
}

} // namespace ns3
