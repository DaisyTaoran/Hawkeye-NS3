#include <stdint.h>
#include <iostream>
#include "bth-header.h"
#include "ns3/buffer.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

NS_LOG_COMPONENT_DEFINE("bthHeader");

namespace ns3 {

	NS_OBJECT_ENSURE_REGISTERED(bthHeader);
	
	bthHeader::bthHeader(uint16_t pg)
		: m_pg(pg), flags(0), m_seq(0), m_key(0xffff)
	{
		if (IntHeader::mode == 1)
			ih.ts = Simulator::Now().GetTimeStep();
	}

	bthHeader::bthHeader()
		: m_pg(0), flags(0), m_seq(0), m_key(0xffff)
	{
		if (IntHeader::mode == 1)
			ih.ts = Simulator::Now().GetTimeStep();
	}

	bthHeader::~bthHeader()
	{}

	void bthHeader::SetPG(uint16_t pg)
	{
		m_pg = pg;
	}

	void bthHeader::SetKey(uint16_t key)
	{
		m_key = key;
	}
	
	void bthHeader::SetSeq(uint32_t seq)
	{
		m_seq = seq;
	}
	
	void bthHeader::SetTs(uint64_t ts){
		NS_ASSERT_MSG(IntHeader::mode == 1, "bthHeader cannot SetTs when IntHeader::mode != 1");
		ih.ts = ts;
	}
	void bthHeader::SetIntHeader(const IntHeader &_ih){
		ih = _ih;
	}

	uint16_t bthHeader::GetPG() const
	{
		return m_pg;
	}

	uint16_t bthHeader::GetKey() const
	{
		return m_key;
	}

	uint32_t bthHeader::GetSeq() const
	{
		return m_seq;
	}

	uint64_t bthHeader::GetTs() const {
		NS_ASSERT_MSG(IntHeader::mode == 1, "bthHeader cannot GetTs when IntHeader::mode != 1");
		return ih.ts;
	}

	TypeId
		bthHeader::GetTypeId(void)
	{
		static TypeId tid = TypeId("ns3::bthHeader")
			.SetParent<Header>()
			.AddConstructor<bthHeader>()
			;
		return tid;
	}
	TypeId
		bthHeader::GetInstanceTypeId(void) const
	{
		return GetTypeId();
	}
	void bthHeader::Print(std::ostream &os) const
	{
		os << "bth:" << "pg=" << m_pg << ",seq=" << m_seq;
	}
	uint32_t bthHeader::GetSerializedSize(void)  const
	{
		return GetBaseSize() + IntHeader::GetStaticSize(); // 10 Bytes + sizeof(IntHeader)
	}
	uint32_t bthHeader::GetBaseSize() {
		return 12;
	}
	void bthHeader::Serialize(Buffer::Iterator start)  const
	{
		Buffer::Iterator i = start;
		i.WriteU8(0); 		// Opcode: 0 = SEND First
		i.WriteU8(flags);
		i.WriteU16(m_key);
		i.WriteU16(0); 		// reserved = 8b. this 16b is reversed(8) + m_pg(8)
		i.WriteU16(m_pg); 	// m_pg(24b) = 8b + 16b, 8b in reserved
		i.WriteU32(m_seq);	// A(1b) + Reserves(7b) + PSNSep(24b) = 32b

		// write IntHeader
		ih.Serialize(i);
	}

	uint32_t bthHeader::Deserialize(Buffer::Iterator start)
	{
		Buffer::Iterator i = start;
		i.ReadU8();
		flags = i.ReadU8();
		m_key = i.ReadU16();
		i.ReadU16();
		m_pg = i.ReadU16();
		m_seq = i.ReadU32();

		// read IntHeader
		ih.Deserialize(i);
		return GetSerializedSize();
	}
}; // namespace ns3
