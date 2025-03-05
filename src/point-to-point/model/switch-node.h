#ifndef SWITCH_NODE_H
#define SWITCH_NODE_H

#include <unordered_map>
#include <ns3/node.h>
#include "qbb-net-device.h"
#include "switch-mmu.h"
#include "pint.h"

namespace ns3 {

class Packet;

class SwitchNode : public Node{
	static const uint32_t pCnt = 257;	// Number of ports used
	static const uint32_t qCnt = 8;		// Number of queues/priorities used
	uint32_t m_ecmpSeed;			// 用于计算hash值。ECMP=等价多路径路由
	std::unordered_map<uint32_t, std::vector<int> > m_rtTable; // map from ip address (u32) to possible ECMP port (index of dev) 可能是IP路由表

	// monitor of PFC
	uint32_t m_bytes[pCnt][pCnt][qCnt]; 	// m_bytes[inDev][outDev][qidx] 是来自inDev的bytes，在 qidx排队等待 outDev 
	
	uint64_t m_txBytes[pCnt]; 	// counter of tx bytes

	uint32_t m_lastPktSize[pCnt];
	uint64_t m_lastPktTs[pCnt]; 	// ns
	double m_u[pCnt];


	// RDMA NPA
	static const uint32_t flowHashSeed = 0x233;	// Seed for flow hash
	static const uint32_t flowEntryNum = (1 << 10);	// Number of flowTelemetryData entries
	static const uint32_t epoch = 1000000;		// 可能是时间戳
	static const uint32_t epochNum = 2;	
	static const uint32_t egressThreshold = 64 * 1024;	// signal threshold 信号阈值
	static const uint32_t rateThreshold = 1024 * 1024 * 1024 * epoch / 1000000000 / 8;	// rate threshold??	1Gbps
	// static const uint32_t pausedPacketThreshold = 10;	// pfc paused packet threshold
	static const uint32_t portToPortSlot = 5;		// port to port bytes slot
	uint64_t m_lastSignalEpoch;		// last signal time
	uint32_t m_slotIdx;			// current epoch index
	uint64_t m_lastPollingEpoch[pCnt];	// last polling epoch

	struct FiveTuple{		// 五元组
		uint32_t srcIp;			// 源ip
		uint32_t dstIp;			// 目的ip
		uint16_t srcPort;		// 源端口
		uint16_t dstPort;		// 目的端口
		uint8_t protocol;		// 协议
		bool operator==(const FiveTuple &other) const{
			return srcIp == other.srcIp
				&& dstIp == other.dstIp 
				&& srcPort == other.srcPort 
				&& dstPort == other.dstPort 
				&& protocol == other.protocol;
		}
	};
	
	struct FlowTelemetryData{	// 流水平遥测数据[五元组哈希值]
		uint16_t minSeq;           	// 16-bit min_seq		序列号范围
		uint16_t maxSeq;           	// 16-bit max_seq		序列号范围
		uint32_t packetNum;		// 32-bit packet_num		数据包数量
		uint32_t enqQdepth;		// 32-bit enq_q_depth		总排队深度
		uint32_t pfcPausedPacketNum;	// 32-bit pfc_paused_packet_num	PFC暂停包数量

		FiveTuple flowTuple;		// 5-tuple			五元组
		uint64_t lastTimeStep;		// last timestep
	};
	struct PortTelemetryData{	// 端口水平遥测数据[端口号]
		uint32_t enqQdepth;		// 32-bit enq_q_depth		出口队列长度
		uint32_t pfcPausedPacketNum;	//				PFC暂停包数量

		uint32_t lastTimeStep;		// last timestep >> 5
	};
	FlowTelemetryData m_flowTelemetryData[pCnt][epochNum][flowEntryNum]; 	// flow telemetry data
	PortTelemetryData m_portTelemetryData[epochNum][pCnt]; 			// port telemetry data
	uint32_t m_portToPortBytes[pCnt][pCnt]; 			// bytes from port to port

	uint32_t m_portToPortBytesSlot[pCnt][pCnt][portToPortSlot]; 	// port to port bytes slot

protected:
	bool m_ecnEnabled;
	uint32_t m_ccMode;
	uint64_t m_maxRtt;

	uint32_t m_ackHighPrio; // set high priority for ACK/NACK

private:
	int GetOutDev(Ptr<const Packet>, CustomHeader &ch);				// 根据目的ip等，返回下一跳出口的端口号
	void SendToDev(Ptr<Packet>p, CustomHeader &ch);					// 从队列中取出数据包并发送。根据数据包，更新下一跳端口的各类遥测数据和端口字节数据
	static uint32_t EcmpHash(const uint8_t* key, size_t len, uint32_t seed);	// 计算hash值。
	void CheckAndSendPfc(uint32_t inDev, uint32_t qIndex);				// 尝试设置暂停状态，并发送pfc Pause包。
	void CheckAndSendResume(uint32_t inDev, uint32_t qIndex);			// 尝试取消暂停状态，并发送pfc Resume包。
	// RDMA NPA
	static uint32_t FiveTupleHash(const FiveTuple &fiveTuple);
	static uint32_t GetEpochIdx();

public:
	Ptr<SwitchMmu> m_mmu;		// 可能是内存管理单元

	static TypeId GetTypeId (void);
	SwitchNode();
	void SetEcmpSeed(uint32_t seed);
	void AddTableEntry(Ipv4Address &dstAddr, uint32_t intf_idx);	// 在IP路由表的dstAddr.ip项中，加入一个值intf_idx
	void ClearTable();
	bool SwitchReceiveFromDevice(Ptr<NetDevice> device, Ptr<Packet> packet, CustomHeader &ch); // 根据数据包，更新下一跳端口的各类遥测数据和端口字节数据
	void SwitchNotifyDequeue(uint32_t ifIndex, uint32_t qIndex, Ptr<Packet> p);		   // 通知交换机，数据包p已经从队列中出队

	// for approximate calc in PINT
	int logres_shift(int b, int l);
	int log2apprx(int x, int b, int m, int l); // given x of at most b bits, use most significant m bits of x, calc the result in l bits

	// for RDMA NPA detect
	FILE *fp_telemetry = NULL;	// 文件名为telemetry_x.txt，其中x=node_number，在third.cc中有定义
};

} /* namespace ns3 */

#endif /* SWITCH_NODE_H */
