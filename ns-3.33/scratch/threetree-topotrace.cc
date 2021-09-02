#include <iostream>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/bridge-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/scheduler-topo.h"

#include <iostream>
#include <fstream>
#include <unordered_map>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("THREETREE");

#define __DUMP_IPS 0 
#define __ASCII_TRACE 0
#define __PCAP_TRACE 0 

CsmaHelper createCsmaHelper (std::string dataRate, std::string queueSize, int delayMicroSec);

int
main (int argc, char *argv[])
{

  int dumpterminalips = 0;
  int runtrace = 0;
  int fromTerminalIdx = 0;
  int toTerminalIdx = 0;

  SchedulerTopo topo;

  CommandLine cmd;
  cmd.AddValue("dumpterminalips", "terminal ips", dumpterminalips);
  cmd.AddValue("runtrace", "run trace", runtrace);
  cmd.AddValue("fromTerminal", "from terminal index", fromTerminalIdx);
  cmd.AddValue("toTerminal", "to terminal index", toTerminalIdx);
  cmd.Parse (argc, argv);



  // topology configurations
  int AGGREGATION_SW_N = 10;
  int EDGE_PER_AGG_SW_N = 2;
  int NODE_PER_EDGE = 2;
  // ------------------------

  // probing configurations
  uint16_t PROBE_PORT = 9999;
  uint16_t PROBE_SENDER_MAX_PACKETS = 1; // 0 = unlimited
  float PROBE_SENDER_INTERVAL_SEC = 0.1; // 100ms
  // --------------------------------------

  // dataplane switch configuration
  uint64_t ROLL_STATS_PERIOD_MS = PROBE_SENDER_INTERVAL_SEC * 1000;
  // ------------------------------------------------------------------

  // CSMA links configurations
  std::string LINK_DATA_RATE = "10Mbps";
  std::string QUEUE_SIZE = "100p";
  int DELAY_MICROSEC = 5;
  CsmaHelper generalCsmaHelper = createCsmaHelper (LINK_DATA_RATE, QUEUE_SIZE, DELAY_MICROSEC);
  // -----------------------------------------------------------------------------------------

  /**
 * Create dataplane switch nodes
 * */
  NodeContainer dataplaneCoreSw;
  dataplaneCoreSw.Add (CreateObject<SwitchNode> (PROBE_PORT, ROLL_STATS_PERIOD_MS));

  NodeContainer dataplaneAggSw;
  for (int i = 0; i < AGGREGATION_SW_N; i++)
    {
      dataplaneAggSw.Add (CreateObject<SwitchNode> (PROBE_PORT, ROLL_STATS_PERIOD_MS));
    }

  NodeContainer dataplaneEdgeSw;
  for (int i = 0; i < AGGREGATION_SW_N * EDGE_PER_AGG_SW_N; i++)
    {
      dataplaneEdgeSw.Add (CreateObject<SwitchNode> (PROBE_PORT, ROLL_STATS_PERIOD_MS));
    }

  /**
    * Use CSMA to link all the dataplane switches
    * */
  NetDeviceContainer coreNetDevices;
  std::unordered_map<int, NetDeviceContainer> aggregateNetDevices;
  std::unordered_map<int, NetDeviceContainer> edgeNetDevices;

  // link core with aggregation sws
  for (int i = 0; i < AGGREGATION_SW_N; i++)
    {
      NetDeviceContainer link = generalCsmaHelper.Install (
          NodeContainer (dataplaneCoreSw.Get (0), dataplaneAggSw.Get (i)));
      coreNetDevices.Add (link.Get (0));
      aggregateNetDevices[i].Add (link.Get (1));
    }

  // link aggregation sw with edge sw
  int edge_idx = 0;
  for (int i = 0; i < AGGREGATION_SW_N; i++)
    {
      for (int j = 0; j < EDGE_PER_AGG_SW_N; j++)
        {
          NetDeviceContainer link = generalCsmaHelper.Install (
              NodeContainer (dataplaneAggSw.Get (i), dataplaneEdgeSw.Get (edge_idx)));

          aggregateNetDevices[i].Add (link.Get (0));
          edgeNetDevices[edge_idx++].Add (link.Get (1));
        }
    }

  // we will create probing node which is attached to all the network switches
  int totalProbingNodes = 1 + AGGREGATION_SW_N + AGGREGATION_SW_N * EDGE_PER_AGG_SW_N;
  NodeContainer probingNodes;
  probingNodes.Create (totalProbingNodes);

  // now we will attach these probing nodes to all of the switches
  NetDeviceContainer probingDevices;
  int probe_idx = 0;
  // start off with the core
  {
    NetDeviceContainer probeCoreLink = generalCsmaHelper.Install (
        NodeContainer (dataplaneCoreSw.Get (0), probingNodes.Get (probe_idx++)));
    coreNetDevices.Add (probeCoreLink.Get (0));
    probingDevices.Add (probeCoreLink.Get (1));
  }
  // now we add probing node to each of the aggregate switches
  for (int i = 0; i < AGGREGATION_SW_N; i++)
    {
      NetDeviceContainer link = generalCsmaHelper.Install (
          NodeContainer (dataplaneAggSw.Get (i), probingNodes.Get (probe_idx++)));
      aggregateNetDevices[i].Add (link.Get (0));
      probingDevices.Add (link.Get (1));
    }
  // now we add probing node to each of the edge switches
  for (int i = 0; i < AGGREGATION_SW_N * EDGE_PER_AGG_SW_N; i++)
    {
      NetDeviceContainer link = generalCsmaHelper.Install (
          NodeContainer (dataplaneEdgeSw.Get (i), probingNodes.Get (probe_idx++)));
      edgeNetDevices[i].Add (link.Get (0));
      probingDevices.Add (link.Get (1));
    }
    // end of adding probing nodes

  // now we will create terminal edge nodes
  NodeContainer terminals;
  terminals.Create (AGGREGATION_SW_N * EDGE_PER_AGG_SW_N * NODE_PER_EDGE);

  // link all the terminals to the edges
  NetDeviceContainer terminalDevices;
  int terminal_idx = 0;
  for (int i = 0; i < AGGREGATION_SW_N * EDGE_PER_AGG_SW_N; i++)
    {
      for (int j = 0; j < NODE_PER_EDGE; j++)
        {
          NetDeviceContainer link = generalCsmaHelper.Install (
              NodeContainer (dataplaneEdgeSw.Get (i), terminals.Get (terminal_idx++)));

          edgeNetDevices[i].Add (link.Get (0));
          terminalDevices.Add (link.Get (1));
        }
    }

  // Install the dataplane module to the switches
  SwitchHelper switchHelper;
  switchHelper.Install (dataplaneCoreSw.Get (0), coreNetDevices);
  for (int i = 0; i < AGGREGATION_SW_N; i++)
    {
      switchHelper.Install (dataplaneAggSw.Get (i), aggregateNetDevices[i]);
    }
  for (int i = 0; i < AGGREGATION_SW_N * EDGE_PER_AGG_SW_N; i++)
    {
      switchHelper.Install (dataplaneEdgeSw.Get (i), edgeNetDevices[i]);
    }

  // Install internet stack to all the terminal nodes
  InternetStackHelper internet;
  internet.Install(probingNodes);
  internet.Install (terminals);

  // now we give ip address to all the terminals
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer probeips = ipv4.Assign(probingDevices);
  Ipv4InterfaceContainer terminalips = ipv4.Assign (terminalDevices);

#if __DUMP_IPS == 1
  std::cout << "[ Probing Nodes IPs ] \n";
  for (int i = 0; i < (int) probeips.GetN (); i++)
    {
      std::stringstream peerAddressStringStream, p2;
      peerAddressStringStream << Ipv4Address::ConvertFrom (probeips.GetAddress (i));
      std::cout << " " << i << " = " << peerAddressStringStream.str () << std::endl;
    }

  std::cout << "[ Terminal Nodes IPs ] \n";
  for (int i = 0; i < (int) terminalips.GetN (); i++)
    {
      std::stringstream peerAddressStringStream, p2;
      peerAddressStringStream << Ipv4Address::ConvertFrom (terminalips.GetAddress (i));
      std::cout << " " << i << " = " << peerAddressStringStream.str () << std::endl;
    }
#endif

  // uint16_t port = 9; // Discard port (RFC 863)
  // Address sinkAddress (InetSocketAddress (ips.GetAddress (ips.GetN () - 1), port));
  // PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory",
  //                                    InetSocketAddress (Ipv4Address::GetAny (), port));
  // ApplicationContainer sinkapp = packetSinkHelper.Install (terminals.Get (terminals.GetN () - 1));
  // sinkapp.Start (Seconds (0));
  // sinkapp.Stop (Seconds (10));

  // BulkSendHelper source0 ("ns3::TcpSocketFactory", sinkAddress);
  // ApplicationContainer sourceapp = source0.Install (probingNodes.Get (0));
  // sourceapp.Start (Seconds (1));
  // sourceapp.Stop (Seconds (10));


 #ifdef SCHEDULER 
  // we will last one as the scheduler node
  PSchedulerHelper schedulerhelper(PROBE_PORT);
  ns3::Address schedulerIpAddress = ns3::Address (terminalips.GetAddress (terminalips.GetN()-1));

  ApplicationContainer schedulerApp = schedulerhelper.Install(terminals.Get(terminals.GetN()-1));
  schedulerApp.Start(Seconds(0.0));
  schedulerApp.Stop(Seconds(10.0));

  
  ProbeAppHelper probeapphelper(schedulerIpAddress, PROBE_PORT, PROBE_SENDER_MAX_PACKETS,
                            PROBE_SENDER_INTERVAL_SEC);
  ApplicationContainer app = probeapphelper.Install(probingNodes.Get(0));
  app.Start(Seconds(1));
  app.Stop(Seconds(10));
#endif

#define PATH_TRACE
#ifdef PATH_TRACE

  if(dumpterminalips == 1) {
    for (int i = 0; i < (int) terminalips.GetN (); i++)
    {
      std::stringstream peerAddressStringStream, p2;
      peerAddressStringStream << Ipv4Address::ConvertFrom (terminalips.GetAddress (i));
      std::cout << "DUMPIPS " << i << " " << peerAddressStringStream.str () << std::endl;
    }
    return 0;
  }

  if(runtrace == 1) {
    ns3::Address targetIpAddress = ns3::Address (terminalips.GetAddress (toTerminalIdx));
    ProbeAppHelper probeapphelper(targetIpAddress, PROBE_PORT+2, PROBE_SENDER_MAX_PACKETS,
                              PROBE_SENDER_INTERVAL_SEC);
    ApplicationContainer app = probeapphelper.Install(terminals.Get(fromTerminalIdx));
    app.Start(Seconds(1));
    app.Stop(Seconds(10));
  }
#endif


#ifdef TEST_DATA_TRANSFER
  // data transfer application
  int dataTransferPort = 12345;
  Address sinkAddress (InetSocketAddress (terminalips.GetAddress (terminalips.GetN()-2), dataTransferPort));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory",
                                     InetSocketAddress (Ipv4Address::GetAny (), dataTransferPort));
  ApplicationContainer sinkapp = packetSinkHelper.Install (terminals.Get (terminals.GetN()-2));
  sinkapp.Start (Seconds (1));
  sinkapp.Stop (Seconds(10));
  
  BulkSendHelper source0 ("ns3::TcpSocketFactory", sinkAddress);
  ApplicationContainer sourceapp = source0.Install (terminals.Get (1));
  sourceapp.Start (Seconds (1));
  sourceapp.Stop (Seconds(10));
#endif


#if __ASCII_TRACE == 1
  AsciiTraceHelper ascii;
  generalCsmaHelper.EnableAsciiAll (ascii.CreateFileStream ("tree.tr"));
#endif

#if __PCAP_TRACE == 1
  generalCsmaHelper.EnablePcapAll ("tree", false);
#endif

  Simulator::Stop (Seconds (10));
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