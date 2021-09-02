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
#include "gtcp-client.h"
#include "ns3/probe-header.h"
#include "ns3/probe-payload.h"
#include <cstdlib>
#include <cstdio>
#include <ns3/tcp-socket-factory.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GtcpClient");

NS_OBJECT_ENSURE_REGISTERED (GtcpClient);

TypeId
GtcpClient::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::GtcpClient")
          .SetParent<Application> ()
          .SetGroupName ("Applications")
          .AddConstructor<GtcpClient> ()
          .AddAttribute ("MaxPackets", "The maximum number of packets the application will send",
                         UintegerValue (100), MakeUintegerAccessor (&GtcpClient::m_count),
                         MakeUintegerChecker<uint32_t> ())
          .AddAttribute ("Interval", "The time to wait between packets", TimeValue (Seconds (1.0)),
                         MakeTimeAccessor (&GtcpClient::m_interval), MakeTimeChecker ())
          .AddAttribute ("RemoteAddress", "The destination Address of the outbound packets",
                         AddressValue (), MakeAddressAccessor (&GtcpClient::m_peerAddress),
                         MakeAddressChecker ())
          .AddAttribute ("RemotePort", "The destination port of the outbound packets",
                         UintegerValue (100), MakeUintegerAccessor (&GtcpClient::m_peerPort),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("PacketSize",
                         "Size of packets generated. The minimum packet size is 12 bytes which is "
                         "the size of the header carrying the sequence number and the time stamp.",
                         UintegerValue (1024), MakeUintegerAccessor (&GtcpClient::m_size),
                         MakeUintegerChecker<uint32_t> (12, 65507));
  return tid;
}

GtcpClient::GtcpClient ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_totalTx = 0;
  m_socket = 0;
  m_sendEvent = EventId ();
}

GtcpClient::~GtcpClient ()
{
  NS_LOG_FUNCTION (this);
}

void
GtcpClient::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void
GtcpClient::SetRemote (Address addr)
{
  NS_LOG_FUNCTION (this << addr);
  m_peerAddress = addr;
}

void
GtcpClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
GtcpClient::StartApplication (void)
{
  NS_LOG_INFO("GTCP CLIENT STARTING!!");
  m_socket = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId ());
  int ret = m_socket->Bind ();
  NS_LOG_DEBUG (this << " [GtcpClient] Bind() return value= " << ret
                     << " GetErrNo= " << m_socket->GetErrno () << ".");
  Ipv4Address ipv4 = Ipv4Address::ConvertFrom (m_peerAddress);
  InetSocketAddress inetSocket = InetSocketAddress (ipv4, m_peerPort);
  NS_LOG_INFO (this << " [GtcpClient] connecting to " << ipv4 << "@" << m_peerPort
                    << " GetErrNo=" << m_socket->GetErrno ());
  ret = m_socket->Connect (inetSocket);
  NS_LOG_INFO (this << " [GtcpClient] Connect() returned " << ret
                    << " GetErrNo=" << m_socket->GetErrno ());

  m_socket->SetConnectCallback (MakeCallback (&GtcpClient::ConnectionSucceededCallback, this),
                                MakeCallback (&GtcpClient::ConnectionFailedCallback, this));
  m_socket->SetCloseCallbacks (MakeCallback (&GtcpClient::NormalCloseCallback, this),
                               MakeCallback (&GtcpClient::ErrorCloseCallback, this));
  m_socket->SetRecvCallback (MakeCallback (&GtcpClient::ReceivedDataCallback, this));

  ProbeHeader header;
  header.SetCount (9101);
  Ptr<Packet> pkt = Create<Packet> (header.GetSerializedSize ());
  pkt->AddHeader (header);
  m_socket->Send (pkt);
}

void
GtcpClient::StopApplication (void)
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_sendEvent);
}

uint64_t
GtcpClient::GetTotalTx () const
{
  return m_totalTx;
}

void
GtcpClient::ConnectionSucceededCallback (Ptr<Socket> socket)
{
  NS_LOG_INFO (this << " [GtcpClient] connection successful");
}

void
GtcpClient::ConnectionFailedCallback (Ptr<Socket> socket)
{
  NS_LOG_INFO (this << " [GtcpClient] connection unsuccessful");
}

void
GtcpClient::NormalCloseCallback (Ptr<Socket> socket)
{
  NS_LOG_INFO (this << " [GtcpClient] Normal close");
}

void
GtcpClient::ErrorCloseCallback (Ptr<Socket> socket)
{
  NS_LOG_INFO (this << " [GtcpClient] Error closed");
}

void
GtcpClient::ReceivedDataCallback (Ptr<Socket> socket)
{
  NS_LOG_INFO (this << " [GtcpClient] Data received");

  Ptr<Packet> packet;
  Address from;

  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
        break;
      NS_LOG_INFO (this << " [GtcpClient] A packet of " << packet->GetSize () << " bytes"
                        << " received from " << InetSocketAddress::ConvertFrom (from).GetIpv4 ()
                        << " port " << InetSocketAddress::ConvertFrom (from).GetPort () << " / "
                        << InetSocketAddress::ConvertFrom (from));
      ProbeHeader header;
      packet->RemoveHeader (header);

      NS_LOG_INFO (this << " [GtcpClient] Header count = " << header.GetCount ());
    }
  
  // socket->Close();
}

} // Namespace ns3
