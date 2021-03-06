/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"
#include "probe-header2.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ProbeHeader2");

NS_OBJECT_ENSURE_REGISTERED (ProbeHeader2);

// ---------------------------------------------------

ProbeHeader2::ProbeHeader2 () : m_count (0)
{
  NS_LOG_FUNCTION (this);
}

void
ProbeHeader2::SetCount (uint16_t count)
{
  NS_LOG_FUNCTION (this << count);
  m_count = count;
}
uint16_t
ProbeHeader2::GetCount (void) const
{
  NS_LOG_FUNCTION (this);
  return m_count;
}

void
ProbeHeader2::SetSwid (uint16_t swid)
{
  this->m_swid = swid;
}

uint16_t
ProbeHeader2::GetSwid (void)
{
  return m_swid;
}

TypeId
ProbeHeader2::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ProbeHeader2")
                          .SetParent<Header> ()
                          .SetGroupName ("Applications")
                          .AddConstructor<ProbeHeader2> ();
  return tid;
}
TypeId
ProbeHeader2::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void
ProbeHeader2::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "Count " << m_count;
}
uint32_t
ProbeHeader2::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return 4;
}

void
ProbeHeader2::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  i.WriteHtonU16 (m_swid);
  i.WriteHtonU16 (m_count);
}
uint32_t
ProbeHeader2::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  m_swid = i.ReadNtohU16 ();
  m_count = i.ReadNtohU16 ();
  return GetSerializedSize ();
}

} // namespace ns3
