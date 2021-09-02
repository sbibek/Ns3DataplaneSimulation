#ifndef PRB_SND2_H 
#define PRB_SND2_H 

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"

namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup ProbeApp2server
 *
 * \brief A Udp client. Sends UDP packet carrying sequence number and time stamp
 *  in their payloads
 *
 */
class ProbeApp2 : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  ProbeApp2 ();

  virtual ~ProbeApp2 ();

  /**
   * \brief set the remote address and port
   * \param ip remote IP address
   * \param port remote port
   */
  void SetRemote (Address ip, uint16_t port);
  /**
   * \brief set the remote address
   * \param addr remote address
   */
  void SetRemote (Address addr);

  /**
   * \return the total bytes sent by this app
   */
  uint64_t GetTotalTx () const;

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  /**
   * \brief Send a packet
   */
  void Send (void);
  void SendProbe(void);
  void SendQuery(void);

  void QueryResponseHandler(Ptr<Socket>);

  uint32_t m_count; //!< Maximum number of packets the application will send
  Time m_interval; //!< Packet inter-send time
  uint32_t m_size; //!< Size of the sent packet (including the SeqTsHeader)

  uint32_t m_sent; //!< Counter for sent packets
  uint64_t m_totalTx; //!< Total bytes sent
  Ptr<Socket> m_socket; //!< Socket
  Ptr<Socket> m_querysocket;
  Address m_peerAddress; //!< Remote peer address
  uint16_t m_peerPort; //!< Remote peer port
  EventId m_sendEvent; //!< Event to send the next packet

  uint32_t count = 0;

};

} // namespace ns3

#endif /* UDP_CLIENT_H */
