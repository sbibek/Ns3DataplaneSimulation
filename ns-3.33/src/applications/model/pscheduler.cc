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
 * Foundation, Inc., 59 Temple PlConfig::Connect("/NodeList/100/DeviceList/99/$ns3::CsmaNetDevice/TxQueue/PacketsInQueue", MakeCallback(&packetsInQueueTrace))oace, Suite 330, Boston, MA  02111-1307  USA
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

#include "pscheduler.h"
#include "query-header.h"
#include "query-response.h"
#include "query-value.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PScheduler");

NS_OBJECT_ENSURE_REGISTERED (PScheduler);

TypeId
PScheduler::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::PScheduler")
          .SetParent<Application> ()
          .SetGroupName ("Applications")
          .AddConstructor<PScheduler> ()
          .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                         UintegerValue (100), MakeUintegerAccessor (&PScheduler::m_port),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("PacketWindowSize",
                         "The size of the window used to compute the packet loss. This value "
                         "should be a multiple of 8.",
                         UintegerValue (32),
                         MakeUintegerAccessor (&PScheduler::GetPacketWindowSize,
                                               &PScheduler::SetPacketWindowSize),
                         MakeUintegerChecker<uint16_t> (8, 256))
          .AddTraceSource ("Rx", "A packet has been received",
                           MakeTraceSourceAccessor (&PScheduler::m_rxTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("RxWithAddresses", "A packet has been received",
                           MakeTraceSourceAccessor (&PScheduler::m_rxTraceWithAddresses),
                           "ns3::Packet::TwoAddressTracedCallback");
  return tid;
}

PScheduler::PScheduler () : m_lossCounter (0)
{
  NS_LOG_FUNCTION (this);
  m_received = 0;
}

PScheduler::~PScheduler ()
{
  NS_LOG_FUNCTION (this);
}

uint16_t
PScheduler::GetPacketWindowSize () const
{
  NS_LOG_FUNCTION (this);
  return m_lossCounter.GetBitMapSize ();
}

void
PScheduler::SetPacketWindowSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_lossCounter.SetBitMapSize (size);
}

uint32_t
PScheduler::GetLost (void) const
{
  NS_LOG_FUNCTION (this);
  return m_lossCounter.GetLost ();
}

uint64_t
PScheduler::GetReceived (void) const
{
  NS_LOG_FUNCTION (this);
  return m_received;
}

void
PScheduler::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
PScheduler::StartApplication (void)
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
      InetSocketAddress querylocal = InetSocketAddress (Ipv4Address::GetAny (), 8080);
      if (m_querysocket->Bind (querylocal) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind query UDP socket");
        }
    }

  m_socket->SetRecvCallback (MakeCallback (&PScheduler::HandleRead_Probe, this));
  m_querysocket->SetRecvCallback (MakeCallback (&PScheduler::HandleRead_Query, this));
}

void
PScheduler::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());
      m_querysocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());
    }
}

void
PScheduler::HandleRead_Probe (Ptr<Socket> socket)
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
      // std:: cout << "probe packet \n";
      #if 1 
      if (packet->GetSize () > 0)
        {
          // NS_LOG_INFO (InetSocketAddress::ConvertFrom (from).GetIpv4 ());
          ProbeHeader2 header;
          packet->RemoveHeader (header);
          // std::cout << "From swid=" << header.GetSwid () << " count=" << header.GetCount() << std::endl;
          std::vector<ProbePayload2> payload;
          for (uint32_t i = 0; i < header.GetCount (); i++)
            {
              ProbePayload2 load;
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
PScheduler::HandleProbeData (ProbeHeader2 &header, std::vector<ProbePayload2> &payload,
                                Address &from)
{
  // NS_LOG_INFO ("[Rx Probe] "
  //              << "payload.count=" << header.GetCount ());
  store.onSwitchUpdate(header, payload);
  // store.tracePath(0,39);
  //  store.log(header.GetSwid());
  // for (ProbePayload2 p : payload)
  //   {
  //     // std::cout << "(p) " << p.GetPortId() << " = (q) " << p.GetMaxQueueDepth() << std::endl;
  //   }
}

void
PScheduler::HandleRead_Query (Ptr<Socket> socket)
{
  std::cout << "Query << " << std::endl;
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
PScheduler::HandleQuery (QueryHeader &header, Address &from, Ptr<Socket> socket)
{
  NS_LOG_DEBUG("[Rx Query] swid=" << header.GetNodeId());

  QueryResponse response;
  Ptr<Packet> packet = Create<Packet>(response.GetSerializedSize());

  #if 0 
  std::unordered_map<int, int> results = store.tracePath(header.GetNodeId());
  for(auto x: results) { 
    QueryValue v;
    v.SetNodeId(x.first);
    v.SetValue(x.second);
    packet->AddHeader(v);
  }
  #else
    std::vector<std::tuple<int,int>> results = schedulingKernel(store.tracePathWithTuple(header.GetNodeId()));
    std::vector<std::tuple<int,int>>::iterator i = results.end();
    while(i != results.begin()) {
      --i;
      QueryValue v;
      std::tuple<int, int> t = *i;
      v.SetNodeId(std::get<1>(t));
      v.SetValue(std::get<0>(t));
      packet->AddHeader(v);
    }
  #endif

  response.SetCount(results.size());



  // NS_LOG_INFO("[Tx Query Response (listener)] count=" << response.GetCount());
  // for(int i=0;i<random;i++){
  //   QueryValue value;
  //   value.SetNodeId(i+1);
  //   value.SetValue(rand()%20+10);
  //   packet->AddHeader(value);

  //   NS_LOG_INFO("   swid=" << value.GetNodeId() << " value="<<value.GetValue());
  // }

  packet->AddHeader(response);

  if(socket->SendTo(packet, 0, from) >= 0) {
    // NS_LOG_INFO("[Tx Query Response (listener)] count=" << response.GetCount());
  }


}


std::vector<std::tuple<int,int>> 
PScheduler::schedulingKernel(std::vector<std::tuple<int,int>> info) {
  // write the kernel here/
  // important is that the first element is actually the effectiuve queue and 2nd is the node id
  // must return it in the same direction, sorted by first key
  return info;
}

} // Namespace ns3
