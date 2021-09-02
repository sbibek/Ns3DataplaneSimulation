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
#include "sim-server.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SimServer");

NS_OBJECT_ENSURE_REGISTERED (SimServer);

TypeId
SimServer::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::SimServer")
          .SetParent<Application> ()
          .SetGroupName ("Applications")
          .AddConstructor<SimServer> ()
          .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                         UintegerValue (100), MakeUintegerAccessor (&SimServer::m_port),
                         MakeUintegerChecker<uint16_t> ());
  return tid;
}

SimServer::SimServer ()
{
  NS_LOG_FUNCTION (this);
}

SimServer::~SimServer ()
{
  NS_LOG_FUNCTION (this);
}

void
SimServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
SimServer::StartApplication (void)
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

  m_socket->SetAcceptCallback (MakeCallback (&SimServer::ConnectionRequestCallback, this),
                               MakeCallback (&SimServer::NewConnectionCreatedCallback, this));
  m_socket->SetCloseCallbacks (MakeCallback (&SimServer::NormalCloseCallback, this),
                               MakeCallback (&SimServer::ErrorCloseCallback, this));
  m_socket->SetRecvCallback (MakeCallback (&SimServer::ReceivedDataCallback, this));
  // m_socket->SetSendCallback (MakeCallback (&SimServer::SendCallback, this));
}

void
SimServer::StopApplication ()
{
}

bool
SimServer::ConnectionRequestCallback (Ptr<Socket> socket, const Address &address)
{
  // unconditionally acept the request
  return true;
}

void
SimServer::NewConnectionCreatedCallback (Ptr<Socket> socket, const Address &address)
{
  NS_LOG_INFO (this << " [GTCP] New connection received from "
                    << InetSocketAddress::ConvertFrom (address).GetIpv4 ());
  socket->SetCloseCallbacks (MakeCallback (&SimServer::NormalCloseCallback, this),
                             MakeCallback (&SimServer::ErrorCloseCallback, this));
  socket->SetRecvCallback (MakeCallback (&SimServer::ReceivedDataCallback, this));
  // socket->SetSendCallback (MakeCallback (&SimServer::SendCallback, this));
  ReceivedDataCallback (socket);
}

void
SimServer::NormalCloseCallback (Ptr<Socket> socket)
{
  NS_LOG_INFO (this << " [GTCP] Normal Close Callback");
}

void
SimServer::ErrorCloseCallback (Ptr<Socket> socket)
{
  NS_LOG_INFO (this << " [GTCP] Error Close Callback");
}

void
SimServer::ReceivedDataCallback (Ptr<Socket> socket)
{
  NS_LOG_INFO (this << " [GTCP] Received Data Callback");

  Ptr<Packet> packet;
  Address from;

  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
        break;
      NS_LOG_INFO (this << " [GTCP] A packet of " << packet->GetSize () << " bytes"
                        << " received from " << InetSocketAddress::ConvertFrom (from).GetIpv4 ()
                        << " port " << InetSocketAddress::ConvertFrom (from).GetPort () << " / "
                        << InetSocketAddress::ConvertFrom (from));
      ProbeHeader header;
      packet->RemoveHeader (header);

      NS_LOG_INFO (this << " [GTCP] Header count = " << header.GetCount ());

      // header.SetCount (header.GetCount () + 1);
      // packet->AddHeader (header);

      // Simulator::ScheduleWithContext(socket, Seconds(5), &Socket::Send, header );

    // Simulator::Schedule (Seconds(4), &SimServer::test, this, socket);

    //   socket->Send (packet);
    }

  // socket->Close();
}


// void
// SimServer::SendCallback (Ptr<Socket> socket, uint32_t availableBufferSize)
// {
//   // NS_LOG_INFO (this << " [GTCP] Send Callback");
  
// }

} // Namespace ns3
