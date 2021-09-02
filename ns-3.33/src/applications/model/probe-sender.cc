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
#include "probe-sender.h"
#include "ns3/probe-header.h"
#include "ns3/probe-payload.h"
#include "query-header.h"
#include "query-response.h"
#include "query-value.h"
#include <cstdlib>
#include <cstdio>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ProbeSender");

NS_OBJECT_ENSURE_REGISTERED (ProbeSender);

TypeId
ProbeSender::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::ProbeSender")
          .SetParent<Application> ()
          .SetGroupName ("Applications")
          .AddConstructor<ProbeSender> ()
          .AddAttribute ("MaxPackets", "The maximum number of packets the application will send",
                         UintegerValue (100), MakeUintegerAccessor (&ProbeSender::m_count),
                         MakeUintegerChecker<uint32_t> ())
          .AddAttribute ("Interval", "The time to wait between packets", TimeValue (Seconds (1)),
                         MakeTimeAccessor (&ProbeSender::m_interval), MakeTimeChecker ())
          .AddAttribute ("RemoteAddress", "The destination Address of the outbound packets",
                         AddressValue (), MakeAddressAccessor (&ProbeSender::m_peerAddress),
                         MakeAddressChecker ())
          .AddAttribute ("RemotePort", "The destination port of the outbound packets",
                         UintegerValue (100), MakeUintegerAccessor (&ProbeSender::m_peerPort),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("PacketSize",
                         "Size of packets generated. The minimum packet size is 12 bytes which is "
                         "the size of the header carrying the sequence number and the time stamp.",
                         UintegerValue (1024), MakeUintegerAccessor (&ProbeSender::m_size),
                         MakeUintegerChecker<uint32_t> (12, 65507));
  return tid;
}

ProbeSender::ProbeSender ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_totalTx = 0;
  m_socket = 0;
  m_sendEvent = EventId ();
}

ProbeSender::~ProbeSender ()
{
  NS_LOG_FUNCTION (this);
}

void
ProbeSender::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void
ProbeSender::SetRemote (Address addr)
{
  NS_LOG_FUNCTION (this << addr);
  m_peerAddress = addr;
}

void
ProbeSender::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
ProbeSender::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);

      if (m_socket->Bind () == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
      m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom (m_peerAddress), m_peerPort));

      // TOBE REMOVED LATER, for test only
      m_querysocket = Socket::CreateSocket (GetNode (), tid);
      m_querysocket->Connect (
          InetSocketAddress (Ipv4Address::ConvertFrom (m_peerAddress), m_peerPort + 1));
      m_querysocket->SetRecvCallback (MakeCallback(&ProbeSender::QueryResponseHandler, this));
    }

  m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());
  m_socket->SetAllowBroadcast (true);
  m_sendEvent = Simulator::Schedule (Seconds (0.0), &ProbeSender::Send, this);
}

void
ProbeSender::StopApplication (void)
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_sendEvent);
}

void
ProbeSender::Send (void)
{
  NS_LOG_FUNCTION (this);

  // send the probe
  SendProbe ();

  // if the total send count is not reached, then reschedule the probing
  // if m_count is zero then always schedule
  if (m_count == 0 || (m_sent < m_count) )
    {
      m_sendEvent = Simulator::Schedule (m_interval, &ProbeSender::Send, this);
    }
}

void
ProbeSender::SendProbe (void)
{
  // creating dummy probe sender for now

  // ProbePayload payload;
  // payload.SetSwid (m_sent);
  // payload.SetGlobalHopLatency (m_sent);
  // payload.SetGlobalQueueDepth (m_sent);
  // payload.SetTotalPackets (m_sent);

  // ProbePayload payload2;
  // payload2.SetSwid (m_sent);
  // payload2.SetGlobalHopLatency (m_sent);
  // payload2.SetGlobalQueueDepth (m_sent);
  // payload2.SetTotalPackets (m_sent);

  ProbeHeader ph;
  ph.SetCount (0);
  Ptr<Packet> p = Create<Packet> (1024);

  // we send two payloads
  // p->AddHeader (payload2);
  // p->AddHeader (payload);
  p->AddHeader (ph);

  std::stringstream peerAddressStringStream;
  peerAddressStringStream << Ipv4Address::ConvertFrom (m_peerAddress);

  if (m_socket->Send (p) >= 0)
    {
      ++m_sent;
      // this means that we were able to send out the packet, lets log what we sent
      NS_LOG_INFO ("[Tx Probe] dst=" << peerAddressStringStream.str () << " hdr.count=" << ph.GetCount ()
                               );
    }
  else
    {
      NS_LOG_INFO ("Error while sending " << m_size << " bytes to "
                                          << peerAddressStringStream.str ());
    }

   // TOBE REMOVED
  //  SendQuery(); 
}


void ProbeSender::SendQuery(void) {
  QueryHeader header;
  header.SetSwid(m_sent);

  Ptr<Packet> packet = Create<Packet>(header.GetSerializedSize());
  packet->AddHeader(header);
  m_querysocket->Send(packet);

  NS_LOG_INFO("[Tx Query] swid=" << header.GetSwid());
}

void ProbeSender::QueryResponseHandler(Ptr<Socket> socket) {
  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  while ((packet = socket->RecvFrom (from)))
    {
      socket->GetSockName (localAddress);
      if (packet->GetSize () > 0) {
          QueryResponse response;
          packet->RemoveHeader(response);

          NS_LOG_INFO("[Rx Query Respone (sender)] count=" << response.GetCount());
          QueryValue value;
          for(int i=0;i<response.GetCount();i++) {
            packet->RemoveHeader(value);
            NS_LOG_INFO("   swid=" << value.GetSwid() << " value=" << value.GetValue());
          }
      }
    }

}

uint64_t
ProbeSender::GetTotalTx () const
{
  return m_totalTx;
}

} // Namespace ns3
