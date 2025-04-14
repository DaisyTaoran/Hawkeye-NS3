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
#include "seq-ts-header.h"

NS_LOG_COMPONENT_DEFINE ("SeqTsHeader");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SeqTsHeader);

SeqTsHeader::SeqTsHeader ()
  : m_seq (0)
{
	if (IntHeader::mode == 1)
		ih.ts = Simulator::Now().GetTimeStep();
}

void
SeqTsHeader::SetSeq (uint32_t seq)
{
  m_seq = seq;
}
uint32_t
SeqTsHeader::GetSeq (void) const
{
  return m_seq;
}

void
SeqTsHeader::SetPG (uint16_t pg)
{
	m_pg = pg;
}
uint16_t
SeqTsHeader::GetPG (void) const
{
	return m_pg;
}

Time
SeqTsHeader::GetTs (void) const
{
	NS_ASSERT_MSG(IntHeader::mode == 1, "SeqTsHeader cannot GetTs when IntHeader::mode != 1");
	return TimeStep (ih.ts);
}

TypeId
SeqTsHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SeqTsHeader")
    .SetParent<Header> ()
    .AddConstructor<SeqTsHeader> ()
  ;
  return tid;
}
TypeId
SeqTsHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void
SeqTsHeader::Print (std::ostream &os) const
{
  //os << "(seq=" << m_seq << " time=" << TimeStep (m_ts).GetSeconds () << ")";
	//os << m_seq << " " << TimeStep (m_ts).GetSeconds () << " " << m_pg;
	os << m_seq << " " << m_pg;
}
uint32_t
SeqTsHeader::GetSerializedSize (void) const
{
	return GetHeaderSize();
}
uint32_t SeqTsHeader::GetHeaderSize(void){ // static
	return 12 + IntHeader::GetStaticSize();
}

void
SeqTsHeader::Serialize (Buffer::Iterator start) const
{ 
  Buffer::Iterator i = start;
  /* 改 */
  if(isRdma){   
        i.WriteU8(0); 		// Opcode: 0 = SEND First
	i.WriteU8(0);
	i.WriteU16(0xffff);     // Partition Key
	i.WriteU16(0); 		// reserved = 8b. this 16b is reversed(8) + m_pg(8)
	i.WriteHtonU16(m_pg); 	// m_pg(24b) = 8b + 16b, 8b in reserved
	i.WriteHtonU32(m_seq);	// A(1b) + Reserves(7b) + PSNSep(24b) = 32b
  } else {      
        /* 原 */
        i.WriteU32 (0); // new add
        i.WriteHtonU32 (m_seq);
        i.WriteU16(0); // new
        i.WriteHtonU16 (m_pg);
  }
        // write IntHeader
        ih.Serialize(i);
}
uint32_t
SeqTsHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  /* 改 */
  if(isRdma){   
	i.ReadU16();
	i.ReadU32();
	m_pg = i.ReadNtohU16();
	m_seq = i.ReadNtohU32();
  } else {      
        /* 原 */
	i.ReadU32();
        m_seq = i.ReadNtohU32 ();
	i.ReadU16();
        m_pg =  i.ReadNtohU16 ();
        
  }
  // read IntHeader
  ih.Deserialize(i);
  return GetSerializedSize ();
}

} // namespace ns3
