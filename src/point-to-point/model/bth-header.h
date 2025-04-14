
#ifndef BTH_HEADER_H
#define BTH_HEADER_H

#include <stdint.h>
#include "ns3/header.h"
#include "ns3/buffer.h"
#include "ns3/int-header.h"

namespace ns3 {
 
class bthHeader : public Header
{
public:
 
  enum {
	  FLAG_CNP = 0
  };
  bthHeader (uint16_t pg);
  bthHeader ();
  virtual ~bthHeader ();

  void SetPG (uint16_t pg);
  void SetKey(uint16_t key);
  void SetSeq(uint32_t seq);
  void SetTs(uint64_t ts);
  void SetIntHeader(const IntHeader &_ih);

  uint16_t GetPG () const;
  uint16_t GetKey () const;
  uint32_t GetSeq() const;
  uint64_t GetTs() const;

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  static uint32_t GetBaseSize(); // size without INT

private:
  uint8_t flags;
  uint16_t m_key;
  uint16_t m_pg;
  uint32_t m_seq; // the qbb sequence number.
  IntHeader ih; // 支持带内网络遥测（INT）的自定义协议头部，用于在数据包中嵌入网络设备的实时状态信息
  
};

}; // namespace ns3

#endif /* BTH_HEADER */
