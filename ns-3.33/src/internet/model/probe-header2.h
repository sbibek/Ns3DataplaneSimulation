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

#ifndef PROBE_HEADER2_H
#define PROBE_HEADER2_H

#include "ns3/header.h"
#include "ns3/nstime.h"

namespace ns3 {
class ProbeHeader2 : public Header
{
public:
  ProbeHeader2 ();

  /**
   * \param seq the sequence number
   */
  void SetCount (uint16_t);
  /**
   * \return the sequence number
   */
  uint16_t GetCount (void) const;

  void SetSwid(uint16_t);
  uint16_t GetSwid(void);

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

private:
  uint16_t m_swid;  // -- switch id
  uint16_t m_count; // -- count corresponds to number of ports in the switch 
};

} // namespace ns3

#endif /* SEQ_TS_HEADER_H */
