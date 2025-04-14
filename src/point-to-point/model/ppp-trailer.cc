
#include <iostream>
#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/trailer.h"
#include "ppp-trailer.h"

NS_LOG_COMPONENT_DEFINE ("PppTrailer");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (PppTrailer);

PppTrailer::PppTrailer ()
{
	m_flag = 0x7e;
	m_fcs = 0;
}

PppTrailer::~PppTrailer ()
{
}

TypeId
PppTrailer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PppTrailer")
    .SetParent<Trailer> ()
    .AddConstructor<PppTrailer> ()
  ;
  return tid;
}

TypeId
PppTrailer::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void 
PppTrailer::Print (std::ostream &os) const
{
  std::string flag;

  switch(m_flag)
    {
    case 0x7e: /* ppp */
      flag = "Point-to-Point Proctol Trail (0x7e)";
      break;/*
    case 0x0057: 
      flag = "IPv6 (0x0057)";
      break;*/
    default:
      NS_ASSERT_MSG (false, "PPP Protocol trail not defined!");
    }
  os << "Point-to-Point Protocol: " << flag; 
}

uint32_t
PppTrailer::GetSerializedSize (void) const
{
	return GetStaticSize();
}
uint32_t PppTrailer::GetStaticSize (void){
	return 3;
}

void
PppTrailer::Serialize (Buffer::Iterator end) const
{
  end.Prev(GetSerializedSize());
  end.WriteHtonU16 (m_fcs);
  end.WriteU8(m_flag);
  
}

uint32_t
PppTrailer::Deserialize (Buffer::Iterator end)
{
  uint32_t size = GetSerializedSize();
  end.Prev(size);
  m_fcs = end.ReadNtohU16 ();
  m_flag = end.ReadU8();
  return size;
}

void
PppTrailer::SetFcs (uint16_t fcs)
{
  m_fcs=fcs;
}

uint16_t
PppTrailer::GetFcs (void)
{
  return m_fcs;
}


} // namespace ns3
