#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"
#include "query-response.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QueryResponse");

NS_OBJECT_ENSURE_REGISTERED (QueryResponse);

QueryResponse::QueryResponse ()
  : m_count (0)
{
  NS_LOG_FUNCTION (this);
}


  void QueryResponse::SetCount(uint16_t count){
     NS_LOG_FUNCTION (this << count);
     m_count = count;

  }
  uint16_t QueryResponse::GetCount(void) const {
     NS_LOG_FUNCTION (this);
      return m_count;
  }  


TypeId
QueryResponse::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::QueryResponse")
    .SetParent<Header> ()
    .SetGroupName("Applications")
    .AddConstructor<QueryResponse> ()
  ;
  return tid;
}
TypeId
QueryResponse::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void
QueryResponse::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "Count: " << m_count;
}
uint32_t
QueryResponse::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return 2;
}

void
QueryResponse::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  i.WriteHtonU16 (m_count);
}
uint32_t
QueryResponse::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  m_count = i.ReadNtohU16 ();
  return GetSerializedSize ();
}

} // namespace ns3
