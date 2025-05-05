#include <stdint.h>
#include <iostream>
#include "mytag-header.h"
#include "ns3/buffer.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("MyTagHeader");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (MyTagHeader);

MyTagHeader::MyTagHeader()
 : m_tag(0) 
{
}

MyTagHeader::MyTagHeader(uint32_t tag) 
 : m_tag(tag) 
{
}

MyTagHeader::~MyTagHeader()
{
}

TypeId 
MyTagHeader::GetTypeId(void) 
{
        static TypeId tid = TypeId("MyTagHeader")
            .SetParent<Header>()
            .AddConstructor<MyTagHeader>();
        return tid;
}

TypeId 
MyTagHeader::GetInstanceTypeId() const 
{ 
	return GetTypeId(); 
}

void 
MyTagHeader::SetTag(uint32_t tag) 
{ 
	m_tag = tag; 
}

uint32_t 
MyTagHeader::GetTag() const 
{ 
	return m_tag; 
}

uint32_t 
MyTagHeader::GetSerializedSize() const 
{ 
	return 4; 
}
    
void 
MyTagHeader::Serialize(Buffer::Iterator start) const 
{
        start.WriteHtonU32(m_tag);
}
    
uint32_t 
MyTagHeader::Deserialize(Buffer::Iterator start) 
{
        m_tag = start.ReadNtohU32();
        return 4;
}
    
void 
MyTagHeader::Print(std::ostream &os) const 
{ 
	os << "Tag=" << m_tag; 
}

}
