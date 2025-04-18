/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 Emmanuelle Laprise
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
 * Author: Emmanuelle Laprise <emmanuelle.laprise@bluekazoo.ca
 */

#ifndef MYCSMA_NET_DEVICE_H
#define MYCSMA_NET_DEVICE_H

#include <string.h>
#include <ns3/rdma.h>
#include "ns3/node.h"
#include "ns3/backoff.h"
#include "ns3/address.h"
#include "ns3/net-device.h"
#include "ns3/callback.h"
#include "ns3/packet.h"
#include "ns3/traced-callback.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/ptr.h"
#include "ns3/mac48-address.h"
#include "myrdma-queue-pair.h"
#include "ns3/broadcom-egress-queue.h"

namespace ns3 {

class Queue;
class MyCsmaChannel;
class ErrorModel;

class MyRdmaEgressQueue : public Object{
public:
	static const uint32_t qCnt = 8;
	static uint32_t ack_q_idx;
	int m_qlast;
	uint32_t m_rrlast;
	Ptr<DropTailQueue> m_ackQ; // highest priority queue
	Ptr<MyRdmaQueuePairGroup> m_qpGrp; // queue pairs

	// callback for get next packet
	typedef Callback<Ptr<Packet>, Ptr<MyRdmaQueuePair> > RdmaGetNxtPkt;
	RdmaGetNxtPkt m_rdmaGetNxtPkt;

	static TypeId GetTypeId (void);
	MyRdmaEgressQueue();
	Ptr<Packet> DequeueQindex(int qIndex);
	int GetNextQindex(bool paused[]);
	int GetLastQueue();
	uint32_t GetNBytes(uint32_t qIndex);
	uint32_t GetFlowCount(void);
	Ptr<MyRdmaQueuePair> GetQp(uint32_t i);
	void RecoverQueue(uint32_t i);
	void EnqueueHighPrioQ(Ptr<Packet> p);
	void CleanHighPrio(TracedCallback<Ptr<const Packet>, uint32_t> dropCb);

	TracedCallback<Ptr<const Packet>, uint32_t> m_traceRdmaEnqueue;
	TracedCallback<Ptr<const Packet>, uint32_t> m_traceRdmaDequeue;
};


class MyCsmaNetDevice : public NetDevice 
{
public:
  static const uint32_t qCnt = 8;	// Number of queues/priorities used
  static TypeId GetTypeId (void);

  enum EncapsulationMode {
    ILLEGAL,     /**< Encapsulation mode not set */
    DIX,         /**< DIX II / Ethernet II packet */
    LLC,         /**< 802.2 LLC/SNAP Packet*/
  };

  MyCsmaNetDevice ();

  virtual ~MyCsmaNetDevice ();
  
  virtual bool IsCsma (void) const;
  
  void SetDataRate (DataRate bps);
  DataRate GetDataRate();

  void SetInterframeGap (Time t);

  void SetBackoffParams (Time slotTime, uint32_t minSlots, uint32_t maxSlots, 
                         uint32_t maxRetries, uint32_t ceiling);

  bool Attach (Ptr<MyCsmaChannel> ch);

  void SetQueue (Ptr<BEgressQueue> queue);
  Ptr<BEgressQueue> GetQueue (void) const; 

  void SetReceiveErrorModel (Ptr<ErrorModel> em);
  void Receive (Ptr<Packet> p, Ptr<MyCsmaNetDevice> sender);

  bool IsSendEnabled (void);
  void SetSendEnable (bool enable);

  bool IsReceiveEnabled (void);
  void SetReceiveEnable (bool enable);

  void SetEncapsulationMode (MyCsmaNetDevice::EncapsulationMode mode);
  MyCsmaNetDevice::EncapsulationMode  GetEncapsulationMode (void);

  virtual void SetIfIndex (const uint32_t index);
  virtual uint32_t GetIfIndex (void) const;
  virtual Ptr<Channel> GetChannel (void) const;
  virtual bool SetMtu (const uint16_t mtu);
  virtual uint16_t GetMtu (void) const;
  virtual void SetAddress (Address address);
  virtual Address GetAddress (void) const;
  virtual bool IsLinkUp (void) const;
  virtual void AddLinkChangeCallback (Callback<void> callback);
  virtual bool IsBroadcast (void) const;
  virtual Address GetBroadcast (void) const;
  virtual bool IsMulticast (void) const;

   
  virtual Address GetMulticast (Ipv4Address multicastGroup) const;

  virtual bool IsPointToPoint (void) const;
  virtual bool IsBridge (void) const;

  virtual bool Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber);
  virtual bool SwitchSend (uint32_t qIndex, Ptr<Packet> packet, CustomHeader &ch);
   void SendPfc(uint32_t qIndex, uint32_t type); // type: 0 = pause, 1 = resume
 
   //void SetQueue (Ptr<BEgressQueue> q);
   //Ptr<BEgressQueue> GetQueue ();
   void NewQp(Ptr<MyRdmaQueuePair> qp);
   void ReassignedQp(Ptr<MyRdmaQueuePair> qp);
   void TriggerTransmit(void);

  // TODO:RDMA NPA
  bool GetEgressPaused(uint32_t qIndex);
  void SendSignal(uint32_t qIndex, uint32_t rate, uint32_t epoch, uint32_t congestionPort, bool pfcOff);
  void SendAnalysis(uint32_t qIndex, uint32_t rate, uint32_t epoch, Ipv4Address dst_addr);

	TracedCallback<Ptr<const Packet>, uint32_t> m_traceEnqueue;
	TracedCallback<Ptr<const Packet>, uint32_t> m_traceDequeue;
	TracedCallback<Ptr<const Packet>, uint32_t> m_traceDrop;
	TracedCallback<uint32_t> m_tracePfc; // 0: resume, 1: pause

  virtual bool SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber);

  virtual Ptr<Node> GetNode (void) const;

  virtual void SetNode (Ptr<Node> node);

  virtual bool NeedsArp (void) const;

  virtual void SetReceiveCallback (NetDevice::ReceiveCallback cb);

  virtual Address GetMulticast (Ipv6Address addr) const;

  virtual void SetPromiscReceiveCallback (PromiscReceiveCallback cb);
  virtual bool SupportsSendFrom (void) const;

  int64_t AssignStreams (int64_t stream);

protected:
 
  virtual void DoDispose (void);

  void TransmitStart (void);
  void TransmitCompleteEvent (void);
  void TransmitReadyEvent (void);
  void TransmitAbort (void);
  virtual void TransmitComplete(void);
  virtual void DequeueAndTransmit(void);

  
  virtual void Resume(unsigned qIndex);
  
  void AddHeader (Ptr<Packet> p, Mac48Address source, Mac48Address dest, uint16_t protocolNumber);
  bool ProcessHeader (Ptr<Packet> p, uint16_t & param);
  
  Ptr<BEgressQueue> m_queue;//
  Ptr<MyCsmaChannel> m_channel;

  //pfc
  bool m_csmaEnabled;	//< PFC behaviour enabled
  bool m_qcnEnabled;
  bool m_dynamicth;
  uint32_t m_pausetime;	//< Time for each Pause
  bool m_paused[qCnt];	//< Whether a queue paused

  //qcn
  EventId  m_nextSend;		//< The next send event
  struct ECNAccount{
	  Ipv4Address source;
	  uint32_t qIndex;
	  uint32_t port;
	  uint8_t ecnbits;
	  uint16_t qfb;
	  uint16_t total;
  };
  std::vector<ECNAccount> *m_ecn_source;

private:

  MyCsmaNetDevice &operator = (const MyCsmaNetDevice &o);
  MyCsmaNetDevice (const MyCsmaNetDevice &o);

  void Init (bool sendEnable, bool receiveEnable);


  void NotifyLinkUp (void);

  uint32_t m_deviceId; 
  bool m_sendEnable;
  bool m_receiveEnable;

  enum TxMachineState
  {
    READY,   /**< The transmitter is ready to begin transmission of a packet */
    BUSY,    /**< The transmitter is busy transmitting a packet */
    GAP,      /**< The transmitter is in the interframe gap time */
    BACKOFF      /**< The transmitter is waiting for the channel to be free */
  };

  TxMachineState m_txMachineState;
  EncapsulationMode m_encapMode;
  DataRate m_bps;
  Time m_tInterframeGap;
  Backoff m_backoff;
  Ptr<Packet> m_currentPkt;
  Ptr<ErrorModel> m_receiveErrorModel;

  TracedCallback<Ptr<const Packet> > m_macTxTrace;
  TracedCallback<Ptr<const Packet> > m_macTxDropTrace;
  TracedCallback<Ptr<const Packet> > m_macPromiscRxTrace;
  TracedCallback<Ptr<const Packet> > m_macRxTrace;
  TracedCallback<Ptr<const Packet> > m_macRxDropTrace;
  TracedCallback<Ptr<const Packet> > m_macTxBackoffTrace;
  TracedCallback<Ptr<const Packet> > m_phyTxBeginTrace;
  TracedCallback<Ptr<const Packet> > m_phyTxEndTrace;
  TracedCallback<Ptr<const Packet> > m_phyTxDropTrace;
  TracedCallback<Ptr<const Packet> > m_phyRxBeginTrace;
  TracedCallback<Ptr<const Packet> > m_phyRxEndTrace;
  TracedCallback<Ptr<const Packet> > m_phyRxDropTrace;
  TracedCallback<Ptr<const Packet> > m_snifferTrace;
  TracedCallback<Ptr<const Packet> > m_promiscSnifferTrace;

  Ptr<Node> m_node;
  Mac48Address m_address;
  NetDevice::ReceiveCallback m_rxCallback;
  NetDevice::PromiscReceiveCallback m_promiscRxCallback;
  uint32_t m_ifIndex;
  bool m_linkUp;
  TracedCallback<> m_linkChangeCallbacks;
  static const uint16_t DEFAULT_MTU = 1500;
  uint32_t m_mtu;
  
public://rdma
        Ptr<MyRdmaEgressQueue> m_rdmaEQ;
	void RdmaEnqueueHighPrioQ(Ptr<Packet> p);
	
	typedef Callback<int, Ptr<Packet>, CustomHeader&> RdmaReceiveCb;    
	RdmaReceiveCb m_rdmaReceiveCb;
	typedef Callback<void, Ptr<MyCsmaNetDevice> > RdmaLinkDownCb;
	RdmaLinkDownCb m_rdmaLinkDownCb;
	typedef Callback<void, Ptr<MyRdmaQueuePair>, Ptr<Packet>, Time> RdmaPktSent;
	RdmaPktSent m_rdmaPktSent;
	
        Ptr<MyRdmaEgressQueue> GetRdmaQueue();
	void TakeDown(); // TODO:take down this device
	void UpdateNextAvail(Time t);//TODO
	
	TracedCallback<Ptr<const Packet>, Ptr<MyRdmaQueuePair> > m_traceQpDequeue;
};

} // namespace ns3

#endif /* MYCSMA_NET_DEVICE_H */
