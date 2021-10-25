#include <iostream>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/bridge-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
// #include "ns3/gtcp-helper.h"
#include <iostream>
#include <fstream>
#include <unordered_map>


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TEST");


CsmaHelper createCsmaHelper (std::string dataRate, std::string queueSize, int delayMicroSec);

/* temp fn(s) */
void packetsInQueueTrace (std::string context, uint32_t oldValue, uint32_t newValue);

std::string LINK_DATA_RATE = "10Mbps";
std::string QUEUE_SIZE = "1000p";
int DELAY_MICROSEC = 5;

int
main (int argc, char *argv[])
{
  // lets create a simple topology
  CsmaHelper generalCsmaHelper = createCsmaHelper (LINK_DATA_RATE, QUEUE_SIZE, DELAY_MICROSEC);
  NodeContainer switches;
  for(int i=0;i<4;i++) {
    switches.Add(CreateObject<Node>());    
  }

  NetDeviceContainer switchLinks;
  std::unordered_map<int, NetDeviceContainer> devicesList;
  // 0-1
  NetDeviceContainer l01 = generalCsmaHelper.Install(NodeContainer(switches.Get(0), switches.Get(1)));
  devicesList[0].Add(l01.Get(0));
  devicesList[1].Add(l01.Get(1));

  // 1-2
  NetDeviceContainer l12 = generalCsmaHelper.Install(NodeContainer(switches.Get(1), switches.Get(2)));
  devicesList[1].Add(l12.Get(0));
  devicesList[2].Add(l12.Get(1));

  // 2-3
  NetDeviceContainer l23 = generalCsmaHelper.Install(NodeContainer(switches.Get(2), switches.Get(3)));
  devicesList[2].Add(l23.Get(0));
  devicesList[3].Add(l23.Get(1));

  // 0-3
  // NetDeviceContainer l03 = generalCsmaHelper.Install(NodeContainer(switches.Get(0), switches.Get(3)));
  // devicesList[0].Add(l03.Get(0));
  // devicesList[3].Add(l03.Get(1));


  NodeContainer nodes;
  nodes.Create(4);
  NetDeviceContainer nodeDevices;

  // link the nodes
  for(int i=0;i<4;i++) {
    NetDeviceContainer n00 = generalCsmaHelper.Install(NodeContainer(nodes.Get(i), switches.Get(i)));
    nodeDevices.Add(n00.Get(0));
    devicesList[i].Add(n00.Get(1));
  }

  //install bridge
  BridgeHelper switchHelper;
  for(int i=0;i<4;i++) {
    switchHelper.Install(switches.Get(i), devicesList[i]);
  }

  //
  //ip
  InternetStackHelper internet;
  internet.Install(nodes);
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer nodeips = ipv4.Assign (nodeDevices);


// from node, to node, start time, end time
  std::vector<std::tuple<int, int, int, int>> backgroundTransfersMapTerminal2Terminal;
  int psinkPort = 9997;
  PacketSinkHelper psink ("ns3::TcpSocketFactory",
                          InetSocketAddress (Ipv4Address::GetAny (), psinkPort));
  ApplicationContainer sinkapps = psink.Install (nodes);
  sinkapps.Start (Seconds (0));
  sinkapps.Stop (Seconds (50));

  backgroundTransfersMapTerminal2Terminal.push_back (std::make_tuple (0, 1, 5, 20));
//  backgroundTransfersMapTerminal2Terminal.push_back (std::make_tuple (2, 3, 5, 20));

  for (std::tuple<int, int, int, int> h2h : backgroundTransfersMapTerminal2Terminal)
    {
      // this means the transfer is set between them (sender host, destination host, start time, end time)
      BulkSendHelper psrc (
          "ns3::TcpSocketFactory",
          InetSocketAddress (nodeips.GetAddress (std::get<1> (h2h)), psinkPort));
      psrc.SetAttribute ("MaxBytes", UintegerValue (0));
      ApplicationContainer psrcapp = psrc.Install (nodes.Get (std::get<0> (h2h)));
      psrcapp.Start (Seconds (std::get<2> (h2h)));
      psrcapp.Stop (Seconds (std::get<3> (h2h)));
   }


  Config::Connect("/NodeList/*/DeviceList/*/$ns3::CsmaNetDevice/TxQueue/PacketsInQueue", MakeCallback(&packetsInQueueTrace));
  Simulator::Stop (Seconds (50));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}

CsmaHelper
createCsmaHelper (std::string dataRate, std::string queueSize, int delayMicroSec)
{
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue (dataRate));
  csma.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (delayMicroSec)));
  csma.SetQueue ("ns3::DropTailQueue<Packet>", "MaxSize", QueueSizeValue (QueueSize (queueSize)));
  return csma;
}

void packetsInQueueTrace (std::string context, uint32_t oldValue, uint32_t newValue)
{
  NS_LOG_UNCOND("QUEUE " << Simulator::Now().GetMilliSeconds() <<" " << context << " " << oldValue <<" " << newValue);
}