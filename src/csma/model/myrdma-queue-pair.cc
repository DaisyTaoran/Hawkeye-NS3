#include <ns3/hash.h>
#include <ns3/uinteger.h>
#include <ns3/seq-ts-header.h>
#include <ns3/udp-header.h>
#include <ns3/ipv4-header.h>
#include <ns3/simulator.h>
#include "ns3/ppp-header.h"
#include "myrdma-queue-pair.h"

namespace ns3 {

/**************************
 * MyRdmaQueuePair
 *************************/
TypeId MyRdmaQueuePair::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::MyRdmaQueuePair")
		.SetParent<Object> ()
		;
	return tid;
}

MyRdmaQueuePair::MyRdmaQueuePair(uint16_t pg, Ipv4Address _sip, Ipv4Address _dip, uint16_t _sport, uint16_t _dport){
	startTime = Simulator::Now();
	sip = _sip;
	dip = _dip;
	sport = _sport;
	dport = _dport;
	m_size = 0;
	snd_nxt = snd_una = 0;
	m_pg = pg;
	m_ipid = 0;
	m_win = 0;
	m_baseRtt = 0;
	m_max_rate = 0;
	m_var_win = false;
	m_rate = 0;
	m_nextAvail = Time(0);
	mlx.m_alpha = 1;
	mlx.m_alpha_cnp_arrived = false;
	mlx.m_first_cnp = true;
	mlx.m_decrease_cnp_arrived = false;
	mlx.m_rpTimeStage = 0;
	hp.m_lastUpdateSeq = 0;
	for (uint32_t i = 0; i < sizeof(hp.keep) / sizeof(hp.keep[0]); i++)
		hp.keep[i] = 0;
	hp.m_incStage = 0;
	hp.m_lastGap = 0;
	hp.u = 1;
	for (uint32_t i = 0; i < IntHeader::maxHop; i++){
		hp.hopState[i].u = 1;
		hp.hopState[i].incStage = 0;
	}

	tmly.m_lastUpdateSeq = 0;
	tmly.m_incStage = 0;
	tmly.lastRtt = 0;
	tmly.rttDiff = 0;

	dctcp.m_lastUpdateSeq = 0;
	dctcp.m_caState = 0;
	dctcp.m_highSeq = 0;
	dctcp.m_alpha = 1;
	dctcp.m_ecnCnt = 0;
	dctcp.m_batchSizeOfAlpha = 0;

	hpccPint.m_lastUpdateSeq = 0;
	hpccPint.m_incStage = 0;

	npa.m_lastPollingTime = 0;
	npa.m_maxRtt = 0;
}

void MyRdmaQueuePair::SetSize(uint64_t size){
	m_size = size;
}

void MyRdmaQueuePair::SetWin(uint32_t win){
	m_win = win;
}

void MyRdmaQueuePair::SetBaseRtt(uint64_t baseRtt){
	m_baseRtt = baseRtt;
}

void MyRdmaQueuePair::SetVarWin(bool v){
	m_var_win = v;
}

void MyRdmaQueuePair::SetAppNotifyCallback(Callback<void> notifyAppFinish){
	m_notifyAppFinish = notifyAppFinish;
}

uint64_t MyRdmaQueuePair::GetBytesLeft(){
	return m_size >= snd_nxt ? m_size - snd_nxt : 0;
}

uint32_t MyRdmaQueuePair::GetHash(void){
	union{
		struct {
			uint32_t sip, dip;
			uint16_t sport, dport;
		};
		char c[12];
	} buf;
	buf.sip = sip.Get();
	buf.dip = dip.Get();
	buf.sport = sport;
	buf.dport = dport;
	return Hash32(buf.c, 12);
}

void MyRdmaQueuePair::Acknowledge(uint64_t ack){
	if (ack > snd_una){
		snd_una = ack;
	}
}

uint64_t MyRdmaQueuePair::GetOnTheFly(){
	return snd_nxt - snd_una;
}

bool MyRdmaQueuePair::IsWinBound(){
	uint64_t w = GetWin();
	return w != 0 && GetOnTheFly() >= w;
}

uint64_t MyRdmaQueuePair::GetWin(){
	if (m_win == 0)
		return 0;
	uint64_t w;
	if (m_var_win){
		w = m_win * m_rate.GetBitRate() / m_max_rate.GetBitRate();
		if (w == 0)
			w = 1; // must > 0
	}else{
		w = m_win;
	}
	return w;
}

uint64_t MyRdmaQueuePair::HpGetCurWin(){
	if (m_win == 0)
		return 0;
	uint64_t w;
	if (m_var_win){
		w = m_win * hp.m_curRate.GetBitRate() / m_max_rate.GetBitRate();
		if (w == 0)
			w = 1; // must > 0
	}else{
		w = m_win;
	}
	return w;
}

bool MyRdmaQueuePair::IsFinished(){
	return snd_una >= m_size;
}

/*********************
 * MyRdmaRxQueuePair
 ********************/
TypeId MyRdmaRxQueuePair::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::MyRdmaRxQueuePair")
		.SetParent<Object> ()
		;
	return tid;
}

MyRdmaRxQueuePair::MyRdmaRxQueuePair(){
	sip = dip = sport = dport = 0;
	m_ipid = 0;
	ReceiverNextExpectedSeq = 0;
	m_nackTimer = Time(0);
	m_milestone_rx = 0;
	m_lastNACK = 0;
}

uint32_t MyRdmaRxQueuePair::GetHash(void){
	union{
		struct {
			uint32_t sip, dip;
			uint16_t sport, dport;
		};
		char c[12];
	} buf;
	buf.sip = sip;
	buf.dip = dip;
	buf.sport = sport;
	buf.dport = dport;
	return Hash32(buf.c, 12);
}

/*********************
 * MyRdmaQueuePairGroup
 ********************/
TypeId MyRdmaQueuePairGroup::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::MyRdmaQueuePairGroup")
		.SetParent<Object> ()
		;
	return tid;
}

MyRdmaQueuePairGroup::MyRdmaQueuePairGroup(void){
}

uint32_t MyRdmaQueuePairGroup::GetN(void){
	return m_qps.size();
}

Ptr<MyRdmaQueuePair> MyRdmaQueuePairGroup::Get(uint32_t idx){
	return m_qps[idx];
}

Ptr<MyRdmaQueuePair> MyRdmaQueuePairGroup::operator[](uint32_t idx){
	return m_qps[idx];
}

void MyRdmaQueuePairGroup::AddQp(Ptr<MyRdmaQueuePair> qp){
	m_qps.push_back(qp);
}

#if 0
void MyRdmaQueuePairGroup::AddRxQp(Ptr<MyRdmaRxQueuePair> rxQp){
	m_rxQps.push_back(rxQp);
}
#endif

void MyRdmaQueuePairGroup::Clear(void){
	m_qps.clear();
}

}
