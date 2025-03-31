
#include "analysis-client-server-helper.h"
#include "ns3/analysis-server.h"
#include "ns3/analysis-client.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"

namespace ns3 {

AnalysisServerHelper::AnalysisServerHelper ()
{
}

AnalysisServerHelper::AnalysisServerHelper (uint16_t port)
{
  	m_factory.SetTypeId (AnalysisServer::GetTypeId ());
  	SetAttribute ("Port", UintegerValue (port));
}

void
AnalysisServerHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  	m_factory.Set (name, value);
}


void 
AnalysisServerHelper::SetNextHop(std::map<Ptr<Node>, std::map<Ptr<Node>, std::vector<Ptr<Node>> > > *nexthop){
	m_server->SetNextHop(nexthop);
}

ApplicationContainer
AnalysisServerHelper::Install (NodeContainer c)
{
  	ApplicationContainer apps;
  	for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    	{
      		Ptr<Node> node = *i;

      		m_server = m_factory.Create<AnalysisServer> ();
      		node->AddApplication (m_server);
      		apps.Add (m_server);

    	}
  	return apps;
}

Ptr<AnalysisServer>
AnalysisServerHelper::GetServer (void)
{
  	return m_server;
}



AnalysisClientHelper::AnalysisClientHelper ()
{
}

AnalysisClientHelper::AnalysisClientHelper (Address address, uint16_t port)
{
  	m_factory.SetTypeId (AnalysisClient::GetTypeId ());
  	SetAttribute ("RemoteAddress", AddressValue (address));
  	SetAttribute ("RemotePort", UintegerValue (port));
}

AnalysisClientHelper::AnalysisClientHelper (Ipv4Address address, uint16_t port)
{
  	m_factory.SetTypeId (AnalysisClient::GetTypeId ());
  	SetAttribute ("RemoteAddress", AddressValue (Address(address)));
  	SetAttribute ("RemotePort", UintegerValue (port));
}


void
AnalysisClientHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  	m_factory.Set (name, value);
}

ApplicationContainer
AnalysisClientHelper::Install (NodeContainer c)
{
  	ApplicationContainer apps;
  	for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    	{
      		Ptr<Node> node = *i;
      		Ptr<AnalysisClient> client = m_factory.Create<AnalysisClient> ();
      		node->AddApplication (client);
      		apps.Add (client);
    	}
  	return apps;
}


} // namespace ns3

