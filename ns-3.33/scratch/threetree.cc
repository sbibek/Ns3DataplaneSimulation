#include <iostream>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/bridge-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/gtcp-helper.h"
#include "rapidjson/document.h"
#include <iostream>
#include <fstream>
#include <unordered_map>


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("THREETREE");

// typedefs for making names more identifiable
typedef BulkSendHelper2 GtcpOffloadHelper;
typedef GtcpServerHelper GtcpOffloadServerHelper;


#define __DUMP_IPS 0
#define __ASCII_TRACE 1
#define __PCAP_TRACE 0

/*********** SIMULATION VARS ********************/
int TOTAL_SIMULATION_TIME_S = 100;

/********* topology configurations ***************/
int AGGREGATION_SW_N = 10;
int EDGE_PER_AGG_SW_N = 2;
int NODE_PER_EDGE = 2;
// ------------------------

// probing configurations
uint16_t PROBE_PORT = 9999;
uint16_t PROBE_SENDER_MAX_PACKETS = 0; // 0 = unlimited
float PROBE_SENDER_INTERVAL_SEC = 0.1; // 100ms
// --------------------------------------

// dataplane switch configuration
uint64_t ROLL_STATS_PERIOD_MS = PROBE_SENDER_INTERVAL_SEC * 1000;
// ------------------------------------------------------------------

// CSMA links configurations
std::string LINK_DATA_RATE = "10Mbps";
std::string CORE_LINK_DATA_RATE = "50Mbps";
std::string AGG_LINK_DATA_RATE = "20Mbps";
std::string QUEUE_SIZE = "1000p";
int DELAY_MICROSEC = 5;


/************* GTCP Offload server conf ********************/
int GTCP_WAIT_TIME_TO_SIMULATE_PROCESSING_S = 5;
uint GTCP_MAX_RESPONSE_BYTES_TO_SIMULATE_RESPONSE = 1024*1024;
int GTCP_DATA_TRANSFER_PORT = 12345;
// ---------------------------------------------------------

/************* OFFLOAD Application conf ********************/
int OF_SELECTION_STRATEGY_OPTIMAL = 0;
int OF_SELECTION_STRATEGY_NEAR = 1;
int OF_SELECTION_STRATEGY_RAND = 2;


// offload plans (which node will offload, how many servers to offload to, max bytes to offload per server, selection_mode, start_time, end_time)
std::vector<std::tuple<int, int, uint64_t, int, int, int>> offloadPlans;

// --------------------------------------------------------

/**/
/************* Background transfer Application conf ********************/
// from node, to node, start time, end time
std::vector<std::tuple<int, int, int, int>> backgroundTransfersMapTerminal2Terminal;
// -----------------------------------------------------------------------------------

CsmaHelper createCsmaHelper (std::string dataRate, std::string queueSize, int delayMicroSec);

/**
 *  Important** 
 *  1. The data transfer port should be always be 12345 
 *  2. Query port is always 8080
 *  3. Background data transfer port is always 9990
 * 
 * */

/* temp fn(s) */
void packetsInQueueTrace (std::string context, uint32_t oldValue, uint32_t newValue);

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse(argc, argv);
  //   LogComponentEnable ("GtcpServer", LOG_LEVEL_DEBUG);
  //   LogComponentEnable ("BulkSendApplication2", LOG_LEVEL_DEBUG);
  //   LogComponentEnable ("GtcpClient", LOG_LEVEL_INFO);
  //   LogComponentEnable ("PScheduler", LOG_LEVEL_DEBUG);
  // LogComponentEnable ("BulkSendApplication", LOG_LEVEL_INFO);
  // LogComponentEnable("PacketSink", LOG_LEVEL_INFO);

    //   const char* json = "{\"project\":\"rapidjson\",\"stars\":10}";
    // rapidjson::Document d;
    // d.Parse(json);

  /*############################ TOPOLOGY SECTION ############################*/

  CsmaHelper generalCsmaHelper = createCsmaHelper (LINK_DATA_RATE, QUEUE_SIZE, DELAY_MICROSEC);
  CsmaHelper coreCsmaHelper = createCsmaHelper(CORE_LINK_DATA_RATE, QUEUE_SIZE, DELAY_MICROSEC);
  CsmaHelper aggCsmaHelper = createCsmaHelper(AGG_LINK_DATA_RATE, QUEUE_SIZE, DELAY_MICROSEC);
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
      NetDeviceContainer link = coreCsmaHelper.Install (
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
          NetDeviceContainer link = aggCsmaHelper.Install (
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
  // set terminal ids
  for (int i = 0; i < (int) terminals.GetN (); i++)
    {
      terminals.Get (i)->SetIdx (i);
    }

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
  internet.Install (probingNodes);
  internet.Install (terminals);

  // now we give ip address to all the terminals
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer probeips = ipv4.Assign (probingDevices);
  Ipv4InterfaceContainer terminalips = ipv4.Assign (terminalDevices);

  // updating the terminal nodes with ip information as required for our simulation
  // This is important because the nodes information is required for the data transfer operations
  for (int i = 0; i < (int) terminals.GetN (); i++)
    {
      terminals.Get (i)->SetTerminalIps (&terminalips);
    }

    // now we will share terminalDeviecs to all the terminal nodes

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
    /*############################ END OF TOPOLOGY SECTION ############################*/

    /*############################ INSTALL SCHEDULER AND PROBE MODULE SECTION ############################*/
#define SCHEDULER
#ifdef SCHEDULER
  int _schid = 1;
  // terminalips.GetN()-1;
  // we will last one as the scheduler node
  PSchedulerHelper schedulerhelper (PROBE_PORT);
  ns3::Address schedulerIpAddress = ns3::Address (terminalips.GetAddress (_schid));

  ApplicationContainer schedulerApp = schedulerhelper.Install (terminals.Get (_schid));
  schedulerApp.Start (Seconds (0.0));
  schedulerApp.Stop (Seconds (TOTAL_SIMULATION_TIME_S));

  ProbeAppHelper probeapphelper (schedulerIpAddress, PROBE_PORT, PROBE_SENDER_MAX_PACKETS,
                                 PROBE_SENDER_INTERVAL_SEC);
  // NodeContainer probeNodes;
  // probeNodes.Add(probingNodes.Get(0));
  // probeNodes.Add(probingNodes.Get(10));
  // probeNodes.Add(probingNodes.Get(30));
  ApplicationContainer app = probeapphelper.Install (
      // probingNodes.Get(0)
      probingNodes
      // probeNodes
  );
  app.Start (Seconds (1));
  app.Stop (Seconds (TOTAL_SIMULATION_TIME_S));
#endif
  /*############################ END OF INSTALL SCHEDULER AND PROBE MODULE SECTION ############################*/

  /*############################ OPTIONAL PATHTRACE MODULE ############################*/

#ifdef PATH_TRACE
  ns3::Address schedulerIpAddress = ns3::Address (terminalips.GetAddress (terminalips.GetN () - 1));
  ProbeAppHelper probeapphelper (schedulerIpAddress, PROBE_PORT + 1, PROBE_SENDER_MAX_PACKETS + 1,
                                 PROBE_SENDER_INTERVAL_SEC);
  ApplicationContainer app = probeapphelper.Install (terminals.Get (0));
  app.Start (Seconds (1));
  app.Stop (Seconds (10));
#endif
  /*############################ END OF OPTIONAL PATHTRACE MODULE ############################*/

  /*############################ OFFLOAD MODULE ############################*/
#define OFFLOAD_MOD
#ifdef OFFLOAD_MOD
  GtcpOffloadServerHelper gtcpserverHelper (GTCP_DATA_TRANSFER_PORT);
  gtcpserverHelper.SetAttribute ("WaitPeriod", TimeValue (Seconds (GTCP_WAIT_TIME_TO_SIMULATE_PROCESSING_S)));
  gtcpserverHelper.SetAttribute ("TotalResponseBytes", UintegerValue (GTCP_MAX_RESPONSE_BYTES_TO_SIMULATE_RESPONSE));

  ApplicationContainer gtcpOffloadApp = gtcpserverHelper.Install (terminals);
  gtcpOffloadApp.Start (Seconds (1));
  gtcpOffloadApp.Stop (Seconds (TOTAL_SIMULATION_TIME_S));

  // selection mode : 0 = OPTIMAL, 1 = NEAR, 2 = RANDOM
  offloadPlans.push_back(std::make_tuple(0, 1, 1024*1024*2, OF_SELECTION_STRATEGY_NEAR, 21, 50));
  // offloadPlans.push_back(std::make_tuple(5, 3, 1024*1024*2, OF_SELECTION_STRATEGY_NEAR, 10, 34));
  // offloadPlans.push_back(std::make_tuple(39, 3, 1024*1024*2, OF_SELECTION_STRATEGY_NEAR, 15, 34));
  // offloadPlans.push_back(std::make_tuple(38, 3, 1024*1024*2, OF_SELECTION_STRATEGY_NEAR, 10, 34));
  // offloadPlans.push_back(std::make_tuple(28, 5, 1024*1024*2, OF_SELECTION_STRATEGY_OPTIMAL, 5, 34));

  for(std::tuple<int, int, uint64_t, int, int, int> ofplan: offloadPlans) {
    GtcpOffloadHelper offload;
    offload.SetAttribute ("MaxBytes", UintegerValue (std::get<2>(ofplan)));
    offload.SetAttribute ("schedularAddress", AddressValue (schedulerIpAddress));
    offload.SetAttribute("totalServersToOffload", UintegerValue(std::get<1>(ofplan)));
    offload.SetAttribute("serverSelectionStrategy", IntegerValue(std::get<3>(ofplan)));

    NodeContainer offloadSrc;
    offloadSrc.Add(terminals.Get(std::get<0>(ofplan)));
    ApplicationContainer offloadApp = offload.Install (offloadSrc);
    offloadApp.Start (Seconds (std::get<4>(ofplan)));
    offloadApp.Stop (Seconds (std::get<5>(ofplan)));
  }
#endif
/*############################ END OF OFFLOAD MODULE ############################*/

/*############################ BACKGROUND TRAFFIC ############################*/
// we will schedule background data transfers from hosts to hosts
#define BACKGROUND_DATA_TRANSFERS
#ifdef BACKGROUND_DATA_TRANSFERS
  // we will install sink on all nodes
  int psinkPort = 9997;
  PacketSinkHelper psink ("ns3::TcpSocketFactory",
                          InetSocketAddress (Ipv4Address::GetAny (), psinkPort));
  ApplicationContainer sinkapps = psink.Install (terminals);
  sinkapps.Start (Seconds (0));
  sinkapps.Stop (Seconds (TOTAL_SIMULATION_TIME_S));

  //for example, the transfer between 10 and 20 host start at 0s and stop at 30s
  // backgroundTransfersMapTerminal2Terminal.push_back (std::make_tuple (0, 1, 20, 30));
  // backgroundTransfersMapTerminal2Terminal.push_back (std::make_tuple (4, 1, 15, 30));
  for(int i=0;i<3;i+=2) {
    backgroundTransfersMapTerminal2Terminal.push_back (std::make_tuple (i, i+1, 15, 30));
  }

  for (std::tuple<int, int, int, int> h2h : backgroundTransfersMapTerminal2Terminal)
    {
      // this means the transfer is set between them (sender host, destination host, start time, end time)
      BulkSendHelper psrc (
          "ns3::TcpSocketFactory",
          InetSocketAddress (terminalips.GetAddress (std::get<1> (h2h)), psinkPort));
      psrc.SetAttribute ("MaxBytes", UintegerValue (0));
      ApplicationContainer psrcapp = psrc.Install (terminals.Get (std::get<0> (h2h)));
      psrcapp.Start (Seconds (std::get<2> (h2h)));
      psrcapp.Stop (Seconds (std::get<3> (h2h)));
    }

#endif
/*############################ END OF BACKGROUND TRAFFIC ############################*/

//#define QUEUE_MONITOR
#ifdef QUEUE_MONITOR
Config::Connect("/NodeList/*/DeviceList/*/$ns3::CsmaNetDevice/TxQueue/PacketsInQueue", MakeCallback(&packetsInQueueTrace));
#endif

/*############################ TRACING ############################*/
#if __ASCII_TRACE == 1
  AsciiTraceHelper ascii;
  generalCsmaHelper.EnableAsciiAll (ascii.CreateFileStream ("tree.tr"));
#endif

#if __PCAP_TRACE == 1
  generalCsmaHelper.EnablePcapAll ("tree", false);
#endif
  /*############################ END OF TRACING ############################*/

  Simulator::Stop (Seconds (TOTAL_SIMULATION_TIME_S));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
  //  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkapps.Get (1));
  // std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
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