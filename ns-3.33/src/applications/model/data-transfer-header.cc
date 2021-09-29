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
#include "data-transfer-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DataTransferHeader");

NS_OBJECT_ENSURE_REGISTERED (DataTransferHeader);

DataTransferHeader::DataTransferHeader () : m_totalBytesFollowingThis (0)
{
  NS_LOG_FUNCTION (this);
}

void
DataTransferHeader::SetTotalBytesFollowingThis (uint64_t bytes)
{
  m_totalBytesFollowingThis = bytes;
}
uint64_t
DataTransferHeader::GetTotalBytesFollowingThis (void) const
{
  return m_totalBytesFollowingThis;
}

  void DataTransferHeader::SetConnectionId(uint64_t cid){
    this->m_connectionId = cid;
  }

  uint64_t DataTransferHeader::GetConnectionId(void) const{
    return m_connectionId;
  }


  void DataTransferHeader::SetOffloadSessionId(uint32_t ofid) {
    m_offloadSessionId = ofid;
  }

  uint32_t DataTransferHeader::GetOffloadSessionId(void) const {
    return m_offloadSessionId;
  }

TypeId
DataTransferHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DataTransferHeader")
                          .SetParent<Header> ()
                          .SetGroupName ("Applications")
                          .AddConstructor<DataTransferHeader> ();
  return tid;
}
TypeId
DataTransferHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void
DataTransferHeader::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "Swid " << m_totalBytesFollowingThis;
}
uint32_t
DataTransferHeader::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return 20;
}

void
DataTransferHeader::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  i.WriteHtonU64 (m_totalBytesFollowingThis);
  i.WriteHtonU64 (m_connectionId);
  i.WriteHtonU32(m_offloadSessionId);
}
uint32_t
DataTransferHeader::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  m_totalBytesFollowingThis = i.ReadNtohU64 ();
  m_connectionId = i.ReadNtohU64();
  m_offloadSessionId = i.ReadNtohU32();
  return GetSerializedSize ();
}

} // namespace ns3
