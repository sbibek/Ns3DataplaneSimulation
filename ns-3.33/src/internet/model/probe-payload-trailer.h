#ifndef PROBE_PAYLOAD_T_H
#define PROBE_PAYLOAD_T_H

#include "ns3/trailer.h"
#include "ns3/nstime.h"

namespace ns3 {

class ProbePayloadTrailer : public Trailer
{
public:
  ProbePayloadTrailer ();

  void SetSwid(uint16_t);
  uint16_t GetSwid(void) const;  

  void SetTotalPackets(uint32_t);
  uint32_t GetTotalPackets(void) const;

  void SetGlobalHopLatency(uint32_t);
  uint32_t GetGlobalHopLatency(void) const;

  void SetGlobalQueueDepth(uint16_t);
  uint16_t GetGlobalQueueDepth(void) const;

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
  uint16_t m_swid; // switch id
  uint32_t m_global_hop_latency;
  uint16_t m_global_qdepth;
  uint64_t m_totalPackets; 
};

} // namespace ns3

#endif 
