#ifndef MYRDMA_DRIVER_H
#define MYRDMA_DRIVER_H

#include <ns3/node.h>
#include <ns3/mycsma-net-device.h>
#include <ns3/rdma.h>
#include "myrdma-queue-pair.h"
#include "myrdma-hw.h"
#include <vector>
#include <unordered_map>

namespace ns3 {

class MyRdmaDriver : public Object {
public:
	Ptr<Node> m_node;
	Ptr<MyRdmaHw> m_rdma;

	// trace
	TracedCallback<Ptr<MyRdmaQueuePair> > m_traceQpComplete;

	static TypeId GetTypeId (void);
	MyRdmaDriver();

	// This function init the m_nic according to the NetDevice
	// So this must be called after all NICs are installed
	void Init(void);

	// Set Node
	void SetNode(Ptr<Node> node);

	// Set RdmaHw
	void SetRdmaHw(Ptr<MyRdmaHw> rdma);

	// add a queue pair
	void AddQueuePair(uint64_t size, uint16_t pg, Ipv4Address _sip, Ipv4Address _dip, uint16_t _sport, uint16_t _dport, uint32_t win, uint64_t baseRtt, Callback<void> notifyAppFinish);

	// callback when qp completes
	void QpComplete(Ptr<MyRdmaQueuePair> q);
};

} // namespace ns3

#endif /* MyRdma_DRIVER_H */
