
#include "ns3/log.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"
#include "ns3/ethernet-header.h"
#include "ns3/ethernet-trailer.h"
#include "ns3/llc-snap-header.h"
#include "ns3/pause-header.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-header.h"
#include "ns3/random-variable.h"
#include "ns3/error-model.h"
#include "ns3/enum.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/flow-id-tag.h"
#include "ns3/trace-source-accessor.h"
#include "mycsma-net-device.h"
#include "mycsma-channel.h"

NS_LOG_COMPONENT_DEFINE ("MyCsmaNetDevice");

namespace ns3 {

	uint32_t MyRdmaEgressQueue::ack_q_idx = 3;
	TypeId MyRdmaEgressQueue::GetTypeId (void)
	{
		static TypeId tid = TypeId ("ns3::MyRdmaEgressQueue")
			.SetParent<Object> ()
			.AddTraceSource ("RdmaEnqueue", "Enqueue a packet in the MyRdmaEgressQueue.",
					MakeTraceSourceAccessor (&MyRdmaEgressQueue::m_traceRdmaEnqueue))
			.AddTraceSource ("RdmaDequeue", "Dequeue a packet in the MyRdmaEgressQueue.",
					MakeTraceSourceAccessor (&MyRdmaEgressQueue::m_traceRdmaDequeue))
			;
		return tid;
	}

	MyRdmaEgressQueue::MyRdmaEgressQueue(){
		m_rrlast = 0;
		m_qlast = 0;
		m_ackQ = CreateObject<DropTailQueue>();
		m_ackQ->SetAttribute("MaxBytes", UintegerValue(0xffffffff)); // queue limit is on a higher level, not here
	}

	Ptr<Packet> MyRdmaEgressQueue::DequeueQindex(int qIndex){
		if (qIndex == -1){ // high prio
			Ptr<Packet> p = m_ackQ->Dequeue();
			m_qlast = -1;
			m_traceRdmaDequeue(p, 0);
			return p;
		}
		if (qIndex >= 0){ // qp
			Ptr<Packet> p = m_rdmaGetNxtPkt(m_qpGrp->Get(qIndex));
			m_rrlast = qIndex;
			m_qlast = qIndex;
			m_traceRdmaDequeue(p, m_qpGrp->Get(qIndex)->m_pg);
			return p;
		}
		return 0;
	}
	int MyRdmaEgressQueue::GetNextQindex(bool paused[]){
		bool found = false;
		uint32_t qIndex;
		if (!paused[ack_q_idx] && m_ackQ->GetNPackets() > 0)
			return -1;

		// no pkt in highest priority queue, do rr for each qp
		int res = -1024;
		uint32_t fcount = m_qpGrp->GetN();
		uint32_t min_finish_id = 0xffffffff;
		for (qIndex = 1; qIndex <= fcount; qIndex++){
			uint32_t idx = (qIndex + m_rrlast) % fcount;
			Ptr<MyRdmaQueuePair> qp = m_qpGrp->Get(idx);
			if (!paused[qp->m_pg] && qp->GetBytesLeft() > 0 && !qp->IsWinBound()){
				if (m_qpGrp->Get(idx)->m_nextAvail.GetTimeStep() > Simulator::Now().GetTimeStep()) //not available now
					continue;
				res = idx;
				break;
			}else if (qp->IsFinished()){
				min_finish_id = idx < min_finish_id ? idx : min_finish_id;
			}
		}

		// clear the finished qp
		if (min_finish_id < 0xffffffff){
			int nxt = min_finish_id;
			auto &qps = m_qpGrp->m_qps;
			for (int i = min_finish_id + 1; i < fcount; i++) if (!qps[i]->IsFinished()){
				if (i == res) // update res to the idx after removing finished qp
					res = nxt;
				qps[nxt] = qps[i];
				nxt++;
			}
			qps.resize(nxt);
		}
		return res;
	}

	int MyRdmaEgressQueue::GetLastQueue(){
		return m_qlast;
	}

	uint32_t MyRdmaEgressQueue::GetNBytes(uint32_t qIndex){
		NS_ASSERT_MSG(qIndex < m_qpGrp->GetN(), "MyRdmaEgressQueue::GetNBytes: qIndex >= m_qpGrp->GetN()");
		return m_qpGrp->Get(qIndex)->GetBytesLeft();
	}

	uint32_t MyRdmaEgressQueue::GetFlowCount(void){
		return m_qpGrp->GetN();
	}

	Ptr<MyRdmaQueuePair> MyRdmaEgressQueue::GetQp(uint32_t i){
		return m_qpGrp->Get(i);
	}
 
	void MyRdmaEgressQueue::RecoverQueue(uint32_t i){
		NS_ASSERT_MSG(i < m_qpGrp->GetN(), "MyRdmaEgressQueue::RecoverQueue: qIndex >= m_qpGrp->GetN()");
		m_qpGrp->Get(i)->snd_nxt = m_qpGrp->Get(i)->snd_una;
	}

	void MyRdmaEgressQueue::EnqueueHighPrioQ(Ptr<Packet> p){
		m_traceRdmaEnqueue(p, 0);
		m_ackQ->Enqueue(p);
	}

	void MyRdmaEgressQueue::CleanHighPrio(TracedCallback<Ptr<const Packet>, uint32_t> dropCb){
		while (m_ackQ->GetNPackets() > 0){
			Ptr<Packet> p = m_ackQ->Dequeue();
			dropCb(p, 0);
		}
	}

NS_OBJECT_ENSURE_REGISTERED (MyCsmaNetDevice);

TypeId
MyCsmaNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MyCsmaNetDevice")
    .SetParent<NetDevice> ()
    .AddConstructor<MyCsmaNetDevice> ()
    .AddAttribute ("Address", 
                   "The MAC address of this device.",
                   Mac48AddressValue (Mac48Address ("ff:ff:ff:ff:ff:ff")),
                   MakeMac48AddressAccessor (&MyCsmaNetDevice::m_address),
                   MakeMac48AddressChecker ())
    .AddAttribute ("Mtu", "The MAC-level Maximum Transmission Unit",
                   UintegerValue (DEFAULT_MTU),
                   MakeUintegerAccessor (&MyCsmaNetDevice::SetMtu,
                                         &MyCsmaNetDevice::GetMtu),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("EncapsulationMode", 
                   "The link-layer encapsulation type to use.",
                   EnumValue (DIX),
                   MakeEnumAccessor (&MyCsmaNetDevice::SetEncapsulationMode),
                   MakeEnumChecker (DIX, "Dix",
                                    LLC, "Llc"))
    .AddAttribute ("SendEnable", 
                   "Enable or disable the transmitter section of the device.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&MyCsmaNetDevice::m_sendEnable),
                   MakeBooleanChecker ())
    .AddAttribute ("ReceiveEnable",
                   "Enable or disable the receiver section of the device.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&MyCsmaNetDevice::m_receiveEnable),
                   MakeBooleanChecker ())
    .AddAttribute ("ReceiveErrorModel", 
                   "The receiver error model used to simulate packet loss",
                   PointerValue (),
                   MakePointerAccessor (&MyCsmaNetDevice::m_receiveErrorModel),
                   MakePointerChecker<ErrorModel> ())

    //
    // Transmit queueing discipline for the device which includes its own set
    // of trace hooks.
    //
    .AddAttribute ("TxQueue", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&MyCsmaNetDevice::m_queue),
                   MakePointerChecker<Queue> ())

    //
    // Trace sources at the "top" of the net device, where packets transition
    // to/from higher layers.
    //
    .AddTraceSource ("MacTx", 
                     "Trace source indicating a packet has arrived for transmission by this device",
                     MakeTraceSourceAccessor (&MyCsmaNetDevice::m_macTxTrace))
    .AddTraceSource ("MacTxDrop", 
                     "Trace source indicating a packet has been dropped by the device before transmission",
                     MakeTraceSourceAccessor (&MyCsmaNetDevice::m_macTxDropTrace))
    .AddTraceSource ("MacPromiscRx", 
                     "A packet has been received by this device, has been passed up from the physical layer "
                     "and is being forwarded up the local protocol stack.  This is a promiscuous trace,",
                     MakeTraceSourceAccessor (&MyCsmaNetDevice::m_macPromiscRxTrace))
    .AddTraceSource ("MacRx", 
                     "A packet has been received by this device, has been passed up from the physical layer "
                     "and is being forwarded up the local protocol stack.  This is a non-promiscuous trace,",
                     MakeTraceSourceAccessor (&MyCsmaNetDevice::m_macRxTrace))
    .AddTraceSource ("MacTxBackoff", 
                     "Trace source indicating a packet has been delayed by the CSMA backoff process",
                     MakeTraceSourceAccessor (&MyCsmaNetDevice::m_macTxBackoffTrace))
    .AddTraceSource ("PhyTxBegin", 
                     "Trace source indicating a packet has begun transmitting over the channel",
                     MakeTraceSourceAccessor (&MyCsmaNetDevice::m_phyTxBeginTrace))
    .AddTraceSource ("PhyTxEnd", 
                     "Trace source indicating a packet has been completely transmitted over the channel",
                     MakeTraceSourceAccessor (&MyCsmaNetDevice::m_phyTxEndTrace))
    .AddTraceSource ("PhyTxDrop", 
                     "Trace source indicating a packet has been dropped by the device during transmission",
                     MakeTraceSourceAccessor (&MyCsmaNetDevice::m_phyTxDropTrace))
    .AddTraceSource ("PhyRxEnd", 
                     "Trace source indicating a packet has been completely received by the device",
                     MakeTraceSourceAccessor (&MyCsmaNetDevice::m_phyRxEndTrace))
    .AddTraceSource ("PhyRxDrop", 
                     "Trace source indicating a packet has been dropped by the device during reception",
                     MakeTraceSourceAccessor (&MyCsmaNetDevice::m_phyRxDropTrace))
    .AddTraceSource ("Sniffer", 
                     "Trace source simulating a non-promiscuous packet sniffer attached to the device",
                     MakeTraceSourceAccessor (&MyCsmaNetDevice::m_snifferTrace))
    .AddTraceSource ("PromiscSniffer", 
                     "Trace source simulating a promiscuous packet sniffer attached to the device",
                     MakeTraceSourceAccessor (&MyCsmaNetDevice::m_promiscSnifferTrace))
    .AddTraceSource ("MyCsmaPfc", "get a PFC packet. 0: resume, 1: pause",     // 添加一个名为 QbbPfc 的跟踪源
		     MakeTraceSourceAccessor (&MyCsmaNetDevice::m_tracePfc))
                        .AddAttribute("QcnEnabled",
				        "Enable the generation of PAUSE packet.",
				        BooleanValue(false),
				        MakeBooleanAccessor(&MyCsmaNetDevice::m_qcnEnabled),
				        MakeBooleanChecker())
			.AddAttribute("DynamicThreshold",
				        "Enable dynamic threshold.",
				        BooleanValue(false),
				        MakeBooleanAccessor(&MyCsmaNetDevice::m_dynamicth),
				        MakeBooleanChecker())
                        .AddAttribute("PauseTime",
				        "Number of microseconds to pause upon congestion",
				        UintegerValue(5),
				        MakeUintegerAccessor(&MyCsmaNetDevice::m_pausetime),
				        MakeUintegerChecker<uint32_t>())
			.AddAttribute ("MyRdmaEgressQueue", 
					"A queue to use as the transmit queue in the device.",
					PointerValue (),
					MakePointerAccessor (&MyCsmaNetDevice::m_rdmaEQ),
					MakePointerChecker<Object> ())
			.AddTraceSource ("MyCsmaEnqueue", "Enqueue a packet in the MyCsmaNetDevice.",
					MakeTraceSourceAccessor (&MyCsmaNetDevice::m_traceEnqueue))
			.AddTraceSource ("MyCsmaDequeue", "Dequeue a packet in the MyCsmaNetDevice.",
					MakeTraceSourceAccessor (&MyCsmaNetDevice::m_traceDequeue))
			.AddTraceSource ("MyCsmaDrop", "Drop a packet in the MyCsmaNetDevice.",
					MakeTraceSourceAccessor (&MyCsmaNetDevice::m_traceDrop))
			.AddTraceSource ("MyRdmaQpDequeue", "A qp dequeue a packet.",
					MakeTraceSourceAccessor (&MyCsmaNetDevice::m_traceQpDequeue))
			.AddTraceSource ("MyCsmaPfc", "get a PFC packet. 0: resume, 1: pause",   
					MakeTraceSourceAccessor (&MyCsmaNetDevice::m_tracePfc)) 
			.AddAttribute("MyCsmaEnabled",
				"Enable the generation of PAUSE packet.",
				BooleanValue(true),
				MakeBooleanAccessor(&MyCsmaNetDevice::m_csmaEnabled),
				MakeBooleanChecker())
  ;
  return tid;
}

MyCsmaNetDevice::MyCsmaNetDevice ()
  : m_linkUp (false)
{
        NS_LOG_FUNCTION (this);
        m_txMachineState = READY;
        m_tInterframeGap = Seconds (0);
        m_channel = 0; 
        m_encapMode = DIX;
	
	m_ecn_source = new std::vector<ECNAccount>;
        for (uint32_t i = 0; i < qCnt; i++){
		m_paused[i] = false;
	}

	m_rdmaEQ = CreateObject<MyRdmaEgressQueue>();
}

MyCsmaNetDevice::~MyCsmaNetDevice()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_queue = 0;
}

bool
MyCsmaNetDevice::IsCsma (void) const
{
        return true;
}

void
MyCsmaNetDevice::SetDataRate (DataRate bps)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_bps = bps;
}

DataRate MyCsmaNetDevice::GetDataRate(){
	return m_bps;
}

void
MyCsmaNetDevice::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_channel = 0;
  m_node = 0;
  NetDevice::DoDispose ();
}


void
MyCsmaNetDevice::TransmitComplete(void)
{
	NS_LOG_FUNCTION(this);
	NS_ASSERT_MSG(m_txMachineState == BUSY, "Must be BUSY if transmitting");
	m_txMachineState = READY;
	NS_ASSERT_MSG(m_currentPkt != 0, "MyCsmaNetDevice::TransmitComplete(): m_currentPkt zero");
	m_phyTxEndTrace(m_currentPkt);
	m_currentPkt = 0;
	DequeueAndTransmit();
}


void
MyCsmaNetDevice::DequeueAndTransmit(void)
{
		NS_LOG_FUNCTION(this);
		if (!m_linkUp) return; // if link is down, return
		if (m_txMachineState == BUSY) return;	// Quit if channel busy, can't deal with new trans
		Ptr<Packet> p;
		if (m_node->GetNodeType() == 0){
			int qIndex = m_rdmaEQ->GetNextQindex(m_paused);
			if (qIndex != -1024){
				if (qIndex == -1){ // high prio
					p = m_rdmaEQ->DequeueQindex(qIndex); // get p from vector<packet>
					m_traceDequeue(p, 0);
					m_currentPkt = p;//newadd
					TransmitStart(); // 通过Channel发到dstDev,使得调用dst-dev::Receive(p)
					return;
				}
				// a qp dequeue a packet
				Ptr<MyRdmaQueuePair> lastQp = m_rdmaEQ->GetQp(qIndex);
				p = m_rdmaEQ->DequeueQindex(qIndex);

				// transmit
				m_traceQpDequeue(p, lastQp);
				m_currentPkt = p;//newadd
				TransmitStart();

				// update for the next avail time
				m_rdmaPktSent(lastQp, p, m_tInterframeGap);
			}else { // no packet to send
				NS_LOG_INFO("PAUSE prohibits send at node " << m_node->GetId());
				Time t = Simulator::GetMaximumSimulationTime();
				for (uint32_t i = 0; i < m_rdmaEQ->GetFlowCount(); i++){
					Ptr<MyRdmaQueuePair> qp = m_rdmaEQ->GetQp(i);
					if (qp->GetBytesLeft() == 0)
						continue;
					t = Min(qp->m_nextAvail, t);
				}
				if (m_nextSend.IsExpired() && t < Simulator::GetMaximumSimulationTime() && t > Simulator::Now()){
					m_nextSend = Simulator::Schedule(t - Simulator::Now(), &MyCsmaNetDevice::DequeueAndTransmit, this);
				}
			}
			return;
		}else{   //switch, doesn't care about qcn, just send
			p = m_queue->DequeueRR(m_paused);		//this is round-robin
			if (p != 0){
				m_snifferTrace(p);
				m_promiscSnifferTrace(p);
				Ptr<Packet> packet = p->Copy();
				uint16_t protocol = 0;
				ProcessHeader(packet, protocol);
				Ipv4Header h;
				packet->RemoveHeader(h);
				FlowIdTag t;
				uint32_t qIndex = m_queue->GetLastQueue();
				if (qIndex == 0){//this is a pause or cnp, send it immediately!
					m_node->SwitchNotifyDequeue(m_ifIndex, qIndex, p);      // 通知交换机，数据包p已经从端口队列中出队
					p->RemovePacketTag(t);
				}else{
					m_node->SwitchNotifyDequeue(m_ifIndex, qIndex, p);      // 通知交换机，数据包p已经从端口队列中出队
					p->RemovePacketTag(t);
				}
				m_traceDequeue(p, qIndex);
				m_currentPkt = p;//newadd
				TransmitStart();
				return;
			}else{ //No queue can deliver any packet
				NS_LOG_INFO("PAUSE prohibits send at node " << m_node->GetId());
				if (m_node->GetNodeType() == 0 && m_qcnEnabled){ //nothing to send, possibly due to qcn flow control, if so reschedule sending
					Time t = Simulator::GetMaximumSimulationTime();
					for (uint32_t i = 0; i < m_rdmaEQ->GetFlowCount(); i++){
						Ptr<MyRdmaQueuePair> qp = m_rdmaEQ->GetQp(i);
						if (qp->GetBytesLeft() == 0)
							continue;
						t = Min(qp->m_nextAvail, t);
					}
					if (m_nextSend.IsExpired() && t < Simulator::GetMaximumSimulationTime() && t > Simulator::Now()){
						m_nextSend = Simulator::Schedule(t - Simulator::Now(), &MyCsmaNetDevice::DequeueAndTransmit, this);
					}
				}
			}
		}
		return;
}

void
MyCsmaNetDevice::Resume(unsigned qIndex)
{
	NS_LOG_FUNCTION(this << qIndex);
	NS_ASSERT_MSG(m_paused[qIndex], "Must be PAUSEd");
	m_paused[qIndex] = false;
	NS_LOG_INFO("Node " << m_node->GetId() << " dev " << m_ifIndex << " queue " << qIndex <<
		" resumed at " << Simulator::Now().GetSeconds());
	DequeueAndTransmit();
}

void
MyCsmaNetDevice::SetEncapsulationMode (enum EncapsulationMode mode)
{
  NS_LOG_FUNCTION (mode);

  m_encapMode = mode;

  NS_LOG_LOGIC ("m_encapMode = " << m_encapMode);
  NS_LOG_LOGIC ("m_mtu = " << m_mtu);
}

MyCsmaNetDevice::EncapsulationMode
MyCsmaNetDevice::GetEncapsulationMode (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_encapMode;
}

bool
MyCsmaNetDevice::SetMtu (uint16_t mtu)
{
  NS_LOG_FUNCTION (this << mtu);
  m_mtu = mtu;

  NS_LOG_LOGIC ("m_encapMode = " << m_encapMode);
  NS_LOG_LOGIC ("m_mtu = " << m_mtu);

  return true;
}

uint16_t
MyCsmaNetDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_mtu;
}


void
MyCsmaNetDevice::SetSendEnable (bool sendEnable)
{
  NS_LOG_FUNCTION (sendEnable);
  m_sendEnable = sendEnable;
}

void
MyCsmaNetDevice::SetReceiveEnable (bool receiveEnable)
{
  NS_LOG_FUNCTION (receiveEnable);
  m_receiveEnable = receiveEnable;
}

bool
MyCsmaNetDevice::IsSendEnabled (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_sendEnable;
}

bool
MyCsmaNetDevice::IsReceiveEnabled (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_receiveEnable;
}

void
MyCsmaNetDevice::SetInterframeGap (Time t)
{
  NS_LOG_FUNCTION (t);
  m_tInterframeGap = t;
}

void
MyCsmaNetDevice::SetBackoffParams (Time slotTime, uint32_t minSlots, uint32_t maxSlots, uint32_t ceiling, uint32_t maxRetries)
{
  NS_LOG_FUNCTION (slotTime << minSlots << maxSlots << ceiling << maxRetries);
  m_backoff.m_slotTime = slotTime;
  m_backoff.m_minSlots = minSlots;
  m_backoff.m_maxSlots = maxSlots;
  m_backoff.m_ceiling = ceiling;
  m_backoff.m_maxRetries = maxRetries;
}

void
MyCsmaNetDevice::AddHeader (Ptr<Packet> p,   Mac48Address source,  Mac48Address dest,  uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (p << source << dest << protocolNumber);

  EthernetHeader header (false);
  header.SetSource (source);
  header.SetDestination (dest);

  EthernetTrailer trailer;

  uint16_t lengthType = 0;
  switch (m_encapMode) 
    {
    case DIX:
      lengthType = protocolNumber;

      //
      // All Ethernet frames must carry a minimum payload of 46 bytes.  We need
      // to pad out if we don't have enough bytes.  These must be real bytes 
      // since they will be written to pcap files and compared in regression 
      // trace files.
      //
      if (p->GetSize () < 46)
        {
          uint8_t buffer[46];
          memset (buffer, 0, 46);
          Ptr<Packet> padd = Create<Packet> (buffer, 46 - p->GetSize ());
          p->AddAtEnd (padd);
        }
      break;
    case LLC: 
      {
        NS_LOG_LOGIC ("Encapsulating packet as LLC (length interpretation)");

        LlcSnapHeader llc;
        llc.SetType (protocolNumber);
        p->AddHeader (llc);

        //
        // This corresponds to the length interpretation of the lengthType 
        // field but with an LLC/SNAP header added to the payload as in 
        // IEEE 802.2
        //
        lengthType = p->GetSize ();

        //
        // All Ethernet frames must carry a minimum payload of 46 bytes.  The 
        // LLC SNAP header counts as part of this payload.  We need to padd out
        // if we don't have enough bytes.  These must be real bytes since they 
        // will be written to pcap files and compared in regression trace files.
        //
        if (p->GetSize () < 46)
          {
            uint8_t buffer[46];
            memset (buffer, 0, 46);
            Ptr<Packet> padd = Create<Packet> (buffer, 46 - p->GetSize ());
            p->AddAtEnd (padd);
          }

        NS_ASSERT_MSG (p->GetSize () <= GetMtu (),
                       "MyCsmaNetDevice::AddHeader(): 802.3 Length/Type field with LLC/SNAP: "
                       "length interpretation must not exceed device frame size minus overhead");
      }
      break;
    case ILLEGAL:
    default:
      NS_FATAL_ERROR ("MyCsmaNetDevice::AddHeader(): Unknown packet encapsulation mode");
      break;
    }

  NS_LOG_LOGIC ("header.SetLengthType (" << lengthType << ")");
  header.SetLengthType (lengthType);
  p->AddHeader (header);

  if (Node::ChecksumEnabled ())
    {
      trailer.EnableFcs (true);
    }
  trailer.CalcFcs (p);
  p->AddTrailer (trailer);
}

#if 1
bool
MyCsmaNetDevice::ProcessHeader (Ptr<Packet> p, uint16_t & param)
{
  NS_LOG_FUNCTION (p << param);

  EthernetTrailer trailer;
  p->RemoveTrailer (trailer);

  EthernetHeader header (false);
  p->RemoveHeader (header);

  if ((header.GetDestination () != GetBroadcast ()) &&
      (header.GetDestination () != GetAddress ()))
    {
      return false;
    }

  switch (m_encapMode)
    {
    case DIX:
      param = header.GetLengthType ();
      break;
    case LLC: 
      {
        LlcSnapHeader llc;
        p->RemoveHeader (llc);
        param = llc.GetType ();
      } 
      break;
    case ILLEGAL:
    default:
      NS_FATAL_ERROR ("MyCsmaNetDevice::ProcessHeader(): Unknown packet encapsulation mode");
      break;
    }
  return true;
}
#endif


void
MyCsmaNetDevice::TransmitStart (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  NS_ASSERT_MSG (m_currentPkt != 0, "MyCsmaNetDevice::TransmitStart(): m_currentPkt not set");

  NS_LOG_LOGIC ("m_currentPkt = " << m_currentPkt);
  NS_LOG_LOGIC ("UID = " << m_currentPkt->GetUid ());

  if (IsSendEnabled () == false)
    {
      m_phyTxDropTrace (m_currentPkt);
      m_currentPkt = 0;
      return;
    }

  NS_ASSERT_MSG ((m_txMachineState == READY) || (m_txMachineState == BACKOFF), 
                 "Must be READY to transmit. Tx state is: " << m_txMachineState);

  if (m_channel->GetState () != IDLE)
    {
      m_txMachineState = BACKOFF;

      if (m_backoff.MaxRetriesReached ())
        { 
          TransmitAbort ();
        } 
      else 
        {
          m_macTxBackoffTrace (m_currentPkt);

          m_backoff.IncrNumRetries ();
          Time backoffTime = m_backoff.GetBackoffTime ();

          NS_LOG_LOGIC ("Channel busy, backing off for " << backoffTime.GetSeconds () << " sec");

          Simulator::Schedule (backoffTime, &MyCsmaNetDevice::TransmitStart, this);
        }
    } 
  else 
    {
      //
      // The channel is free, transmit the packet
      //
      if (m_channel->TransmitStart (m_currentPkt, m_deviceId) == false)
        {
          NS_LOG_WARN ("Channel TransmitStart returns an error");
          m_phyTxDropTrace (m_currentPkt);
          m_currentPkt = 0;
          m_txMachineState = READY;
        } 
      else 
        {
          //
          // Transmission succeeded, reset the backoff time parameters and
          // schedule a transmit complete event.
          //
          m_backoff.ResetBackoffTime ();
          m_txMachineState = BUSY;
          m_phyTxBeginTrace (m_currentPkt);

          Time tEvent = Seconds (m_bps.CalculateTxTime (m_currentPkt->GetSize ()));
          NS_LOG_LOGIC ("Schedule TransmitCompleteEvent in " << tEvent.GetSeconds () << "sec");
          Simulator::Schedule (tEvent, &MyCsmaNetDevice::TransmitCompleteEvent, this);
        }
    }
}

void
MyCsmaNetDevice::TransmitAbort (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  //
  // When we started the process of transmitting the current packet, it was 
  // placed in m_currentPkt.  So we had better find one there.
  //
  NS_ASSERT_MSG (m_currentPkt != 0, "MyCsmaNetDevice::TransmitAbort(): m_currentPkt zero");
  NS_LOG_LOGIC ("m_currentPkt=" << m_currentPkt);
  NS_LOG_LOGIC ("Pkt UID is " << m_currentPkt->GetUid () << ")");

  m_phyTxDropTrace (m_currentPkt);
  m_currentPkt = 0;

  NS_ASSERT_MSG (m_txMachineState == BACKOFF, "Must be in BACKOFF state to abort.  Tx state is: " << m_txMachineState);

  // 
  // We're done with that one, so reset the backoff algorithm and ready the
  // transmit state machine.
  //
  m_backoff.ResetBackoffTime ();
  m_txMachineState = READY;

  //
  // If there is another packet on the input queue, we need to start trying to 
  // get that out.  If the queue is empty we just wait until someone puts one
  // in.
  //
  if (m_queue->IsEmpty ())
    {
      return;
    }
  else
    {
      m_currentPkt = m_queue->Dequeue ();
      NS_ASSERT_MSG (m_currentPkt != 0, "MyCsmaNetDevice::TransmitAbort(): IsEmpty false but no Packet on queue?");
      m_snifferTrace (m_currentPkt);
      m_promiscSnifferTrace (m_currentPkt);
      TransmitStart ();
    }
}

void
MyCsmaNetDevice::TransmitCompleteEvent (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  //
  // This function is called to finish the  process of transmitting a packet.
  // We need to tell the channel that we've stopped wiggling the wire and
  // schedule an event that will be executed when it's time to re-enable
  // the transmitter after the interframe gap.
  //
  NS_ASSERT_MSG (m_txMachineState == BUSY, "MyCsmaNetDevice::transmitCompleteEvent(): Must be BUSY if transmitting");
  NS_ASSERT (m_channel->GetState () == TRANSMITTING);
  m_txMachineState = GAP;

  //
  // When we started transmitting the current packet, it was placed in 
  // m_currentPkt.  So we had better find one there.
  //
  NS_ASSERT_MSG (m_currentPkt != 0, "MyCsmaNetDevice::TransmitCompleteEvent(): m_currentPkt zero");
  NS_LOG_LOGIC ("m_currentPkt=" << m_currentPkt);
  NS_LOG_LOGIC ("Pkt UID is " << m_currentPkt->GetUid () << ")");

  m_channel->TransmitEnd (); 
  m_phyTxEndTrace (m_currentPkt);
  m_currentPkt = 0;

  NS_LOG_LOGIC ("Schedule TransmitReadyEvent in " << m_tInterframeGap.GetSeconds () << "sec");

  Simulator::Schedule (m_tInterframeGap, &MyCsmaNetDevice::TransmitReadyEvent, this);
}

void
MyCsmaNetDevice::TransmitReadyEvent (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  //
  // This function is called to enable the transmitter after the interframe
  // gap has passed.  If there are pending transmissions, we use this opportunity
  // to start the next transmit.
  //
  NS_ASSERT_MSG (m_txMachineState == GAP, "MyCsmaNetDevice::TransmitReadyEvent(): Must be in interframe gap");
  m_txMachineState = READY;

  //
  // We expect that the packet we had been transmitting was cleared when the 
  // TransmitCompleteEvent() was executed.
  //
  NS_ASSERT_MSG (m_currentPkt == 0, "MyCsmaNetDevice::TransmitReadyEvent(): m_currentPkt nonzero");

  //
  // Get the next packet from the queue for transmitting
  //
  if (m_queue->IsEmpty ())
    {
      return;
    }
  else
    {
      m_currentPkt = m_queue->Dequeue ();
      NS_ASSERT_MSG (m_currentPkt != 0, "MyCsmaNetDevice::TransmitReadyEvent(): IsEmpty false but no Packet on queue?");
      m_snifferTrace (m_currentPkt);
      m_promiscSnifferTrace (m_currentPkt);
      TransmitStart ();
    }
}

bool
MyCsmaNetDevice::Attach (Ptr<MyCsmaChannel> ch)
{
  NS_LOG_FUNCTION (this << &ch);

  m_channel = ch;

  m_deviceId = m_channel->Attach (this);

  //
  // The channel provides us with the transmitter data rate.
  //
  m_bps = m_channel->GetDataRate ();

  //
  // We use the Ethernet interframe gap of 96 bit times.
  //
  m_tInterframeGap = Seconds (m_bps.CalculateTxTime (96/8));

  //
  // This device is up whenever a channel is attached to it.
  //
  NotifyLinkUp ();
  return true;
}

void
MyCsmaNetDevice::SetQueue (Ptr<BEgressQueue> q)
{
  NS_LOG_FUNCTION (q);
  m_queue = q;
}

void
MyCsmaNetDevice::SetReceiveErrorModel (Ptr<ErrorModel> em)
{
  NS_LOG_FUNCTION (em);
  m_receiveErrorModel = em; 
}

void
MyCsmaNetDevice::Receive (Ptr<Packet> packet, Ptr<MyCsmaNetDevice> senderDevice)
{
  NS_LOG_FUNCTION (packet << senderDevice);
  NS_LOG_LOGIC ("UID is " << packet->GetUid ());

  if (senderDevice == this)
    {
      return;
    }

  m_phyRxEndTrace (packet);

  if (IsReceiveEnabled () == false)
    {
      m_phyRxDropTrace (packet);
      return;
    }

  if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt (packet) )
    {
      NS_LOG_LOGIC ("Dropping pkt due to error model ");
      m_phyRxDropTrace (packet);
      return;
    }
    Ptr<Packet> originalPacket = packet->Copy ();
/*
  EthernetTrailer trailer;
  packet->RemoveTrailer (trailer);
  if (Node::ChecksumEnabled ())
    {
      trailer.EnableFcs (true);
    }

  trailer.CheckFcs (packet);
  bool crcGood = trailer.CheckFcs (packet);
  if (!crcGood)
    {
      NS_LOG_INFO ("CRC error on Packet " << packet);
      m_phyRxDropTrace (packet);
      return;
    }

  EthernetHeader header (false);
  packet->RemoveHeader (header);
  
  NS_LOG_LOGIC ("Pkt source is " << header.GetSource ());
  NS_LOG_LOGIC ("Pkt destination is " << header.GetDestination ());
*/
        CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
	ch.getInt = 1; // parse INT header
	packet->PeekHeader(ch); // read but not remove header
		
        if (ch.l3Prot == 0xFE){ // PFC
		if (!m_csmaEnabled) return;
		unsigned qIndex = ch.pfc.qIndex;
		if (ch.pfc.time > 0){
				m_tracePfc(1); // 1：pause
				m_paused[qIndex] = true;
		}else{
				m_tracePfc(0); // 0：resume
				Resume(qIndex);
		}
	}else { // non-PFC packets (data, ACK, NACK, CNP...)
		if (m_node->GetNodeType() > 0){ // switch
				packet->AddPacketTag(FlowIdTag(m_ifIndex));
				m_node->SwitchReceiveFromDevice(this, packet, ch);
		}else { // NIC
				// send to MyRdmaHw
				int ret = m_rdmaReceiveCb(packet, ch); // 在MyRdmaHw::Setup中进行了MakeCallback，绑定了RdmaHw::Receive函数
				// TODO we may based on the ret do something
		}
	}
	return;


}

void
MyCsmaNetDevice::NotifyLinkUp (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_linkUp = true;
  m_linkChangeCallbacks ();
}

void
MyCsmaNetDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (index);
  m_ifIndex = index;
}

uint32_t
MyCsmaNetDevice::GetIfIndex (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_ifIndex;
}

Ptr<Channel>
MyCsmaNetDevice::GetChannel (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_channel;
}

void
MyCsmaNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_address = Mac48Address::ConvertFrom (address);
}

Address
MyCsmaNetDevice::GetAddress (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_address;
}

bool
MyCsmaNetDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_linkUp;
}

void
MyCsmaNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
  NS_LOG_FUNCTION (&callback);
  m_linkChangeCallbacks.ConnectWithoutContext (callback);
}

bool
MyCsmaNetDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

Address
MyCsmaNetDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool
MyCsmaNetDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

Address
MyCsmaNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (multicastGroup);

  Mac48Address ad = Mac48Address::GetMulticast (multicastGroup);

  //
  // Implicit conversion (operator Address ()) is defined for Mac48Address, so
  // use it by just returning the EUI-48 address which is automagically converted
  // to an Address.
  //
  NS_LOG_LOGIC ("multicast address is " << ad);

  return ad;
}

bool
MyCsmaNetDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return false;
}

bool
MyCsmaNetDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return false;
}

bool
MyCsmaNetDevice::Send (Ptr<Packet> packet,const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (packet << dest << protocolNumber);
  return SendFrom (packet, m_address, dest, protocolNumber);
}

bool
MyCsmaNetDevice::SendFrom (Ptr<Packet> packet, const Address& src, const Address& dest, uint16_t protocolNumber)
{
	NS_FATAL_ERROR("SendFrom not implemented");
	return false;
}

/*
bool
MyCsmaNetDevice::SendFrom (Ptr<Packet> packet, const Address& src, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (packet << src << dest << protocolNumber);

  NS_ASSERT (IsLinkUp ());

  if (IsSendEnabled () == false)
    {
      m_macTxDropTrace (packet);
      return false;
    }

  Mac48Address destination = Mac48Address::ConvertFrom (dest);
  Mac48Address source = Mac48Address::ConvertFrom (src);
  AddHeader (packet, source, destination, protocolNumber);

  m_macTxTrace (packet);

  if (m_queue->Enqueue (packet) == false)
    {
      m_macTxDropTrace (packet);
      return false;
    }

  if (m_txMachineState == READY) 
    {
      if (m_queue->IsEmpty () == false)
        {
          m_currentPkt = m_queue->Dequeue ();
          NS_ASSERT_MSG (m_currentPkt != 0, "MyCsmaNetDevice::SendFrom(): IsEmpty false but no Packet on queue?");
          m_promiscSnifferTrace (m_currentPkt);
          m_snifferTrace (m_currentPkt);
          TransmitStart ();
        }
    }
  return true;
}
*/
bool
MyCsmaNetDevice::SwitchSend (uint32_t qIndex, Ptr<Packet> packet, CustomHeader &ch)
{
        m_macTxTrace (packet);
	m_traceEnqueue(packet, qIndex);
        m_queue->Enqueue (packet, qIndex);
	DequeueAndTransmit();
	return true;
}

void MyCsmaNetDevice::SendPfc(uint32_t qIndex, uint32_t type)// 从此网卡的队列qIndex处向外广播，发送PFC Pause/Resume 包
{     
	Ptr<Packet> p = Create<Packet>(0);
	// TODO:将PFC帧头添加到数据包中。
	PauseHeader pauseh((type == 0 ? m_pausetime : 0), m_queue->GetNBytes(qIndex), qIndex);
	p->AddHeader(pauseh); 
	// 添加eth头部。
	Mac48Address destination("01:80:c2:00:00:01");
        Mac48Address source = Mac48Address::ConvertFrom (m_address);
        AddHeader (p, source, destination, 0x800);
	// 从队列0处发送数据包p，并传递自定义头部ch
	CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
	p->PeekHeader(ch);
	SwitchSend(0, p, ch);   
}


void MyCsmaNetDevice::SendSignal(uint32_t qIndex, uint32_t rate, uint32_t epoch, uint32_t congestionPort, bool pfcOff)
{
	Ptr<Packet> p = Create<Packet>(0);
        // 将IPv4头部添加到数据包中。
	Ipv4Header ipv4h;  // Prepare IPv4 header
	ipv4h.SetProtocol(0xFB);
	ipv4h.SetSource(m_node->GetObject<Ipv4>()->GetAddress(m_ifIndex, 0).GetLocal());
	ipv4h.SetDestination(Ipv4Address("255.255.255.255"));
	ipv4h.SetPayloadSize(9);	//TODO : signal size
	ipv4h.SetTtl(1);
	ipv4h.SetIdentification(UniformVariable(0, 65536).GetValue());
	p->AddHeader(ipv4h);
	Mac48Address destination("ff:ff:ff:ff:ff:ff");
        Mac48Address source = Mac48Address::ConvertFrom (m_address);
	AddHeader(p, source, destination, 0x800);
        // 从队列0处发送数据包p，并传递自定义头部ch
	CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header);
	p->PeekHeader(ch);
	ch.headerType |= CustomHeader::L4_Header;
	ch.signal.congestionPort = congestionPort;
	ch.signal.epochID = epoch;
	ch.signal.flowRate = rate;
	ch.signal.pfcOff = pfcOff;
	ch.signal.lastTimeStep = (uint32_t)(Simulator::Now().GetTimeStep() >> 5);
	uint16_t temp;
	ProcessHeader(p, temp);
	p->RemoveHeader(ipv4h);
	p->AddHeader(ch);
	SwitchSend(0, p, ch);
}

void MyCsmaNetDevice::SendAnalysis(uint32_t qIndex, uint32_t rate, uint32_t epoch, Ipv4Address dst_addr)
{
	Ptr<Packet> p = Create<Packet>(0);
        // 将IPv4头部添加到数据包中。
	Ipv4Header ipv4h;  // Prepare IPv4 header
	ipv4h.SetProtocol(0xFB);
	ipv4h.SetSource(m_node->GetObject<Ipv4>()->GetAddress(m_ifIndex, 0).GetLocal());
	ipv4h.SetDestination(dst_addr);
	ipv4h.SetPayloadSize(9);	//TODO : signal size
	ipv4h.SetTtl(1);
	ipv4h.SetIdentification(UniformVariable(0, 65536).GetValue());
	p->AddHeader(ipv4h);
	Mac48Address destination = Mac48Address::ConvertFrom (dst_addr);
        Mac48Address source = Mac48Address::ConvertFrom (m_address);
	AddHeader(p, source, destination, 0x800);
        // 从队列0处发送数据包p，并传递自定义头部ch
	CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header);
	p->PeekHeader(ch);
	ch.headerType |= CustomHeader::L4_Header;
	ch.signal.epochID = epoch;
	ch.signal.flowRate = rate;
	ch.signal.lastTimeStep = (uint32_t)(Simulator::Now().GetTimeStep() >> 5);
	uint16_t temp;
	ProcessHeader(p, temp);
	p->RemoveHeader(ipv4h);
	p->AddHeader(ch);
	SwitchSend(0, p, ch);
}

Ptr<Node>
MyCsmaNetDevice::GetNode (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_node;
}

void
MyCsmaNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (node);

  m_node = node;
}

bool
MyCsmaNetDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

void
MyCsmaNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  NS_LOG_FUNCTION (&cb);
  m_rxCallback = cb;
}

Address MyCsmaNetDevice::GetMulticast (Ipv6Address addr) const
{
  Mac48Address ad = Mac48Address::GetMulticast (addr);

  NS_LOG_LOGIC ("MAC IPv6 multicast address is " << ad);
  return ad;
}

void
MyCsmaNetDevice::SetPromiscReceiveCallback (NetDevice::PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION (&cb);
  m_promiscRxCallback = cb;
}

bool
MyCsmaNetDevice::SupportsSendFrom () const
{
  NS_LOG_FUNCTION_NOARGS ();
  return false; // 注释了SendFrom函数
}

int64_t
MyCsmaNetDevice::AssignStreams (int64_t stream)
{
  return m_backoff.AssignStreams (stream);
}


   void MyCsmaNetDevice::NewQp(Ptr<MyRdmaQueuePair> qp){
	   qp->m_nextAvail = Simulator::Now();
	   DequeueAndTransmit();
   }
   void MyCsmaNetDevice::ReassignedQp(Ptr<MyRdmaQueuePair> qp){
	   DequeueAndTransmit();
   }
   void MyCsmaNetDevice::TriggerTransmit(void){
	   DequeueAndTransmit();
   }

	Ptr<MyRdmaEgressQueue> MyCsmaNetDevice::GetRdmaQueue(){
		return m_rdmaEQ;
	}

	void MyCsmaNetDevice::RdmaEnqueueHighPrioQ(Ptr<Packet> p){
		m_traceEnqueue(p, 0);
		m_rdmaEQ->EnqueueHighPrioQ(p);
	}

	void MyCsmaNetDevice::TakeDown(){
		
		if (m_node->GetNodeType() == 0){
			m_rdmaEQ->CleanHighPrio(m_traceDrop);
			m_rdmaLinkDownCb(this);
		}else { // clean the queue
			for (uint32_t i = 0; i < qCnt; i++)
				m_paused[i] = false;
			while (1){
				Ptr<Packet> p = m_queue->DequeueRR(m_paused);
				if (p == 0)
					 break;
				m_traceDrop(p, m_queue->GetLastQueue());
			}
			// TODO: Notify switch that this link is down
		}
		m_linkUp = false;
	}

	void MyCsmaNetDevice::UpdateNextAvail(Time t){
		if (!m_nextSend.IsExpired() && t < m_nextSend.GetTs()){
			Simulator::Cancel(m_nextSend);
			Time delta = t < Simulator::Now() ? Time(0) : t - Simulator::Now();
			m_nextSend = Simulator::Schedule(delta, &MyCsmaNetDevice::DequeueAndTransmit, this);
		}
	}

	bool MyCsmaNetDevice::GetEgressPaused(uint32_t qIndex){
		return m_paused[qIndex];
	}

} // namespace ns3
