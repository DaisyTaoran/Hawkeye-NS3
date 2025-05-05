
#ifndef RDMA_BURSTER_HELPER_H
#define RDMA_BURSTER_HELPER_H

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/rdma-burster.h"

namespace ns3 {


class RdmaBursterHelper
{

public:
  RdmaBursterHelper ();

  RdmaBursterHelper (uint16_t pg, Ipv4Address sip, Ipv4Address dip, uint16_t sport, uint16_t dport, uint64_t size, uint32_t win, uint64_t baseRtt);

  void SetAttribute (std::string name, const AttributeValue &value);

  ApplicationContainer Install (NodeContainer c);

private:
  ObjectFactory m_factory;
};

} // namespace ns3

#endif /* RDMA_BURSTER_HELPER_H */
