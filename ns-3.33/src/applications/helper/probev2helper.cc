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
#include "probev2helper.h"
#include "ns3/pscheduler.h"
#include "ns3/probe-app-2.h"
#include "ns3/udp-trace-client.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"

namespace ns3 {

PSchedulerHelper::PSchedulerHelper ()
{
  m_factory.SetTypeId (PScheduler::GetTypeId ());
}

PSchedulerHelper::PSchedulerHelper (uint16_t port)
{
  m_factory.SetTypeId (PScheduler::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void
PSchedulerHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
PSchedulerHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;

      m_server = m_factory.Create<PScheduler> ();
      node->AddApplication (m_server);
      apps.Add (m_server);

    }
  return apps;
}

Ptr<PScheduler>
PSchedulerHelper::GetServer (void)
{
  return m_server;
}


ProbeAppHelper::ProbeAppHelper ()
{
  m_factory.SetTypeId (ProbeApp2::GetTypeId ());
}

ProbeAppHelper::ProbeAppHelper (Address address, uint16_t port, uint16_t totalPacketsToSend, float interval)
{
  m_factory.SetTypeId (ProbeApp2::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
  SetAttribute ("MaxPackets", UintegerValue (totalPacketsToSend));
  SetAttribute ("Interval", TimeValue (Seconds (interval)));
}

ProbeAppHelper::ProbeAppHelper (Address address, uint16_t port)
{
  m_factory.SetTypeId (ProbeApp2::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
}

ProbeAppHelper::ProbeAppHelper (Address address)
{
  m_factory.SetTypeId (ProbeApp2::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
}

void
ProbeAppHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}


ApplicationContainer
ProbeAppHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<ProbeApp2> client = m_factory.Create<ProbeApp2> ();
      node->AddApplication (client);
      apps.Add (client);
    }
  return apps;
}

} // namespace ns3
