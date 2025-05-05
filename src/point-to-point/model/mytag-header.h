
#ifndef MYTAG_HEADER_H
#define MYTAG_HEADER_H

#include "ns3/packet.h"
#include "ns3/header.h"
#include "ns3/buffer.h"

namespace ns3{

class MyTagHeader : public Header 
{
public:
    MyTagHeader();
    MyTagHeader(uint32_t tag);
    virtual ~MyTagHeader();
    
    static TypeId GetTypeId(void);
    
    TypeId GetInstanceTypeId() const;
    
    void SetTag(uint32_t tag) ;
    uint32_t GetTag() const ;

    uint32_t GetSerializedSize() const ;
    
    void Serialize(Buffer::Iterator start) const ;
    
    uint32_t Deserialize(Buffer::Iterator start);
    
    void Print(std::ostream &os) const;

private:
    uint32_t m_tag;
    // m_tag = FlowId
};
/*

// 在发送数据包时添加标签
Ptr<Packet> packet = Create<Packet>(100); // 原始数据包
MyTagHeader tagHeader;
tagHeader.SetTag(1234); // 设置自定义标签（如流量类型ID）
packet->AddHeader(tagHeader); // 添加头部

*/

};

#endif
