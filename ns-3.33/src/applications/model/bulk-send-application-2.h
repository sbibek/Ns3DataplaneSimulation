/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Georgia Institute of Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: George F. Riley <riley@ece.gatech.edu>
 */

#ifndef BULK_SEND_APPLICATION22_H
#define BULK_SEND_APPLICATION22_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/seq-ts-size-header.h"
#include <unordered_map>

namespace ns3 {

class Address;
class Socket;


typedef struct {
  uint32_t connId;
  Ptr<Socket> m_socket;
  bool m_isConnected = false;
  bool m_sentMetadata = false;
  bool m_transferCompleted = false;
  Time m_transferStartedTime;
  bool m_responseStarted = false;
  Time m_responseStartedTime;

  // total bytes sent so far
  uint64_t m_totBytes = 0;
  Ptr<Packet> m_unsentPacket; //!< Variable to cache unsent packet

  public:
  void updateSocket(Ptr<Socket> socket) {
    this->m_socket = socket;
  }
} OffloadConnection;

/**
 * \ingroup applications
 * \defgroup bulksend BulkSendApplication2
 *
 * This traffic generator simply sends data
 * as fast as possible up to MaxBytes or until
 * the application is stopped (if MaxBytes is
 * zero). Once the lower layer send buffer is
 * filled, it waits until space is free to
 * send more data, essentially keeping a
 * constant flow of data. Only SOCK_STREAM 
 * and SOCK_SEQPACKET sockets are supported. 
 * For example, TCP sockets can be used, but 
 * UDP sockets can not be used.
 */

/**
 * \ingroup bulksend
 *
 * \brief Send as much traffic as possible, trying to fill the bandwidth.
 *
 * This traffic generator simply sends data
 * as fast as possible up to MaxBytes or until
 * the application is stopped (if MaxBytes is
 * zero). Once the lower layer send buffer is
 * filled, it waits until space is free to
 * send more data, essentially keeping a
 * constant flow of data. Only SOCK_STREAM
 * and SOCK_SEQPACKET sockets are supported.
 * For example, TCP sockets can be used, but
 * UDP sockets can not be used.
 *
 * If the attribute "EnableSeqTsSizeHeader" is enabled, the application will
 * use some bytes of the payload to store an header with a sequence number,
 * a timestamp, and the size of the packet sent. Support for extracting 
 * statistics from this header have been added to \c ns3::PacketSink 
 * (enable its "EnableSeqTsSizeHeader" attribute), or users may extract
 * the header via trace sources.
 */
class BulkSendApplication2 : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  BulkSendApplication2 ();

  virtual ~BulkSendApplication2 ();

  /**
   * \brief Set the upper bound for the total number of bytes to send.
   *
   * Once this bound is reached, no more application bytes are sent. If the
   * application is stopped during the simulation and restarted, the 
   * total number of bytes sent is not reset; however, the maxBytes 
   * bound is still effective and the application will continue sending 
   * up to maxBytes. The value zero for maxBytes means that 
   * there is no upper bound; i.e. data is sent until the application 
   * or simulation is stopped.
   *
   * \param maxBytes the upper bound of bytes to send
   */
  void SetMaxBytes (uint64_t maxBytes);

  /**
   * \brief Get the socket this application is attached to.
   * \return pointer to associated socket
   */
  Ptr<Socket> GetSocket (void) const;

protected:
  virtual void DoDispose (void);
private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  /**
   * \brief Send data until the L4 transmission buffer is full.
   * \param from From address
   * \param to To address
   */
  void SendHeader (const Address &from, const Address &to, OffloadConnection* conn);
  void TestSendData (const Address &from, const Address &to, Ptr<Socket> socket, OffloadConnection* conn);

  uint32_t        m_sendSize;     //!< Size of data to send each time
  uint64_t        m_maxBytes;     //!< Limit total number of bytes sent
  TypeId          m_tid;          //!< The type of protocol to use.
  uint32_t        m_seq {0};      //!< Sequence
  bool            m_enableSeqTsSizeHeader {false}; //!< Enable or disable the SeqTsSizeHeader

  static uint32_t connectionId;
  uint16_t m_totalServersToOffload = 1;
  // query sockets for scheduler
  Ptr<Socket> m_querysocket;
  Address m_schedularAddress; //!< Remote peer address
  uint16_t m_queryPort; //!< Remote peer port
  int m_serverSelectionStrategy = 0;

  Time m_waitTime; //!< Packet inter-send time


  // bibek
  // for multiple transfers at the same time, we need to add that ability
  std::unordered_map<void*,OffloadConnection*> m_connections;

  /// Traced Callback: sent packets
  TracedCallback<Ptr<const Packet> > m_txTrace;

  /// Callback for tracing the packet Tx events, includes source, destination,  the packet sent, and header
  TracedCallback<Ptr<const Packet>, const Address &, const Address &, const SeqTsSizeHeader &> m_txTraceWithSeqTsSize;

private:
  /**
   * \brief Connection Succeeded (called by Socket through a callback)
   * \param socket the connected socket
   */
  void ConnectionSucceeded (Ptr<Socket> socket);
  /**
   * \brief Connection Failed (called by Socket through a callback)
   * \param socket the connected socket
   */
  void ConnectionFailed (Ptr<Socket> socket);
  /**
   * \brief Send more data as soon as some has been transmitted.
   */
  void DataSend (Ptr<Socket>, uint32_t); // for socket's SetSendCallback

  // added by bibek
  void ReceivedDataCallback (Ptr<Socket> socket);
  void NormalCloseCallback (Ptr<Socket> socket);
  void ErrorCloseCallback (Ptr<Socket> socket);


  void QueryResponseHandler(Ptr<Socket>);
  void SendQuery(void);
  void QuerySequence(void);
  void DataTransferSequence(std::vector<int> nodes);

  // for multiple connections
  void InitTransferN(Address& address);

};

} // namespace ns3

#endif /* BULK_SEND_APPLICATION_H */
