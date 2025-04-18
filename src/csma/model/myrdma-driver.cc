#include "myrdma-driver.h"
#include "ns3/ipv4.h"

namespace ns3 {

/***********************
 * MyRdmaDriver
 **********************/
TypeId MyRdmaDriver::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::MyRdmaDriver")
		.SetParent<Object> ()
		.AddTraceSource ("QpComplete", "A qp completes.",
				MakeTraceSourceAccessor (&MyRdmaDriver::m_traceQpComplete))
		;
	return tid;
}

MyRdmaDriver::MyRdmaDriver(){
}

void MyRdmaDriver::Init(void){
	Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4> ();
	#if 0
	m_rdma->m_nic.resize(ipv4->GetNInterfaces());
	for (uint32_t i = 0; i < m_rdma->m_nic.size(); i++){
		m_rdma->m_nic[i] = CreateObject<MyRdmaQueuePairGroup>();
		// share the queue pair group with NIC
		if (ipv4->GetNetDevice(i)->IsCsma()){
			DynamicCast<MyCsmaNetDevice>(ipv4->GetNetDevice(i))->m_rdmaEQ->m_qpGrp = m_rdma->m_nic[i];
		}
	}
	#endif
	for (uint32_t i = 0; i < m_node->GetNDevices(); i++){
		Ptr<MyCsmaNetDevice> dev = NULL;
		if (m_node->GetDevice(i)->IsCsma())
			dev = DynamicCast<MyCsmaNetDevice>(m_node->GetDevice(i));
		m_rdma->m_nic.push_back(MyRdmaInterfaceMgr(dev));
		m_rdma->m_nic.back().qpGrp = CreateObject<MyRdmaQueuePairGroup>();
	}
	#if 0
	for (uint32_t i = 0; i < ipv4->GetNInterfaces (); i++){
		if (ipv4->GetNetDevice(i)->IsCsma() && ipv4->IsUp(i)){
			Ptr<MyCsmaNetDevice> dev = DynamicCast<MyCsmaNetDevice>(ipv4->GetNetDevice(i));
			// add a new RdmaInterfaceMgr for this device
			m_rdma->m_nic.push_back(RdmaInterfaceMgr(dev));
			m_rdma->m_nic.back().qpGrp = CreateObject<MyRdmaQueuePairGroup>();
		}
	}
	#endif
	// MyRdmaHw do setup
	m_rdma->SetNode(m_node);
	m_rdma->Setup(MakeCallback(&MyRdmaDriver::QpComplete, this));
}

void MyRdmaDriver::SetNode(Ptr<Node> node){
	m_node = node;
}

void MyRdmaDriver::SetRdmaHw(Ptr<MyRdmaHw> rdma){
	m_rdma = rdma;
}

void MyRdmaDriver::AddQueuePair(uint64_t size, uint16_t pg, Ipv4Address sip, Ipv4Address dip, uint16_t sport, uint16_t dport, uint32_t win, uint64_t baseRtt, Callback<void> notifyAppFinish){
	m_rdma->AddQueuePair(size, pg, sip, dip, sport, dport, win, baseRtt, notifyAppFinish);
}

void MyRdmaDriver::QpComplete(Ptr<MyRdmaQueuePair> q){
	m_traceQpComplete(q);
}

} // namespace ns3
