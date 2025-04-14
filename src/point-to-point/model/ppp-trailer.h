
#ifndef PPP_TRAILER_H
#define PPP_TRAILER_H

#include "ns3/trailer.h"

namespace ns3 {

class PppTrailer : public Trailer 
{
public:

  PppTrailer ();

  virtual ~PppTrailer ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual uint32_t GetSerializedSize (void) const;
  static uint32_t GetStaticSize (void);

  void SetFcs (uint16_t fcs);

  uint16_t GetFcs (void);

private:
  
  uint8_t m_flag;
  uint16_t m_fcs; 
  
};

} // namespace ns3


#endif /* PPP_TRAILER_H */
