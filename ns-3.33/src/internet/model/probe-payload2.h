#ifndef PROBE_PAYLOAD_H
#define PROBE_PAYLOAD_H

#include "ns3/header.h"
#include "ns3/nstime.h"

typedef uint16_t swid_t;
typedef uint16_t portid_t;
typedef uint16_t maxqueue_t;

namespace ns3 {

class ProbePayload2 : public Header
{
public:
  ProbePayload2 ();


  void SetPortId(portid_t);
  portid_t GetPortId(void) const;

  void SetMaxQueueDepth(maxqueue_t);
  maxqueue_t GetMaxQueueDepth(void) const; 

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

private:
  portid_t m_portId;
  maxqueue_t m_maxQueueDepth;
};


} // namespace ns3

#endif 
