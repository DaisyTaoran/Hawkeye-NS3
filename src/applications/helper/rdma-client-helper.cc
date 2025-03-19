/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 */
#include "rdma-client-helper.h"
#include "ns3/rdma-client.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"

namespace ns3 {

RdmaClientHelper::RdmaClientHelper ()
{
}

RdmaClientHelper::RdmaClientHelper (uint16_t pg, Ipv4Address sip, Ipv4Address dip, uint16_t sport, uint16_t dport, uint64_t size, uint32_t win, uint64_t baseRtt)
{
	m_factory.SetTypeId (RdmaClient::GetTypeId ());
	SetAttribute ("PriorityGroup", UintegerValue (pg));
	SetAttribute ("SourceIP", Ipv4AddressValue (sip));
	SetAttribute ("DestIP", Ipv4AddressValue (dip));
	SetAttribute ("SourcePort", UintegerValue (sport));
	SetAttribute ("DestPort", UintegerValue (dport));
	SetAttribute ("WriteSize", UintegerValue (size));
	SetAttribute ("Window", UintegerValue (win));
	SetAttribute ("BaseRtt", UintegerValue (baseRtt));
}

void
RdmaClientHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}
/*
// 在      节点2  上：安装分析服务器
Ptr<AnalysisServer> analysisApp = CreateObject<AnalysisServer>();
analysisApp->SetLocal(InetSocketAddress(interfaces.GetAddress(2), 5000)); // 绑定到节点2的IP和端口5000   analysisApp->SetNode(n.Get(2))
nodes.Get(2)->AddApplication(analysisApp);
analysisApp->SetStartTime(Seconds(1.0));
analysisApp->SetStopTime(Seconds(10.0));
// 在交换机节点上：安装UDP客户端，用于向分析服务器发送数据：
UdpEchoClientHelper echoClient(interfaces.GetAddress(2), 5000); // 目标地址为分析服务器的端口5000
echoClient.SetAttribute("MaxPackets", UintegerValue(10));
echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
echoClient.SetAttribute("PacketSize", UintegerValue(1024));

ApplicationContainer clientApps = echoClient.Install(nodes.Get(0));
clientApps.Start(Seconds(2.0));
clientApps.Stop(Seconds(10.0));
*/
ApplicationContainer
RdmaClientHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<RdmaClient> client = m_factory.Create<RdmaClient> ();
      node->AddApplication (client);
      apps.Add (client);
    }
  return apps;
}

} // namespace ns3
