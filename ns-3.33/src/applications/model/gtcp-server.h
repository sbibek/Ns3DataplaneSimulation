/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007,2008,2009 INRIA, UDCAST
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
 * Author: Amine Ismail <amine.ismail@sophia.inria.fr>
 *                      <amine.ismail@udcast.com>
 *
 */

#ifndef GTCP_C_H
#define GTCP_C_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/traced-callback.h"
#include "packet-loss-counter.h"
#include <unordered_map>

namespace ns3 {


typedef struct {
  uint32_t m_offloadSessionId;
  uint64_t m_connId;
  Ptr<Socket> m_socket;
  uint64_t m_totalBytesExpected = 2147483647;
  uint64_t m_totalBulkTransferRcvdBytes = 0;
  uint64_t m_totalResponseBytes = 1;
  uint64_t m_totalResponseBytesSent = 0;
  bool m_metadataReceived = false;
  bool m_responseCycle = false;

  Time m_receiveDataStarted;
  bool m_responseStarted = false;
  Time m_responseStartTime;
} GtcpConnection;

class GtcpServer : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  GtcpServer ();
  virtual ~GtcpServer ();
;

  /**
   * \brief Returns the size of the window used for checking loss.
   * \return the size of the window used for checking loss.
   */
  uint16_t GetPacketWindowSize () const;

  /**
   * \brief Set the size of the window used for checking loss. This value should
   *  be a multiple of 8
   * \param size the size of the window used for checking loss. This value should
   *  be a multiple of 8
   */
  void SetPacketWindowSize (uint16_t size);

  // socket callbacks
  bool ConnectionRequestCallback (Ptr<Socket>   socket, const Address &address);
  void NewConnectionCreatedCallback (Ptr<Socket>    socket, const Address  &address);
  void NormalCloseCallback (Ptr<Socket> socket);
  void ErrorCloseCallback (Ptr<Socket> socket);
  void ReceivedDataCallback (Ptr<Socket> socket);
  void SendCallback (Ptr<Socket> socket, uint32_t availableBufferSize);

  void test(Ptr<Socket>);
  void ResponseCycle(Ptr<Socket>);


protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  Ptr<Socket> m_socket;
  uint16_t m_port; 
  Time m_waitTime;

  uint64_t m_totalResponseBytes = 1;

  std::unordered_map<void*, GtcpConnection*> m_connections;

};

} // namespace ns3

#endif /* PROBE_LISTENER_H */
