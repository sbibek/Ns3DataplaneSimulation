/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Gustavo Carneiro  <gjc@inescporto.pt>
 */
#include "switch-net-device.h"
#include "switch-node.h"
#include "ns3/channel.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SwitchNetDevice");

NS_OBJECT_ENSURE_REGISTERED (SwitchNetDevice);


TypeId
SwitchNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SwitchNetDevice")
    .SetParent<NetDevice> ()
    .SetGroupName("Bridge")
    .AddConstructor<SwitchNetDevice> ()
    .AddAttribute ("Mtu", "The MAC-level Maximum Transmission Unit",
                   UintegerValue (1500),
                   MakeUintegerAccessor (&SwitchNetDevice::SetMtu,
                                         &SwitchNetDevice::GetMtu),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("EnableLearning",
                   "Enable the learning mode of the Learning Bridge",
                   BooleanValue (true),
                   MakeBooleanAccessor (&SwitchNetDevice::m_enableLearning),
                   MakeBooleanChecker ())
    .AddAttribute ("ExpirationTime",
                   "Time it takes for learned MAC state entry to expire.",
                   TimeValue (Seconds (300)),
                   MakeTimeAccessor (&SwitchNetDevice::m_expirationTime),
                   MakeTimeChecker ())
  ;
  return tid;
}


SwitchNetDevice::SwitchNetDevice ()
  : m_node (0),
    m_ifIndex (0)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_channel = CreateObject<BridgeChannel> ();
}

SwitchNetDevice::~SwitchNetDevice()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
SwitchNetDevice::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();
  for (std::vector< Ptr<NetDevice> >::iterator iter = m_ports.begin (); iter != m_ports.end (); iter++)
    {
      *iter = 0;
    }
  m_ports.clear ();
  m_channel = 0;
  m_node = 0;
  NetDevice::DoDispose ();
}

void
SwitchNetDevice::ReceiveFromDevice (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet, uint16_t protocol,
                                    Address const &src, Address const &dst, PacketType packetType)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_DEBUG ("UID is " << packet->GetUid ());

  // std::cout << "Bridge received packet with UID "<< packet->GetUid() << " from " << incomingPort << std::endl;

  // std::cout << "HERE{{{{{{"<<std::endl<<std::flush;

  Mac48Address src48 = Mac48Address::ConvertFrom (src);
  Mac48Address dst48 = Mac48Address::ConvertFrom (dst);

  if (!m_promiscRxCallback.IsNull ())
    {
      m_promiscRxCallback (this, packet, protocol, src, dst, packetType);
    }

  switch (packetType)
    {
    case PACKET_HOST:
      if (dst48 == m_address)
        {
          std::cout << "Outing from here \n";
          Learn (src48, incomingPort);
          m_rxCallback (this, packet, protocol, src);
        }
      break;

    case PACKET_BROADCAST:
    case PACKET_MULTICAST:
      // std::cout << "broadcast/multicast \n";
      m_rxCallback (this, packet, protocol, src);
      ForwardBroadcast (incomingPort, packet, protocol, src48, dst48);
      break;

    case PACKET_OTHERHOST:
      if (dst48 == m_address)
        {
          //std::cout << "otherhost \n";
          Learn (src48, incomingPort);
          m_rxCallback (this, packet, protocol, src);
        }
      else
        {
          //std::cout << "unicast\n"; 
          // we can always assume that the data incoming is from CSMA net device
          
           
          // incomingPortCsma->GetQueue();
          // __stats stats = getCurrentStats(incomingPort);
          // DynamicCast<SwitchNode>(m_node)->process(incomingPort, const_cast<Packet*>(GetPointer(packet)), protocol, src, dst, packetType, stats);
          ForwardUnicast (incomingPort, packet, protocol, src48, dst48);
        }
      break;
    }
}

__stats SwitchNetDevice::getCurrentStats(Ptr<NetDevice> incomingPort) {
  __stats stats;
  Ptr<CsmaNetDevice> incomingPortCsma = DynamicCast<CsmaNetDevice>(incomingPort);
  stats.queueOccupancy = incomingPortCsma->getCurrentQueueOccupancy();
  stats.timestampMs = Simulator::Now().GetMicroSeconds();
  // incomingPortCsma->GetQueue();
  return stats;
}

void
SwitchNetDevice::ForwardUnicast (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet,
                                 uint16_t protocol, Mac48Address src, Mac48Address dst)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_DEBUG ("LearningBridgeForward (incomingPort=" << incomingPort->GetInstanceTypeId ().GetName ()
                                                       << ", packet=" << packet << ", protocol="<<protocol
                                                       << ", src=" << src << ", dst=" << dst << ")");

  Learn (src, incomingPort);
  Ptr<NetDevice> outPort = GetLearnedState (dst);
  if (outPort != NULL && outPort != incomingPort)
    {
          __stats stats = getCurrentStats(outPort);
          DynamicCast<SwitchNode>(m_node)->process(incomingPort, outPort, const_cast<Packet*>(GetPointer(packet)), protocol, src, dst,  stats);
          // std::cout << "here << \n";

      NS_LOG_LOGIC ("Learning bridge state says to use port `" << outPort->GetInstanceTypeId ().GetName () << "'");
      outPort->SendFrom (packet->Copy (), src, dst, protocol);
    }
  else
    {
      NS_LOG_LOGIC ("No learned state: send through all ports");
      for (std::vector< Ptr<NetDevice> >::iterator iter = m_ports.begin ();
           iter != m_ports.end (); iter++)
        {
          Ptr<NetDevice> port = *iter;
          if (port != incomingPort)
            {
              NS_LOG_LOGIC ("LearningBridgeForward (" << src << " => " << dst << "): " 
                                                      << incomingPort->GetInstanceTypeId ().GetName ()
                                                      << " --> " << port->GetInstanceTypeId ().GetName ()
                                                      << " (UID " << packet->GetUid () << ").");
              port->SendFrom (packet->Copy (), src, dst, protocol);
            }
        }
    }
}

void
SwitchNetDevice::ForwardBroadcast (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet,
                                   uint16_t protocol, Mac48Address src, Mac48Address dst)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_DEBUG ("LearningBridgeForward (incomingPort=" << incomingPort->GetInstanceTypeId ().GetName ()
                                                       << ", packet=" << packet << ", protocol="<<protocol
                                                       << ", src=" << src << ", dst=" << dst << ")");
  Learn (src, incomingPort);

  for (std::vector< Ptr<NetDevice> >::iterator iter = m_ports.begin ();
       iter != m_ports.end (); iter++)
    {
      Ptr<NetDevice> port = *iter;
      if (port != incomingPort)
        {
          NS_LOG_LOGIC ("LearningBridgeForward (" << src << " => " << dst << "): " 
                                                  << incomingPort->GetInstanceTypeId ().GetName ()
                                                  << " --> " << port->GetInstanceTypeId ().GetName ()
                                                  << " (UID " << packet->GetUid () << ").");
          port->SendFrom (packet->Copy (), src, dst, protocol);
        }
    }
}

void SwitchNetDevice::Learn (Mac48Address source, Ptr<NetDevice> port)
{
  NS_LOG_FUNCTION_NOARGS ();
  if (m_enableLearning)
    {
      LearnedState &state = m_learnState[source];
      state.associatedPort = port;
      state.expirationTime = Simulator::Now () + m_expirationTime;
    }
}

Ptr<NetDevice> SwitchNetDevice::GetLearnedState (Mac48Address source)
{
  NS_LOG_FUNCTION_NOARGS ();
  if (m_enableLearning)
    {
      Time now = Simulator::Now ();
      std::map<Mac48Address, LearnedState>::iterator iter =
        m_learnState.find (source);
      if (iter != m_learnState.end ())
        {
          LearnedState &state = iter->second;
          if (state.expirationTime > now)
            {
              return state.associatedPort;
            }
          else
            {
              m_learnState.erase (iter);
            }
        }
    }
  return NULL;
}

uint32_t
SwitchNetDevice::GetNBridgePorts (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_ports.size ();
}


Ptr<NetDevice>
SwitchNetDevice::GetBridgePort (uint32_t n) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_ports[n];
}

void 
SwitchNetDevice::AddBridgePort (Ptr<NetDevice> bridgePort, uint64_t portId)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (bridgePort != this);
  if (!Mac48Address::IsMatchingType (bridgePort->GetAddress ()))
    {
      NS_FATAL_ERROR ("Device does not support eui 48 addresses: cannot be added to bridge.");
    }
  if (!bridgePort->SupportsSendFrom ())
    {
      NS_FATAL_ERROR ("Device does not support SendFrom: cannot be added to bridge.");
    }
  if (m_address == Mac48Address ())
    {
      m_address = Mac48Address::ConvertFrom (bridgePort->GetAddress ());
    }

  NS_LOG_DEBUG ("RegisterProtocolHandler for " << bridgePort->GetInstanceTypeId ().GetName ());
  //@author bibek sh
  // add portId for ease
  bridgePort->setGenericId(portId);
  // std::cout << "  Adding port: " << (uint64_t)PeekPointer(bridgePort) << " PortId: "<<bridgePort->getGenericId()<< std::endl;
  m_node->RegisterProtocolHandler (MakeCallback (&SwitchNetDevice::ReceiveFromDevice, this),
                                   0, bridgePort, true);
  m_ports.push_back (bridgePort);
  m_channel->AddChannel (bridgePort->GetChannel ());
}

void 
SwitchNetDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_ifIndex = index;
}

uint32_t 
SwitchNetDevice::GetIfIndex (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_ifIndex;
}

Ptr<Channel> 
SwitchNetDevice::GetChannel (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_channel;
}

void
SwitchNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_address = Mac48Address::ConvertFrom (address);
}

Address 
SwitchNetDevice::GetAddress (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_address;
}

bool 
SwitchNetDevice::SetMtu (const uint16_t mtu)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_mtu = mtu;
  return true;
}

uint16_t 
SwitchNetDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_mtu;
}


bool 
SwitchNetDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}


void 
SwitchNetDevice::AddLinkChangeCallback (Callback<void> callback)
{}


bool 
SwitchNetDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}


Address
SwitchNetDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool
SwitchNetDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

Address
SwitchNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this << multicastGroup);
  Mac48Address multicast = Mac48Address::GetMulticast (multicastGroup);
  return multicast;
}


bool 
SwitchNetDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return false;
}

bool 
SwitchNetDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}


bool 
SwitchNetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION_NOARGS ();
  return SendFrom (packet, m_address, dest, protocolNumber);
}

bool 
SwitchNetDevice::SendFrom (Ptr<Packet> packet, const Address& src, const Address& dest, uint16_t protocolNumber)
{
  std::cout << "Bridge sending " << std::endl << std::flush;
  NS_LOG_FUNCTION_NOARGS ();
  Mac48Address dst = Mac48Address::ConvertFrom (dest); 

  // try to use the learned state if data is unicast
  if (!dst.IsGroup ())
    {
      Ptr<NetDevice> outPort = GetLearnedState (dst);
      if (outPort != NULL) 
        {
          outPort->SendFrom (packet, src, dest, protocolNumber);
          return true;
        }
    }

  // data was not unicast or no state has been learned for that mac
  // address => flood through all ports.
  Ptr<Packet> pktCopy;
  for (std::vector< Ptr<NetDevice> >::iterator iter = m_ports.begin ();
       iter != m_ports.end (); iter++)
    {
      pktCopy = packet->Copy ();
      Ptr<NetDevice> port = *iter;
      port->SendFrom (pktCopy, src, dest, protocolNumber);
    }

  return true;
}


Ptr<Node> 
SwitchNetDevice::GetNode (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_node;
}


void 
SwitchNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_node = node;
}


bool 
SwitchNetDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}


void 
SwitchNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_rxCallback = cb;
}

void 
SwitchNetDevice::SetPromiscReceiveCallback (NetDevice::PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_promiscRxCallback = cb;
}

bool
SwitchNetDevice::SupportsSendFrom () const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

Address SwitchNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  return Mac48Address::GetMulticast (addr);
}

} // namespace ns3
