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
  */

// Network topology
//
//        n0     n1
//        |      |
//       ----------
//       | Switch |
//       ----------
//        |      |
//        n2     n3
//
//
// - CBR/UDP flows from n0 to n1 and from n3 to n0
// - DropTail queues
// - Tracing of queues and packet receptions to file "csma-bridge.tr"

#include <iostream>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/bridge-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Test3");
void packetsInQueueTrace (std::string context, uint32_t oldValue, uint32_t newValue);

int
main (int argc, char *argv[])
{
  NS_LOG_INFO ("Start Simulation.");
  // LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);
  NodeContainer nodes;
  nodes.Create (3);

  NodeContainer n01 = NodeContainer (nodes.Get (0), nodes.Get (1));
  NodeContainer n12 = NodeContainer (nodes.Get (1), nodes.Get (2));

  PointToPointHelper p2p;
  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("20Mbps"));
  //p2p.SetChannelAttribute ("Delay", StringValue ("0.02ms"));

  PointToPointHelper p2p2;
  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps")); 

  NetDeviceContainer d0d2 = p2p.Install (n01);
  NetDeviceContainer d1d2 = p2p2.Install (n12);

  d0d2.Get(0)->setGenericId(100);
  d0d2.Get(1)->setGenericId(101);
d1d2.Get(0)->setGenericId(102);
d1d2.Get(1)->setGenericId(103);

  InternetStackHelper internet;
  internet.InstallAll ();
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  ipv4.Assign (d0d2);
   ipv4.SetBase ("10.1.2.0", "255.255.255.0");
   ipv4.Assign (d1d2);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  // Ipv4GlobalRoutingHelper::PrintRoutingTableAllEvery();

  int port = 1010;

  Ptr<Node> receiver = nodes.Get (2);
  Ptr<NetDevice> ren = receiver->GetDevice (0);
  Ptr<Ipv4> add_ipv4 = receiver->GetObject<Ipv4> ();
  Ipv4InterfaceAddress r_ip = add_ipv4->GetAddress (1, 0);
  Ipv4Address r_ipaddr = r_ip.GetLocal ();

  ApplicationContainer sinkapps;
  ApplicationContainer sender;



  BulkSendHelper psrc ("ns3::TcpSocketFactory",
                       InetSocketAddress (r_ipaddr, port));
  psrc.SetAttribute ("MaxBytes", UintegerValue (0));
  ApplicationContainer psrcapp = psrc.Install (nodes.Get (0));
  sender.Add(psrcapp);

  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         Address (InetSocketAddress (Ipv4Address::GetAny (), port)));
  sinkapps.Add (sink.Install (nodes.Get (2)));

  sender.Start (Seconds (10.0));
  sender.Stop (Seconds (20.0));

  sinkapps.Start (Seconds (0.0));
  sinkapps.Stop (Seconds (50.0));

  std::cout << "Testing Starting \n" << std::endl << std::flush;
  NS_LOG_INFO ("Start Simulation.");
Config::Connect ("/NodeList/1/DeviceList/*/$ns3::PointToPointNetDevice/TxQueue/PacketsInQueue",
                  MakeCallback (&packetsInQueueTrace));
  Simulator::Stop (Seconds (50));

  NS_LOG_INFO ("Start Simulation.");

  Simulator::Run ();

  Simulator::Destroy ();
  NS_LOG_INFO ("Simulation Done.");

  return 0;
}

void
packetsInQueueTrace (std::string context, uint32_t oldValue, uint32_t newValue)
{
  NS_LOG_UNCOND ("QUEUE " << Simulator::Now ().GetMilliSeconds () << " " << context << " "
                          << oldValue << " " << newValue);
}