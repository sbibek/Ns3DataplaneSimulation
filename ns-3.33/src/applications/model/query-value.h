#ifndef QRESVAL_H
#define QRESVAL_H

#include "ns3/header.h"
#include "ns3/nstime.h"

namespace ns3 {

  typedef struct PayloadValue {

  } Payloadvalue;

class QueryValue : public Header
{
public:
  QueryValue ();

  void SetSwid(uint16_t);
  uint16_t GetSwid(void) const;  

  void SetValue(uint32_t);
  uint32_t GetValue(void) const;

  static TypeId GetTypeId (void);

  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

private:
  uint16_t m_swid;
  uint32_t m_value;

};

} // namespace ns3

#endif 
