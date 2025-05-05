
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/random-variable.h"
#include "ns3/qbb-net-device.h"
#include "ns3/ipv4-end-point.h"
#include "rdma-burster.h"
#include "ns3/seq-ts-header.h"
#include <ns3/rdma-driver.h>
#include <stdlib.h>
#include <stdio.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RdmaBurster");
NS_OBJECT_ENSURE_REGISTERED (RdmaBurster);

TypeId
RdmaBurster::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RdmaBurster")
    .SetParent<Application> ()
    .AddConstructor<RdmaBurster> ()
    .AddAttribute ("WriteSize",
                   "The number of bytes to write",
                   UintegerValue (10000),
                   MakeUintegerAccessor (&RdmaBurster::m_size),
                   MakeUintegerChecker<uint64_t> ())
    .AddAttribute ("SourceIP",
                   "Source IP",
                   Ipv4AddressValue ("0.0.0.0"),
                   MakeIpv4AddressAccessor (&RdmaBurster::m_sip),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("DestIP",
                   "Dest IP",
                   Ipv4AddressValue ("0.0.0.0"),
                   MakeIpv4AddressAccessor (&RdmaBurster::m_dip),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("SourcePort",
                   "Source Port",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RdmaBurster::m_sport),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("DestPort",
                   "Dest Port",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RdmaBurster::m_dport),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PriorityGroup", "The priority group of this flow",
				   UintegerValue (0),
				   MakeUintegerAccessor (&RdmaBurster::m_pg),
				   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Window",
                   "Bound of on-the-fly packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RdmaBurster::m_win),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("BaseRtt",
                   "Base Rtt",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RdmaBurster::m_baseRtt),
                   MakeUintegerChecker<uint64_t> ())
  ;
  return tid;
}

RdmaBurster::RdmaBurster ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

RdmaBurster::~RdmaBurster ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void RdmaBurster::SetRemote (Ipv4Address ip, uint16_t port)
{
  m_dip = ip;
  m_dport = port;
}

void RdmaBurster::SetLocal (Ipv4Address ip, uint16_t port)
{
  m_sip = ip;
  m_sport = port;
}

void RdmaBurster::SetPG (uint16_t pg)
{
	m_pg = pg;
}

void RdmaBurster::SetSize(uint64_t size){
	m_size = size;
}

void RdmaBurster::Finish(){
	m_node->DeleteApplication(this);
}

void RdmaBurster::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Application::DoDispose ();
}

void RdmaBurster::StartApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  // get RDMA driver and add up queue pair
  Ptr<Node> node = GetNode();
  Ptr<RdmaDriver> rdma = node->GetObject<RdmaDriver>();
  rdma->AddQueuePair(m_size, m_pg, m_sip, m_dip, m_sport, m_dport, m_win, m_baseRtt, MakeCallback(&RdmaBurster::Finish, this));
}

void RdmaBurster::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  // TODO stop the queue pair
}

} // Namespace ns3
