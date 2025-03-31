
#ifndef ANALYSIS_CLIENT_SERVER_HELPER_H
#define ANALYSIS_CLIENT_SERVER_HELPER_H

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/analysis-server.h"
#include "ns3/analysis-client.h"
namespace ns3 {

class AnalysisServerHelper
{
public:
  
  AnalysisServerHelper ();

  AnalysisServerHelper (uint16_t port);

  void SetAttribute (std::string name, const AttributeValue &value);
  void SetNextHop(std::map<Ptr<Node>, std::map<Ptr<Node>, std::vector<Ptr<Node>> > > *nexthop);

  ApplicationContainer Install (NodeContainer c);
  Ptr<AnalysisServer> GetServer (void);
private:
  ObjectFactory m_factory;
  Ptr<AnalysisServer> m_server;
};


class AnalysisClientHelper
{

public:
  
  AnalysisClientHelper ();

  AnalysisClientHelper (Ipv4Address ip, uint16_t port);
  AnalysisClientHelper (Address ip, uint16_t port);
  AnalysisClientHelper (Ipv4Address ip, uint16_t port, uint16_t pg);

  void SetAttribute (std::string name, const AttributeValue &value);

  ApplicationContainer Install (NodeContainer c);

private:
  ObjectFactory m_factory;
};


} // namespace ns3

#endif /* ANALYSIS_CLIENT_SERVER_H */
