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

#include <stdlib.h>
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

#include "ns3/probe-listener.h"
#include "query-header.h"
#include "query-response.h"
#include "query-value.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ProbeListener");

NS_OBJECT_ENSURE_REGISTERED (ProbeListener);

TypeId
ProbeListener::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::ProbeListener")
          .SetParent<Application> ()
          .SetGroupName ("Applications")
          .AddConstructor<ProbeListener> ()
          .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                         UintegerValue (100), MakeUintegerAccessor (&ProbeListener::m_port),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("PacketWindowSize",
                         "The size of the window used to compute the packet loss. This value "
                         "should be a multiple of 8.",
                         UintegerValue (32),
                         MakeUintegerAccessor (&ProbeListener::GetPacketWindowSize,
                                               &ProbeListener::SetPacketWindowSize),
                         MakeUintegerChecker<uint16_t> (8, 256))
          .AddTraceSource ("Rx", "A packet has been received",
                           MakeTraceSourceAccessor (&ProbeListener::m_rxTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("RxWithAddresses", "A packet has been received",
                           MakeTraceSourceAccessor (&ProbeListener::m_rxTraceWithAddresses),
                           "ns3::Packet::TwoAddressTracedCallback");
  return tid;
}

ProbeListener::ProbeListener () : m_lossCounter (0)
{
  NS_LOG_FUNCTION (this);
  m_received = 0;
}

ProbeListener::~ProbeListener ()
{
  NS_LOG_FUNCTION (this);
}

uint16_t
ProbeListener::GetPacketWindowSize () const
{
  NS_LOG_FUNCTION (this);
  return m_lossCounter.GetBitMapSize ();
}

void
ProbeListener::SetPacketWindowSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_lossCounter.SetBitMapSize (size);
}

uint32_t
ProbeListener::GetLost (void) const
{
  NS_LOG_FUNCTION (this);
  return m_lossCounter.GetLost ();
}

uint64_t
ProbeListener::GetReceived (void) const
{
  NS_LOG_FUNCTION (this);
  return m_received;
}

void
ProbeListener::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
ProbeListener::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  // NS_LOG_INFO("HERE");

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      if (m_socket->Bind (local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
      else
        {
          NS_LOG_INFO ("Listening @ " << m_port);
        }

      // now we open up the query socket
      m_querysocket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress querylocal = InetSocketAddress (Ipv4Address::GetAny (), m_port + 1);
      if (m_querysocket->Bind (querylocal) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind query UDP socket");
        }
    }

  m_socket->SetRecvCallback (MakeCallback (&ProbeListener::HandleRead_Probe, this));
  m_querysocket->SetRecvCallback (MakeCallback (&ProbeListener::HandleRead_Query, this));
}

void
ProbeListener::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());
      m_querysocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());
    }
}

void
ProbeListener::HandleRead_Probe (Ptr<Socket> socket)
{
  NS_LOG_INFO("\n[P-LISTNR] receied " << m_received << " packets\n");
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  while ((packet = socket->RecvFrom (from)))
    {
      // std::cout << "packet size = " << packet->GetSize() << "\n"; 
      socket->GetSockName (localAddress);
      m_rxTrace (packet);
      m_rxTraceWithAddresses (packet, from, localAddress);
      #if 1 
      if (packet->GetSize () > 0)
        {
          // NS_LOG_INFO (InetSocketAddress::ConvertFrom (from).GetIpv4 ());
          ProbeHeader header;
          packet->RemoveHeader (header);
          // std::cout << "Got Count " << header.GetCount () << "size" << packet->GetSize() << std::endl;
          std::vector<ProbePayload> payload;
          for (uint32_t i = 0; i < header.GetCount (); i++)
            {
              ProbePayload load;
              packet->RemoveHeader (load);
              payload.push_back (load);
            }
          // now send this to the handler
          HandleProbeData (header, payload, from);
          m_received++;
        }
        #endif
    }
}

void
ProbeListener::HandleProbeData (ProbeHeader &header, std::vector<ProbePayload> &payload,
                                Address &from)
{
  NS_LOG_INFO ("[Rx Probe] "
               << "payload.count=" << header.GetCount ());
  for (ProbePayload p : payload)
    {
      NS_LOG_INFO ("   swid=" << p.GetSwid () << " portId=" << p.GetPortId() << " max_queue="<<p.GetMaxQueueDepth());
    }
}

void
ProbeListener::HandleRead_Query (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  while ((packet = socket->RecvFrom (from)))
    {
      socket->GetSockName (localAddress);
      if (packet->GetSize () > 0)
        {
          // means this is valid query packet
          QueryHeader qheader;
          packet->RemoveHeader (qheader);

          // send it to the query handler
          HandleQuery(qheader, from, socket);
        }
    }
}

void
ProbeListener::HandleQuery (QueryHeader &header, Address &from, Ptr<Socket> socket)
{
  NS_LOG_INFO("[Rx Query] swid=" << header.GetNodeId());
  // now we send a dummy response
  QueryResponse response;
  int random = rand() % 8 + 1;
  response.SetCount(random);
  Ptr<Packet> packet = Create<Packet>(response.GetSerializedSize());

  NS_LOG_INFO("[Tx Query Response (listener)] count=" << response.GetCount());
  for(int i=0;i<random;i++){
    QueryValue value;
    value.SetNodeId(i+1);
    value.SetValue(rand()%20+10);
    packet->AddHeader(value);

    NS_LOG_INFO("   swid=" << value.GetNodeId() << " value="<<value.GetValue());
  }

  packet->AddHeader(response);


  if(socket->SendTo(packet, 0, from) >= 0) {
    // NS_LOG_INFO("[Tx Query Response (listener)] count=" << response.GetCount());
  }


}

} // Namespace ns3
