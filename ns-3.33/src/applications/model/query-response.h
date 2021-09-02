#ifndef QRES_H
#define QRES_H

#include "ns3/header.h"
#include "ns3/nstime.h"

namespace ns3 {

class QueryResponse : public Header
{
public:
  QueryResponse ();

  void SetCount (uint16_t);
  uint16_t GetCount (void) const;

  static TypeId GetTypeId (void);

  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

private:
  uint16_t m_count;
};

} // namespace ns3

#endif
