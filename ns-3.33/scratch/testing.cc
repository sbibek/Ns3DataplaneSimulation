#include <iostream>
#include <fstream>
#include <string>
#include <cassert>


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/stats-module.h"
#include "ns3/flow-monitor-helper.h"


using namespace std;
using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("mlgbTest");


int main (int argc, char *argv[])
{
	
	int tcpflows = 80;
	//cout << UintegerValue(enable) << endl;
	//Config::SetDefault ("ns3::DropTailQueue::MaxPackets", UintegerValue(10000));
	//config tcp
	// Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue(1000));
	// Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue (0));
	// Config::SetDefault("ns3::TcpSocket::DelAckTimeout", TimeValue(Seconds (0)));
	// Config::SetDefault ("ns3::TcpSocket::SlowStartThreshold", UintegerValue(65535*2));
	// Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue(131072));
	// Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue(131072));
	//Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue(TypeId::LookupByName ("ns3::TcpNewReno")));
	NS_LOG_INFO ("Create Nodes.");

	NodeContainer nodes;
	nodes.Create(4);
	//cout << nodes.Get(0)->GetReserve() << endl;
	NodeContainer n0n2 = NodeContainer(nodes.Get(0), nodes.Get(2));
	NodeContainer n1n2 = NodeContainer(nodes.Get(1), nodes.Get(2));
	NodeContainer n2n3 = NodeContainer(nodes.Get(2), nodes.Get(3));

	NS_LOG_INFO ("Create channels.");
  	PointToPointHelper p2p;
  	p2p.SetQueue ("ns3::DropTailQueue");
  	p2p.SetDeviceAttribute ("DataRate", StringValue("1Gbps"));
  	p2p.SetChannelAttribute ("Delay", StringValue("0.02ms"));
  	NetDeviceContainer d0d2 = p2p.Install(n0n2);
  	NetDeviceContainer d1d2 = p2p.Install(n1n2);
  	// p2p.SetDeviceAttribute ("DataRate", StringValue("10Mbps"));
  	NetDeviceContainer d2d3 = p2p.Install(n2n3);
  	// add ip/tcp stack to all nodes
  	InternetStackHelper internet;
  	internet.InstallAll();

	NS_LOG_INFO ("Assign IP Addresses.");
	Ipv4AddressHelper ipv4;
	ipv4.SetBase ("10.1.1.0", "255.255.255.0");
	ipv4.Assign (d0d2);
	ipv4.SetBase ("10.1.2.0", "255.255.255.0");
	ipv4.Assign (d1d2);
	ipv4.SetBase ("10.1.3.0", "255.255.255.0");
	ipv4.Assign (d2d3);

	// and setup ip routing tables to get total ip-level connectivity.
	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
	

	//generate traffic
	int port = 1010;
	// int nsender1 = 0;
	int nreceiver = 3;

	Ptr<Node> receiver = nodes.Get(nreceiver);
	Ptr<NetDevice> ren = receiver->GetDevice(0);
	Ptr<Ipv4> add_ipv4 = receiver->GetObject<Ipv4> ();
	Ipv4InterfaceAddress r_ip = add_ipv4->GetAddress (1,0);
	Ipv4Address r_ipaddr = r_ip.GetLocal();

	// sink 
	ApplicationContainer sinkapps;

	// source 0 to 3
	ApplicationContainer sender;
	for(int i = 0; i < 1; i++)
	{
		OnOffHelper source ("ns3::TcpSocketFactory", Address (InetSocketAddress(r_ipaddr, port)));
		source.SetAttribute ("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=100]"));
		source.SetAttribute ("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
		source.SetAttribute ("DataRate", StringValue("1Gbps"));
		source.SetAttribute ("MaxBytes", UintegerValue(200000000));
		source.SetAttribute ("PacketSize", UintegerValue(1400));
		sender.Add(source.Install(nodes.Get(0)));
		PacketSinkHelper sink("ns3::TcpSocketFactory", 
			Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
		sinkapps.Add(sink.Install(nodes.Get(nreceiver)));
		port++;
	}

	for(int i = 0; i < tcpflows; i++)
	{
		OnOffHelper source ("ns3::TcpSocketFactory", Address (InetSocketAddress(r_ipaddr, port)));
		source.SetAttribute ("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=100]"));
		source.SetAttribute ("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
		source.SetAttribute ("DataRate", StringValue("1Gbps"));
		source.SetAttribute ("MaxBytes", UintegerValue(200000000));
		source.SetAttribute ("PacketSize", UintegerValue(1400));
		sender.Add(source.Install(nodes.Get(1)));
		PacketSinkHelper sink("ns3::TcpSocketFactory", 
			Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
		sinkapps.Add(sink.Install(nodes.Get(nreceiver)));
		port++;
	}
	sender.Start(Seconds(0.0));
	sender.Stop(Seconds(100.0));

	cout << "Application number: " << sender.GetN() << endl;
	
	sinkapps.Start(Seconds(0.0));
	sinkapps.Stop(Seconds(100.0));
	// done sink


	FlowMonitorHelper flowmon;
  	Ptr<FlowMonitor> monitor = flowmon.InstallAll();
	

	Simulator::Stop(Seconds(10.0));
	
	NS_LOG_INFO("Start Simulation.");

	Simulator::Run();

	monitor->CheckForLostPackets ();
  	monitor->SerializeToXmlFile("trace.xml", false, false);

	Simulator::Destroy();
	NS_LOG_INFO("Simulation Done.");

	return 0;
}

