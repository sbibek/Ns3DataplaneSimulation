/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *  Copyright (c) 2007,2008,2009 INRIA, UDCAST
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
 */

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "packet-loss-counter.h"
#include <ns3/tcp-socket.h>
#include <ns3/tcp-socket-factory.h>

#include "ns3/probe-header.h"
#include "ns3/probe-payload.h"
#include "gtcp-server.h"
#include "ns3/data-transfer-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GtcpServer");

NS_OBJECT_ENSURE_REGISTERED (GtcpServer);

TypeId
GtcpServer::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::GtcpServer")
          .SetParent<Application> ()
          .SetGroupName ("Applications")
          .AddConstructor<GtcpServer> ()
          .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                         UintegerValue (100), MakeUintegerAccessor (&GtcpServer::m_port),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("TotalResponseBytes", "Bytes of data to send as response.",
                         UintegerValue (512), MakeUintegerAccessor (&GtcpServer::m_totalResponseBytes),
                         MakeUintegerChecker<uint64_t> ())
          .AddAttribute ("WaitPeriod", "Amount of time between send and results recv", TimeValue (Seconds (2.0)),
                       MakeTimeAccessor (&GtcpServer::m_waitTime), MakeTimeChecker ());
  return tid;
}

GtcpServer::GtcpServer (): m_responseCycle(false)
{
  NS_LOG_FUNCTION (this);
}

GtcpServer::~GtcpServer ()
{
  NS_LOG_FUNCTION (this);
}

void
GtcpServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
GtcpServer::StartApplication (void)
{
  // NS_LOG_INFO("GTCP SERVER NOT IMPLEMENTED!!");
  const TypeId tcpSocketTid = TcpSocket::GetTypeId ();
  m_socket = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId ());
  InetSocketAddress localf = InetSocketAddress (Ipv4Address::GetAny (), m_port);
  int ret = m_socket->Bind (localf);
  if (ret == -1)
    {
      NS_LOG_DEBUG (this << " [GTCP] Unable to bind the socket to given address " << ret);
    }
  else
    {
      NS_LOG_INFO (this << " [GTCP] Binding successful to the given address");
    }

  ret = m_socket->Listen ();
  if (ret == -1)
    {
      NS_LOG_DEBUG (this << " [GTCP] Listen failed " << ret);
    }
  else
    {
      NS_LOG_INFO (this << " [GTCP] Listening @ " << m_port);
    }

  m_socket->SetAcceptCallback (MakeCallback (&GtcpServer::ConnectionRequestCallback, this),
                               MakeCallback (&GtcpServer::NewConnectionCreatedCallback, this));
  m_socket->SetCloseCallbacks (MakeCallback (&GtcpServer::NormalCloseCallback, this),
                               MakeCallback (&GtcpServer::ErrorCloseCallback, this));
  m_socket->SetRecvCallback (MakeCallback (&GtcpServer::ReceivedDataCallback, this));
  m_socket->SetSendCallback (MakeCallback (&GtcpServer::SendCallback, this));
}

void
GtcpServer::StopApplication ()
{
}

bool
GtcpServer::ConnectionRequestCallback (Ptr<Socket> socket, const Address &address)
{
  // unconditionally acept the request
  return true;
}

void
GtcpServer::NewConnectionCreatedCallback (Ptr<Socket> socket, const Address &address)
{
  NS_LOG_INFO (this << " [GTCP] New connection received from "
                    << InetSocketAddress::ConvertFrom (address).GetIpv4 ());
  socket->SetCloseCallbacks (MakeCallback (&GtcpServer::NormalCloseCallback, this),
                             MakeCallback (&GtcpServer::ErrorCloseCallback, this));
  socket->SetRecvCallback (MakeCallback (&GtcpServer::ReceivedDataCallback, this));
  socket->SetSendCallback (MakeCallback (&GtcpServer::SendCallback, this));
  // ReceivedDataCallback (socket);
  // Simulator::Schedule (Seconds(1), &GtcpServer::test, this, socket);
}

void
GtcpServer::NormalCloseCallback (Ptr<Socket> socket)
{
  NS_LOG_INFO (this << " [GTCP] Normal Close Callback");
}

void
GtcpServer::ErrorCloseCallback (Ptr<Socket> socket)
{
  NS_LOG_INFO (this << " [GTCP] Error Close Callback");
}

void
GtcpServer::ReceivedDataCallback (Ptr<Socket> socket)
{
  // NS_LOG_INFO (this << " [GTCP] Received Data Callback");
  Ptr<Packet> packet;
  Address from;

  while ((packet = socket->RecvFrom (from)) && m_totalBulkTransferRcvdBytes < m_totalBytesExpected )
    {
      if (packet->GetSize () == 0)
        break;

       m_totalBulkTransferRcvdBytes += packet->GetSize();

      NS_LOG_INFO (this << " [GTCP] A packet of " << packet->GetSize () << " bytes"
                        << " received from " << InetSocketAddress::ConvertFrom (from).GetIpv4 ()
                        << " port " << InetSocketAddress::ConvertFrom (from).GetPort () 
                        << "Total so far = " << m_totalBulkTransferRcvdBytes <<" bytes");

      if(!m_metadataReceived) {
        m_receiveDataStarted = Simulator::Now();
        // lets extract metadata
        DataTransferHeader hdr;
        packet->RemoveHeader(hdr);
        m_totalBytesExpected = hdr.GetTotalBytesFollowingThis();
        m_metadataReceived = true;
        NS_LOG_INFO(this << "Got metadata, total bytes expected = " <<  hdr.GetTotalBytesFollowingThis());
      }

    }


    if(!m_responseCycle && m_totalBulkTransferRcvdBytes >= m_totalBytesExpected) {
      m_responseCycle = true;
      Time elapsed = Simulator::Now() - m_receiveDataStarted;
      // NS_LOG_DEBUG("[GTCP] Took " << elapsed.GetMilliSeconds() << "ms to receive all the data");
      logger("resut").add("TRANSFERED_DATA_RECEIVED_TIME_MS", elapsed.GetMilliSeconds()).log();
      // now we simulate sending response
      // NS_LOG_DEBUG("[GTCP] Scheduling the response after " << m_waitTime.GetSeconds() << "s");
      logger("result").add("SIMULATED_PROCESSING_TIME_S", m_waitTime.GetSeconds()).log();
       Simulator::Schedule (m_waitTime, &GtcpServer::ResponseCycle, this, socket);
      // test(socket);
    }

  // socket->Close();
}


void GtcpServer::ResponseCycle(Ptr<Socket> s) {
      if(m_responseStarted == false) {
          m_responseStarted = true;
          m_responseStartTime = Simulator::Now();
      }

      uint64_t toSend = std::min((uint64_t)512, m_totalResponseBytes-m_totalResponseBytesSent);
      NS_LOG_INFO("[GTCP] Response cycle Remaining=" << toSend << " bytes");

      Ptr<Packet> load = Create<Packet>(toSend);
      int actual = s->Send(load);

      if(actual == -1) {
        // this is bad, lookslike no buffer, so schedule at later time
        NS_LOG_INFO("Looks like buffer is full, so rescheduling after 5ms");
        Simulator::Schedule (MilliSeconds(5), &GtcpServer::ResponseCycle, this, s);
        return;
      }

       m_totalResponseBytesSent += toSend;
      if(m_totalResponseBytesSent < m_totalResponseBytes) {
        // means need to reschedule it
        Simulator::Schedule (Seconds(0), &GtcpServer::ResponseCycle, this, s);
      } else {
        Time elapsed = Simulator::Now() - m_responseStartTime;
        // NS_LOG_DEBUG("[GTPC] Sending response took " << elapsed.GetMilliSeconds() << "ms" );
        logger("result").add("RESPONSE_SEND_TIME_MS", elapsed.GetMilliSeconds()).log();
        // means the response is complete
        s->Close();
      }
}

void GtcpServer::test(Ptr<Socket> s) {
  NS_LOG_INFO("Sending Test Data");
  ProbeHeader hdr;
  hdr.SetCount(111);
  Ptr<Packet> pkt = Create<Packet>(hdr.GetSerializedSize());
  pkt->AddHeader(hdr);
  s->Send(pkt);
}

void
GtcpServer::SendCallback (Ptr<Socket> socket, uint32_t availableBufferSize)
{
  // NS_LOG_INFO (this << " [GTCP] Send Callback");
  
}

} // Namespace ns3
