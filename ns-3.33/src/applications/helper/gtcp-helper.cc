/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 */
#include "gtcp-helper.h"
#include "ns3/probe-listener.h"
#include "ns3/probe-sender.h"
#include "ns3/udp-trace-client.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"

namespace ns3 {

GtcpServerHelper::GtcpServerHelper ()
{
  m_factory.SetTypeId (GtcpServer::GetTypeId ());
}

GtcpServerHelper::GtcpServerHelper (uint16_t port)
{
  m_factory.SetTypeId (GtcpServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void
GtcpServerHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
GtcpServerHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;

      m_server = m_factory.Create<GtcpServer> ();
      node->AddApplication (m_server);
      apps.Add (m_server);

    }
  return apps;
}

Ptr<GtcpServer>
GtcpServerHelper::GetServer (void)
{
  return m_server;
}

GtcpClientHelper::GtcpClientHelper ()
{
  m_factory.SetTypeId (GtcpClient::GetTypeId ());
}

GtcpClientHelper::GtcpClientHelper (Address address, uint16_t port)
{
  m_factory.SetTypeId (GtcpClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
}

GtcpClientHelper::GtcpClientHelper (Address address)
{
  m_factory.SetTypeId (GtcpClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
}

void
GtcpClientHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
GtcpClientHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<GtcpClient> client = m_factory.Create<GtcpClient> ();
      node->AddApplication (client);
      apps.Add (client);
    }
  return apps;
}

GtcpTraceHelper::GtcpTraceHelper ()
{
  m_factory.SetTypeId (UdpTraceClient::GetTypeId ());
}

GtcpTraceHelper::GtcpTraceHelper (Address address, uint16_t port, std::string filename)
{
  m_factory.SetTypeId (UdpTraceClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
  SetAttribute ("TraceFilename", StringValue (filename));
}

GtcpTraceHelper::GtcpTraceHelper (Address address, std::string filename)
{
  m_factory.SetTypeId (UdpTraceClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("TraceFilename", StringValue (filename));
}

void
GtcpTraceHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
GtcpTraceHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<UdpTraceClient> client = m_factory.Create<UdpTraceClient> ();
      node->AddApplication (client);
      apps.Add (client);
    }
  return apps;
}

} // namespace ns3
