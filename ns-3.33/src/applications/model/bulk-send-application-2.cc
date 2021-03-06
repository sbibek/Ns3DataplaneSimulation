/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Georgia Institute of Technology
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
 * Author: George F. Riley <riley@ece.gatech.edu>
 */

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/boolean.h"
#include "bulk-send-application-2.h"
#include "ns3/data-transfer-header.h"
#include "query-header.h"
#include "query-response.h"
#include "query-value.h"
#include "ns3/ipv4-interface-container.h"
#include <experimental/random>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BulkSendApplication2");

NS_OBJECT_ENSURE_REGISTERED (BulkSendApplication2);

uint32_t BulkSendApplication2::connectionId = 0;
uint32_t BulkSendApplication2::offloadSessionId = 0;

TypeId
BulkSendApplication2::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::BulkSendApplication2")
          .SetParent<Application> ()
          .SetGroupName ("Applications")
          .AddConstructor<BulkSendApplication2> ()
          .AddAttribute ("SendSize", "The amount of data to send each time.", UintegerValue (512),
                         MakeUintegerAccessor (&BulkSendApplication2::m_sendSize),
                         MakeUintegerChecker<uint32_t> (1))
          .AddAttribute ("MaxBytes",
                         "The total number of bytes to send. "
                         "Once these bytes are sent, "
                         "no data  is sent again. The value zero means "
                         "that there is no limit.",
                         UintegerValue (100000),
                         MakeUintegerAccessor (&BulkSendApplication2::m_maxBytes),
                         MakeUintegerChecker<uint64_t> ())
          .AddAttribute ("WaitPeriod", "Amount of time between send and results recv",
                         TimeValue (Seconds (2.0)),
                         MakeTimeAccessor (&BulkSendApplication2::m_waitTime), MakeTimeChecker ())
          .AddAttribute ("Protocol", "The type of protocol to use.",
                         TypeIdValue (TcpSocketFactory::GetTypeId ()),
                         MakeTypeIdAccessor (&BulkSendApplication2::m_tid), MakeTypeIdChecker ())
          .AddAttribute ("EnableSeqTsSizeHeader", "Add SeqTsSizeHeader to each packet",
                         BooleanValue (false),
                         MakeBooleanAccessor (&BulkSendApplication2::m_enableSeqTsSizeHeader),
                         MakeBooleanChecker ())
          .AddTraceSource ("Tx", "A new packet is sent",
                           MakeTraceSourceAccessor (&BulkSendApplication2::m_txTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("TxWithSeqTsSize", "A new packet is created with SeqTsSizeHeader",
                           MakeTraceSourceAccessor (&BulkSendApplication2::m_txTraceWithSeqTsSize),
                           "ns3::PacketSink::SeqTsSizeCallback")
          .AddAttribute ("schedularAddress", "The destination Address of the outbound packets",
                         AddressValue (),
                         MakeAddressAccessor (&BulkSendApplication2::m_schedularAddress),
                         MakeAddressChecker ())
          .AddAttribute ("queryPort", "The destination port of the outbound packets",
                         UintegerValue (8080),
                         MakeUintegerAccessor (&BulkSendApplication2::m_queryPort),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("totalServersToOffload", "The destination port of the outbound packets",
                         UintegerValue (1),
                         MakeUintegerAccessor (&BulkSendApplication2::m_totalServersToOffload),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("serverSelectionStrategy", "The destination port of the outbound packets",
                         IntegerValue (0),
                         MakeIntegerAccessor (&BulkSendApplication2::m_serverSelectionStrategy),
                         MakeIntegerChecker<int> ())

      ;
  return tid;
}

BulkSendApplication2::BulkSendApplication2 ()
{
  NS_LOG_FUNCTION (this);
}

BulkSendApplication2::~BulkSendApplication2 ()
{
  NS_LOG_FUNCTION (this);
}

void
BulkSendApplication2::SetMaxBytes (uint64_t maxBytes)
{
  NS_LOG_FUNCTION (this << maxBytes);
  m_maxBytes = maxBytes;
}

void
BulkSendApplication2::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  // chain up
  Application::DoDispose ();
}

// Application Methods
void
BulkSendApplication2::StartApplication (void) // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);
  // query sockets :: Init
  m_querysocket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
  m_querysocket->Connect (
      InetSocketAddress (Ipv4Address::ConvertFrom (m_schedularAddress), m_queryPort));
  m_querysocket->SetRecvCallback (MakeCallback (&BulkSendApplication2::QueryResponseHandler, this));

  // assign offload session id for current offload session
  m_offloadSessionIdForThis = (BulkSendApplication2::offloadSessionId++);

  // Actual Logic
  // As soon the application starts, we need to send the query
  Simulator::Schedule (Seconds (0.0), &BulkSendApplication2::QuerySequence, this);
}

void
BulkSendApplication2::StopApplication (void) // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);
}

void
BulkSendApplication2::SendHeader (const Address &from, const Address &to, OffloadConnection *conn)
{
  NS_LOG_FUNCTION (this);
  // NS_LOG_DEBUG("[BulkSend2] Sending bytes information, total following bytes = " << m_maxBytes);
  // NS_LOG_DEBUG (
  //     "DEBUG NodeId=" << m_node->GetIdx ()
  //                     << " Sending total bytes of data to expect to the receiver which is "
  //                     << m_maxBytes << "bytes");
  logger ("metric")
      .add ("offload-session-id", m_offloadSessionIdForThis)
      .add ("connection-id", conn->connId)
      .add ("TOTAL_BYTES_TO_SEND", m_maxBytes)
      .log ();

  // first thing to do here is send a packet with details of bytes we will be sending
  DataTransferHeader dth;
  dth.SetTotalBytesFollowingThis (m_maxBytes);
  dth.SetConnectionId (conn->connId);
  dth.SetOffloadSessionId (m_offloadSessionIdForThis);

  // // lets send the packet as metadata
  Ptr<Packet> metadata = Create<Packet> (dth.GetSerializedSize ());
  metadata->AddHeader (dth);
  conn->m_socket->Send (metadata);
  conn->m_sentMetadata = true;
}

// Private helpers

void
BulkSendApplication2::TestSendData (const Address &from, const Address &to, Ptr<Socket> socket,
                                    OffloadConnection *conn)
{

  NS_LOG_FUNCTION (this);

  if (!conn->m_sentMetadata)
    {
      SendHeader (from, to, conn);
      logger("metric")
        .add ("offload-session-id", m_offloadSessionIdForThis)
        .add ("connection-id", conn->connId)
        .add("DATA_TRANSFER_STARTED_MS", Simulator::Now().GetMilliSeconds())
        .log();

      conn->m_transferStartedTime = Simulator::Now ();
    }

  while (conn->m_totBytes < m_maxBytes)
    { // Time to send more

      // uint64_t to allow the comparison later.
      // the result is in a uint32_t range anyway, because
      // m_sendSize is uint32_t.
      uint64_t toSend = m_sendSize;
      // Make sure we don't send too many
      if (m_maxBytes > 0)
        {
          toSend = std::min (toSend, m_maxBytes - conn->m_totBytes);
        }

      NS_LOG_LOGIC ("sending packet at " << Simulator::Now ());

      Ptr<Packet> packet;
      if (conn->m_unsentPacket)
        {
          packet = conn->m_unsentPacket;
          toSend = packet->GetSize ();
        }
      else if (m_enableSeqTsSizeHeader)
        {
          SeqTsSizeHeader header;
          header.SetSeq (m_seq++);
          header.SetSize (toSend);
          NS_ABORT_IF (toSend < header.GetSerializedSize ());
          packet = Create<Packet> (toSend - header.GetSerializedSize ());
          // Trace before adding header, for consistency with PacketSink
          m_txTraceWithSeqTsSize (packet, from, to, header);
          packet->AddHeader (header);
        }
      else
        {
          packet = Create<Packet> (toSend);
        }

      int actual = conn->m_socket->Send (packet);
      if ((unsigned) actual == toSend)
        {
          conn->m_totBytes += actual;
          m_txTrace (packet);
          conn->m_unsentPacket = 0;
        }
      else if (actual == -1)
        {
          // We exit this loop when actual < toSend as the send side
          // buffer is full. The "DataSent" callback will pop when
          // some buffer space has freed up.
          // NS_LOG_DEBUG ("Unable to send packet; caching for later attempt");
          conn->m_unsentPacket = packet;
          break;
        }
      else if (actual > 0 && (unsigned) actual < toSend)
        {
          // A Linux socket (non-blocking, such as in DCE) may return
          // a quantity less than the packet size.  Split the packet
          // into two, trace the sent packet, save the unsent packet
          NS_LOG_INFO ("Packet size: " << packet->GetSize () << "; sent: " << actual
                                       << "; fragment saved: " << toSend - (unsigned) actual);
          Ptr<Packet> sent = packet->CreateFragment (0, actual);
          Ptr<Packet> unsent = packet->CreateFragment (actual, (toSend - (unsigned) actual));
          conn->m_totBytes += actual;
          m_txTrace (sent);
          conn->m_unsentPacket = unsent;
          break;
        }
      else
        {
          NS_FATAL_ERROR ("Unexpected return value from m_socket->Send ()");
        }
    }

  // now we schedule the response
  if (conn->m_totBytes >= m_maxBytes && !conn->m_transferCompleted)
    {
      conn->m_transferCompleted = true;
      Time elapsed = Simulator::Now () - conn->m_transferStartedTime;
      // NS_LOG_DEBUG("[BulkSendApp] Took " << elapsed.GetMilliSeconds() << " to complete the transfer");
      // NS_LOG_DEBUG ("RESULT NodeId=" << m_node->GetIdx ()
      //                                << " Time taken to complete the transfer = "
      //                                << elapsed.GetMilliSeconds () << " ms");
      // logger ("result")
      //     .add ("offload-session-id", m_offloadSessionIdForThis)
      //     .add ("connection-id", conn->connId)
      //     .add ("TRANSFER_TIME_MS", elapsed.GetMilliSeconds ())
      //     .log ();
    }
}

void
BulkSendApplication2::ConnectionSucceeded (Ptr<Socket> socket)
{
  OffloadConnection *conn = m_connections[(void *) &(*socket)];
  if (conn == NULL)
    {
      logger ("error").add ("status", "null pointer on offload connections").log ();
    }
  else
    {
      // this means the connection is a success
      conn->m_isConnected = true;
    }
}

void
BulkSendApplication2::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("BulkSendApplication2, Connection Failed");
}

void
BulkSendApplication2::DataSend (Ptr<Socket> socket, uint32_t)
{
  NS_LOG_FUNCTION (this);
  // get the appropriate connection
  OffloadConnection *conn = m_connections[(void *) &(*socket)];
  if (conn && conn->m_isConnected)
    { // Only send new data if the connection has completed
      conn->updateSocket (socket);
      Address from, to;
      socket->GetSockName (from);
      socket->GetPeerName (to);
      TestSendData (from, to, socket, conn);
    }
}

void
BulkSendApplication2::ReceivedDataCallback (Ptr<Socket> socket)
{
  OffloadConnection *conn = m_connections[(void *) &(*socket)];
  if (conn == NULL)
    {
      logger ("error")
          .add ("status", "cannot find appropriate offload connection when receiving data")
          .log ();
      return;
    }

  conn->updateSocket (socket);
  if (conn->m_transferCompleted && !conn->m_responseStarted)
    {
      conn->m_responseStarted = true;
      conn->m_responseStartedTime = Simulator::Now ();
    }

  Ptr<Packet> packet;
  Address from;

  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
        break;

      NS_LOG_INFO (this << " [BulkDataApp] A packet of " << packet->GetSize () << " bytes"
                        << " received from " << InetSocketAddress::ConvertFrom (from).GetIpv4 ()
                        << " port " << InetSocketAddress::ConvertFrom (from).GetPort ());
    }
}

void
BulkSendApplication2::NormalCloseCallback (Ptr<Socket> socket)
{

  OffloadConnection *conn = m_connections[(void *) &(*socket)];
  if (conn == NULL)
    {
      logger ("error")
          .add ("status", "cannot find appropriate offload connection when receiving data")
          .log ();
      return;
    }

  if (conn->m_responseStarted)
    {
      Time elapsed = Simulator::Now () - conn->m_responseStartedTime;
      // NS_LOG_DEBUG("[BulkSendApp] Took " << elapsed.GetMilliSeconds() << "ms to receive the response");
      // NS_LOG_DEBUG ("RESULT NodeId=" << m_node->GetIdx () << " Time taken to receive the data = "
      //                                << elapsed.GetMilliSeconds () << " ms");
      // logger ("result")
      //     .add ("offload-session-id", m_offloadSessionIdForThis)
      //     .add ("connection-id", conn->connId)
      //     .add ("RESPONSE_RECEIVE_TIME_MS", elapsed.GetMilliSeconds ())
      //     .log ();
      logger ("metric")
          .add ("offload-session-id", m_offloadSessionIdForThis)
          .add ("connection-id", conn->connId)
          .add ("RESPONSE_DATA_TRANSFER_COMPLETED_MS", Simulator::Now().GetMilliSeconds())
          .log ();
    }
  NS_LOG_INFO (this << " [BulkSendApp] Connection Closed Normally");
}

void
BulkSendApplication2::ErrorCloseCallback (Ptr<Socket> socket)
{
  NS_LOG_INFO (this << " [BulkSendApp] Connection Closed !Normally");
}

void
BulkSendApplication2::SendQuery (void)
{
  QueryHeader header;
  header.SetNodeId (m_node->GetIdx ());
  header.SetSelectionStrategy ((uint16_t) m_serverSelectionStrategy);

  Ptr<Packet> packet = Create<Packet> (header.GetSerializedSize ());
  packet->AddHeader (header);
  m_querysocket->Send (packet);

  // NS_LOG_DEBUG ("DEBUG NodeId=" << m_node->GetIdx ()
  //                               << " Sent node ranking query to the schedular \n");
  // NS_LOG_INFO("[Tx Query] swid=" << header.GetNodeId());
  //   logger ("debug")
  // .add("offload-session-id", m_offloadSessionIdForThis)
  //   .add ("Ranking query sent to the scheduler, waiting for the response", "").log ();

  logger ("metric")
      .add ("offload-session-id", m_offloadSessionIdForThis)
      .add ("QUERY_INITIATED_MS", Simulator::Now ().GetMilliSeconds ())
      .log();
}

void
BulkSendApplication2::QueryResponseHandler (Ptr<Socket> socket)
{
  logger ("metric")
      .add ("offload-session-id", m_offloadSessionIdForThis)
      .add ("QUERY_RESPONSE_RCVD_MS", Simulator::Now ().GetMilliSeconds ())
      .log();

  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  std::vector<std::tuple<int, int>> results;
  while ((packet = socket->RecvFrom (from)))
    {
      socket->GetSockName (localAddress);
      if (packet->GetSize () > 0)
        {
          QueryResponse response;
          packet->RemoveHeader (response);
          // NS_LOG_DEBUG ("DEBUG NodeId=" << m_node->GetIdx () << " " << response.GetCount () << " "
          //                               << "responses received from schedular\n");
          // NS_LOG_DEBUG("[Rx Query Respone (sender)] count=" << response.GetCount());
          // std::cout << Simulator::Now () << " Response received " << response.GetCount ()
          //           << std::endl
          //           << std::flush;
          // logger ("debug")
          // .add("offload-session-id", m_offloadSessionIdForThis)
          // .add ("Total result count in response", response.GetCount ()).log ();
          QueryValue value;
          for (int i = 0; i < response.GetCount (); i++)
            {
              packet->RemoveHeader (value);
              // NS_LOG_DEBUG ("DEBUG NodeId=" << m_node->GetIdx ()
              //                               << " { nodeid=" << value.GetNodeId ()
              //                               << " value=" << value.GetValue () << "}");
              logger ("debug").add (std::to_string (value.GetNodeId ()), value.GetValue ()).log ();
              results.push_back (std::make_tuple (value.GetNodeId (), value.GetValue ()));
            }
        }
    }

  if (results.size () > 0)
    {
      std::vector<int> nodes;
      // offload to requested amount of servers
      MyLogger _log = logger ("debug")
                          .add ("offload-session-id", m_offloadSessionIdForThis)
                          .add ("selection strategy",
                                (m_serverSelectionStrategy == 0
                                     ? "Optimal"
                                     : (m_serverSelectionStrategy == 1 ? "Near" : "Random")));
      if (m_serverSelectionStrategy == OF_SELECTION_STRATEGY_OPTIMAL ||
          m_serverSelectionStrategy == OF_SELECTION_STRATEGY_NEAR)
        {
          for (int i = 0; i < m_totalServersToOffload; i++)
            {
              //std::cout << std::get<0> (results.at (i)) << " " << std::get<1> (results.at (i)) << std::flush;
              // if((int)this->GetNode()->GetIdx() == results.at(i)) continue;
              int nodeid = std::get<0> (results.at (i));
              if(nodeid == (int)this->GetNode()->GetIdx()) continue;
              nodes.push_back (nodeid);
              _log.add ("->", nodeid);
            }
        }
      else
        {
          // means we are doing random selection
          std::map<int, int *> selection;
          while (nodes.size () < m_totalServersToOffload)
            {
              int g = std::experimental::randint (0, (int) (results.size () - 1));
              if (selection[g] == NULL)
                {
                  selection[g] = new int;
                  _log.add ("->", g);
                  nodes.push_back (g);
                }
            }
        }

      // log the selection

      logger ("debug")
          .add ("offload-session-id", m_offloadSessionIdForThis)
          .add ("Total number of servers to offload", m_totalServersToOffload)
          .log ();

      _log.log ();
      // means we have results
      Simulator::Schedule (Seconds (0.0), &BulkSendApplication2::DataTransferSequence, this, nodes);
    }
}

void
BulkSendApplication2::QuerySequence (void)
{
  Simulator::Schedule (Seconds (0.0), &BulkSendApplication2::SendQuery, this);
}

void
BulkSendApplication2::DataTransferSequence (std::vector<int> nodes)
{
  // lets get the adress of that specific node
  Ipv4InterfaceContainer *ip = ((Ipv4InterfaceContainer *) m_node->getTerminalIps ());
  if (ip == NULL)
    {
      NS_LOG_DEBUG ("DEBUG NodeId=" << m_node->GetIdx () << " Interfaces not set on the Node");
    }
  //  ns3::Address address = ns3::Address(ip->GetAddress(nodeId));
  for (int node : nodes)
    {
#if 1
      std::stringstream peerAddressStringStream, p2;
      peerAddressStringStream << Ipv4Address::ConvertFrom (ip->GetAddress (node));
      //  std::cout << " " << nodeId << " = " << peerAddressStringStream.str () << std::endl;
#endif

      // assign this to the peer that we need to connect to
      //  m_peer = ns3::Address(ip->GetAddress(nodeId));
      Address m_peer_address = (InetSocketAddress (ip->GetAddress (node), 12345));
      // and then initiate the transfer sequence
      // NS_LOG_DEBUG ("DEBUG NodeId=" << m_node->GetIdx () << " Initiating transfer to Node(" << nodeId
      // << ") IP=" << peerAddressStringStream.str ());
      // logger ("debug")
      //     .add ("offload-session-id", m_offloadSessionIdForThis)
      //     .add ("Initiating data tranfer to ", peerAddressStringStream.str ())
      //     .log ();

      Simulator::Schedule (Seconds (0.0), &BulkSendApplication2::InitTransferN, this,
                           m_peer_address);
    }
}

void
BulkSendApplication2::InitTransferN (Address &address)
{
  // since we are to start the transfer now, so we have to create socket now
  Ptr<Socket> currentSocket = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId ());
  int ret = -1;
  if (currentSocket->GetSocketType () != Socket::NS3_SOCK_STREAM &&
      currentSocket->GetSocketType () != Socket::NS3_SOCK_SEQPACKET)
    {
      NS_FATAL_ERROR ("Using BulkSend with an incompatible socket type. "
                      "BulkSend requires SOCK_STREAM or SOCK_SEQPACKET. "
                      "In other words, use TCP instead of UDP.");
    }

  if (Inet6SocketAddress::IsMatchingType (address))
    {
      ret = currentSocket->Bind6 ();
    }
  else if (InetSocketAddress::IsMatchingType (address))
    {
      ret = currentSocket->Bind ();
    }
  if (ret == -1)
    {
      NS_FATAL_ERROR ("Failed to bind socket");
    }
  // now we can publish the socket
  OffloadConnection *conn = new OffloadConnection ();
  conn->connId = (++BulkSendApplication2::connectionId);
  m_connections[(void *) &(*currentSocket)] = conn;

  currentSocket->Connect (address);
  // m_socket->ShutdownRecv ();
  currentSocket->SetConnectCallback (
      MakeCallback (&BulkSendApplication2::ConnectionSucceeded, this),
      MakeCallback (&BulkSendApplication2::ConnectionFailed, this));
  currentSocket->SetCloseCallbacks (MakeCallback (&BulkSendApplication2::NormalCloseCallback, this),
                                    MakeCallback (&BulkSendApplication2::ErrorCloseCallback, this));
  currentSocket->SetSendCallback (MakeCallback (&BulkSendApplication2::DataSend, this));
  currentSocket->SetRecvCallback (MakeCallback (&BulkSendApplication2::ReceivedDataCallback, this));
}

} // Namespace ns3
