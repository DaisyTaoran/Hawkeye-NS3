#ifndef MYRDMA_HW_H
#define MYRDMA_HW_H

#include <ns3/rdma.h>
#include <ns3/node.h>
#include <ns3/custom-header.h>
#include "myrdma-queue-pair.h"
#include "mycsma-net-device.h"
#include <unordered_map>
#include "ns3/pint.h"

#include <ns3/find-root-cal.h>

namespace ns3 {

struct MyRdmaInterfaceMgr{
	Ptr<MyCsmaNetDevice> dev;
	Ptr<MyRdmaQueuePairGroup> qpGrp;

	MyRdmaInterfaceMgr() : dev(NULL), qpGrp(NULL) {}
	MyRdmaInterfaceMgr(Ptr<MyCsmaNetDevice> _dev){
		dev = _dev;
	}
};

class MyRdmaHw : public Object {
public:

	static TypeId GetTypeId (void);
	MyRdmaHw();

	Ptr<Node> m_node;
	DataRate m_minRate;		//< Min sending rate
	uint32_t m_mtu;
	uint32_t m_cc_mode;
	double m_nack_interval;
	uint32_t m_chunk;
	uint32_t m_ack_interval;
	bool m_backto0;
	bool m_var_win, m_fast_react;
	bool m_rateBound;
	std::vector<MyRdmaInterfaceMgr> m_nic; // list of running nic controlled by this MyRdmaHw
	std::unordered_map<uint64_t, Ptr<MyRdmaQueuePair> > m_qpMap; // mapping from uint64_t to qp
	std::unordered_map<uint64_t, Ptr<MyRdmaRxQueuePair> > m_rxQpMap; // mapping from uint64_t to rx qp
	std::unordered_map<uint32_t, std::vector<int> > m_rtTable; // map from ip address (u32) to possible ECMP port (index of dev)

	// qp complete callback
	typedef Callback<void, Ptr<MyRdmaQueuePair> > QpCompleteCallback;
	QpCompleteCallback m_qpCompleteCallback;

	void SetNode(Ptr<Node> node);
	void Setup(QpCompleteCallback cb); // setup shared data and callbacks with the MyCsmaNetDevice 用MakeCallback绑定了MyCsmaNetDevice的Callback对象们
	static uint64_t GetQpKey(uint32_t dip, uint16_t sport, uint16_t pg); // get the lookup key for m_qpMap
	Ptr<MyRdmaQueuePair> GetQp(uint32_t dip, uint16_t sport, uint16_t pg); // get the qp
	uint32_t GetNicIdxOfQp(Ptr<MyRdmaQueuePair> qp); // get the NIC index of the qp
	void AddQueuePair(uint64_t size, uint16_t pg, Ipv4Address _sip, Ipv4Address _dip, uint16_t _sport, uint16_t _dport, uint32_t win, uint64_t baseRtt, Callback<void> notifyAppFinish); // add a new qp (new send)
	void DeleteQueuePair(Ptr<MyRdmaQueuePair> qp);

	Ptr<MyRdmaRxQueuePair> GetRxQp(uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport, uint16_t pg, bool create); // get a rxQp
	uint32_t GetNicIdxOfRxQp(Ptr<MyRdmaRxQueuePair> q); // get the NIC index of the rxQp
	void DeleteRxQp(uint32_t dip, uint16_t pg, uint16_t dport);

	int ReceiveUdp(Ptr<Packet> p, CustomHeader &ch);
	int ReceiveCnp(Ptr<Packet> p, CustomHeader &ch); // 用于处理接收到的CNP（Congestion Notification Packet，拥塞通知包）
	int ReceiveAck(Ptr<Packet> p, CustomHeader &ch); // handle both ACK and NACK 检测到性能下降后，设置轮询包。即实现agent功能
	int ReceiveSignal(Ptr<Packet> p, CustomHeader &ch);
	int Receive(Ptr<Packet> p, CustomHeader &ch); // callback function that the MyCsmaNetDevice should use when receive packets. Only NIC can call this function. And do not call this upon PFC

	void CheckandSendQCN(Ptr<MyRdmaRxQueuePair> q);
	int ReceiverCheckSeq(uint32_t seq, Ptr<MyRdmaRxQueuePair> q, uint32_t size);
	void AddHeader (Ptr<Packet> p, uint16_t protocolNumber);
	void AddHeader (Ptr<Packet> p, uint16_t protocolNumber, Address src, Address dst);
	static uint16_t EtherToPpp (uint16_t protocol);

	void RecoverQueue(Ptr<MyRdmaQueuePair> qp);
	void QpComplete(Ptr<MyRdmaQueuePair> qp);
	void SetLinkDown(Ptr<MyCsmaNetDevice> dev);

	// call this function after the NIC is setup
	void AddTableEntry(Ipv4Address &dstAddr, uint32_t intf_idx);
	void ClearTable();
	void RedistributeQp();

	Ptr<Packet> GetNxtPacket(Ptr<MyRdmaQueuePair> qp); // get next packet to send, inc snd_nxt
	void PktSent(Ptr<MyRdmaQueuePair> qp, Ptr<Packet> pkt, Time interframeGap);
	void UpdateNextAvail(Ptr<MyRdmaQueuePair> qp, Time interframeGap, uint32_t pkt_size);
	void ChangeRate(Ptr<MyRdmaQueuePair> qp, DataRate new_rate);
	/******************************
	 * Mellanox's version of DCQCN
	 *****************************/
	double m_g; //feedback weight
	double m_rateOnFirstCNP; // the fraction of line rate to set on first CNP
	bool m_EcnClampTgtRate;
	double m_rpgTimeReset;
	double m_rateDecreaseInterval;
	uint32_t m_rpgThreshold;
	double m_alpha_resume_interval;
	DataRate m_rai;		//< Rate of additive increase
	DataRate m_rhai;		//< Rate of hyper-additive increase

	// the Mellanox's version of alpha update:
	// every fixed time slot, update alpha.
	void UpdateAlphaMlx(Ptr<MyRdmaQueuePair> q);
	void ScheduleUpdateAlphaMlx(Ptr<MyRdmaQueuePair> q);

	// Mellanox's version of CNP receive
	void cnp_received_mlx(Ptr<MyRdmaQueuePair> q);

	// Mellanox's version of rate decrease
	// It checks every m_rateDecreaseInterval if CNP arrived (m_decrease_cnp_arrived).
	// If so, decrease rate, and reset all rate increase related things
	void CheckRateDecreaseMlx(Ptr<MyRdmaQueuePair> q);
	void ScheduleDecreaseRateMlx(Ptr<MyRdmaQueuePair> q, uint32_t delta);

	// Mellanox's version of rate increase
	void RateIncEventTimerMlx(Ptr<MyRdmaQueuePair> q);
	void RateIncEventMlx(Ptr<MyRdmaQueuePair> q);
	void FastRecoveryMlx(Ptr<MyRdmaQueuePair> q);
	void ActiveIncreaseMlx(Ptr<MyRdmaQueuePair> q);
	void HyperIncreaseMlx(Ptr<MyRdmaQueuePair> q);

	/***********************
	 * High Precision CC
	 ***********************/
	double m_targetUtil;
	double m_utilHigh;
	uint32_t m_miThresh;
	bool m_multipleRate;
	bool m_sampleFeedback; // only react to feedback every RTT, or qlen > 0
	void HandleAckHp(Ptr<MyRdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch);
	void UpdateRateHp(Ptr<MyRdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch, bool fast_react);
	void UpdateRateHpTest(Ptr<MyRdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch, bool fast_react);
	void FastReactHp(Ptr<MyRdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch);

	/**********************
	 * TIMELY
	 *********************/
	double m_tmly_alpha, m_tmly_beta;
	uint64_t m_tmly_TLow, m_tmly_THigh, m_tmly_minRtt;
	void HandleAckTimely(Ptr<MyRdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch);
	void UpdateRateTimely(Ptr<MyRdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch, bool us);
	void FastReactTimely(Ptr<MyRdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch);

	/**********************
	 * DCTCP
	 *********************/
	DataRate m_dctcp_rai;
	void HandleAckDctcp(Ptr<MyRdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch);

	/*********************
	 * HPCC-PINT
	 ********************/
	uint32_t pint_smpl_thresh;
	void SetPintSmplThresh(double p);
	void HandleAckHpPint(Ptr<MyRdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch);
	void UpdateRateHpPint(Ptr<MyRdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch, bool fast_react);

	//MyRdma NPA
	bool m_agent_flag;
	
	// Analysis node
	bool m_analysis_flag;
	Ptr<FindRootCal> analys_app = NULL;
	std::map<Ptr<Node>, std::map<Ptr<Node>, std::vector<Ptr<Node>> > > *nextHop = NULL;
};

} /* namespace ns3 */

#endif /* MyRdma_HW_H */
