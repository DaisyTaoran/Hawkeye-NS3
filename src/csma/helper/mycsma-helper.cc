
#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/object-factory.h"
#include "ns3/queue.h"
#include "ns3/mycsma-net-device.h"
#include "ns3/mycsma-channel.h"
#include "ns3/config.h"
#include "ns3/packet.h"
#include "ns3/names.h"

#include "ns3/trace-helper.h"
#include "mycsma-helper.h"

#include <string>

NS_LOG_COMPONENT_DEFINE ("MyCsmaHelper");

namespace ns3 {

MyCsmaHelper::MyCsmaHelper ()
{
  m_queueFactory.SetTypeId ("ns3::DropTailQueue");
  m_deviceFactory.SetTypeId ("ns3::MyCsmaNetDevice");
  m_channelFactory.SetTypeId ("ns3::MyCsmaChannel");
}

void 
MyCsmaHelper::SetQueue (std::string type,
                      std::string n1, const AttributeValue &v1,
                      std::string n2, const AttributeValue &v2,
                      std::string n3, const AttributeValue &v3,
                      std::string n4, const AttributeValue &v4)
{
  m_queueFactory.SetTypeId (type);
  m_queueFactory.Set (n1, v1);
  m_queueFactory.Set (n2, v2);
  m_queueFactory.Set (n3, v3);
  m_queueFactory.Set (n4, v4);
}

void 
MyCsmaHelper::SetDeviceAttribute (std::string n1, const AttributeValue &v1)
{
  m_deviceFactory.Set (n1, v1);
}

void 
MyCsmaHelper::SetChannelAttribute (std::string n1, const AttributeValue &v1)
{
  	m_channelFactory.Set (n1, v1);
}

void 
MyCsmaHelper::EnablePcapInternal (std::string prefix, Ptr<NetDevice> nd, bool promiscuous, bool explicitFilename)
{
  	Ptr<MyCsmaNetDevice> device = nd->GetObject<MyCsmaNetDevice> ();
  	if (device == 0)
    	{
      		NS_LOG_INFO ("MyCsmaHelper::EnablePcapInternal(): Device " << device << " not of type ns3::CsmaNetDevice");
      		return;
    	}

  	PcapHelper pcapHelper;

  	std::string filename;
  	if (explicitFilename)
     		filename = prefix;
  	else
      		filename = pcapHelper.GetFilenameFromDevice (prefix, device);
    	
 	 Ptr<PcapFileWrapper> file = pcapHelper.CreateFile (filename, std::ios::out, PcapHelper::DLT_EN10MB);
  	if (promiscuous)
      		pcapHelper.HookDefaultSink<MyCsmaNetDevice> (device, "PromiscSniffer", file);
  	else
      		pcapHelper.HookDefaultSink<MyCsmaNetDevice> (device, "Sniffer", file);
    	
}

void 
MyCsmaHelper::EnableAsciiInternal (
  Ptr<OutputStreamWrapper> stream, 
  std::string prefix, 
  Ptr<NetDevice> nd,
  bool explicitFilename)
{
  	Ptr<MyCsmaNetDevice> device = nd->GetObject<MyCsmaNetDevice> ();
  	if (device == 0)
    	{
      		NS_LOG_INFO ("MyCsmaHelper::EnableAsciiInternal(): Device " << device << " not of type ns3::MyCsmaNetDevice");
      		return;
    	}

  	Packet::EnablePrinting ();

  	if (stream == 0)
    	{
      		AsciiTraceHelper asciiTraceHelper;

      		std::string filename;
      		if (explicitFilename)
        	{
          		filename = prefix;
        	}
      		else
        	{
          		filename = asciiTraceHelper.GetFilenameFromDevice (prefix, device);
        	}

      		Ptr<OutputStreamWrapper> theStream = asciiTraceHelper.CreateFileStream (filename);

      		asciiTraceHelper.HookDefaultReceiveSinkWithoutContext<MyCsmaNetDevice> (device, "MacRx", theStream);

      		Ptr<Queue> queue = device->GetQueue ();
      		asciiTraceHelper.HookDefaultEnqueueSinkWithoutContext<Queue> (queue, "Enqueue", theStream);
      		asciiTraceHelper.HookDefaultDropSinkWithoutContext<Queue> (queue, "Drop", theStream);
      		asciiTraceHelper.HookDefaultDequeueSinkWithoutContext<Queue> (queue, "Dequeue", theStream);

      		return;
    	}

  	uint32_t nodeid = nd->GetNode ()->GetId ();
  	uint32_t deviceid = nd->GetIfIndex ();
  	std::ostringstream oss;

  	oss << "/NodeList/" << nd->GetNode ()->GetId () << "/DeviceList/" << deviceid << "/$ns3::MyCsmaNetDevice/MacRx";
  	Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultReceiveSinkWithContext, stream));

  	oss.str ("");
  	oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::MyCsmaNetDevice/TxQueue/Enqueue";
  	Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultEnqueueSinkWithContext, stream));

  	oss.str ("");
  	oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::MyCsmaNetDevice/TxQueue/Dequeue";
  	Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDequeueSinkWithContext, stream));

  	oss.str ("");
  	oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::MyCsmaNetDevice/TxQueue/Drop";
  	Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDropSinkWithContext, stream));
}

NetDeviceContainer
MyCsmaHelper::Install (Ptr<Node> node) const
{
  Ptr<MyCsmaChannel> channel = m_channelFactory.Create ()->GetObject<MyCsmaChannel> ();
  return Install (node, channel);
}

NetDeviceContainer
MyCsmaHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return Install (node);
}

NetDeviceContainer
MyCsmaHelper::Install (Ptr<Node> a, Ptr<Node> b) const
{
  Ptr<MyCsmaChannel> channel = m_channelFactory.Create ()->GetObject<MyCsmaChannel> ();
  NodeContainer c;
  c.Add(a);
  c.Add(b);
  return Install (c, channel);
}

NetDeviceContainer
MyCsmaHelper::Install (Ptr<Node> node, Ptr<MyCsmaChannel> channel) const
{
  return NetDeviceContainer (InstallPriv (node, channel));
}

NetDeviceContainer
MyCsmaHelper::Install (Ptr<Node> node, std::string channelName) const
{
  Ptr<MyCsmaChannel> channel = Names::Find<MyCsmaChannel> (channelName);
  return NetDeviceContainer (InstallPriv (node, channel));
}

NetDeviceContainer
MyCsmaHelper::Install (std::string nodeName, Ptr<MyCsmaChannel> channel) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return NetDeviceContainer (InstallPriv (node, channel));
}

NetDeviceContainer
MyCsmaHelper::Install (std::string nodeName, std::string channelName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  Ptr<MyCsmaChannel> channel = Names::Find<MyCsmaChannel> (channelName);
  return NetDeviceContainer (InstallPriv (node, channel));
}

NetDeviceContainer 
MyCsmaHelper::Install (const NodeContainer &c) const
{
  Ptr<MyCsmaChannel> channel = m_channelFactory.Create ()->GetObject<MyCsmaChannel> ();

  return Install (c, channel);
}

NetDeviceContainer 
MyCsmaHelper::Install (const NodeContainer &c, Ptr<MyCsmaChannel> channel) const
{
  NetDeviceContainer devs;

  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); i++)
    {
      devs.Add (InstallPriv (*i, channel));
    }

  return devs;
}

NetDeviceContainer 
MyCsmaHelper::Install (const NodeContainer &c, std::string channelName) const
{
  Ptr<MyCsmaChannel> channel = Names::Find<MyCsmaChannel> (channelName);
  return Install (c, channel);
}

int64_t
MyCsmaHelper::AssignStreams (NetDeviceContainer c, int64_t stream)
{
  int64_t currentStream = stream;
  Ptr<NetDevice> netDevice;
  for (NetDeviceContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      netDevice = (*i);
      Ptr<MyCsmaNetDevice> csma = DynamicCast<MyCsmaNetDevice> (netDevice);
      if (csma)
        {
          currentStream += csma->AssignStreams (currentStream);
        }
    }
  return (currentStream - stream);
}

Ptr<NetDevice>
MyCsmaHelper::InstallPriv (Ptr<Node> node, Ptr<MyCsmaChannel> channel) const
{
  Ptr<MyCsmaNetDevice> device = m_deviceFactory.Create<MyCsmaNetDevice> ();
  device->SetAddress (Mac48Address::Allocate ());
  node->AddDevice (device);
  Ptr<BEgressQueue> queue = m_queueFactory.Create<BEgressQueue> ();
  device->SetQueue (queue);
  device->Attach (channel);

  return device;
}

void MyCsmaHelper::EnableTracing(FILE *file, NodeContainer node_container){
  NetDeviceContainer devs;
  for (NodeContainer::Iterator i = node_container.Begin (); i != node_container.End (); ++i)
    {
      Ptr<Node> node = *i;
      for (uint32_t j = 0; j < node->GetNDevices (); ++j)
        {
			if (node->GetDevice(j)->IsCsma())
				EnableTracingDevice(file, DynamicCast<MyCsmaNetDevice>(node->GetDevice(j)));
        }
    }
}

void MyCsmaHelper::EnableTracingDevice(FILE *file, Ptr<MyCsmaNetDevice> nd){
	uint32_t nodeid = nd->GetNode ()->GetId ();
	uint32_t deviceid = nd->GetIfIndex ();

	nd->TraceConnectWithoutContext("MacRx", MakeBoundCallback(&MyCsmaHelper::MacRxDetailCallback, file, nd)); // MacRx：Trace源之一，表示数据包接收事件。
	nd->TraceConnectWithoutContext("MyCsmaEnqueue", MakeBoundCallback (&MyCsmaHelper::EnqueueDetailCallback, file, nd));
	nd->TraceConnectWithoutContext("MyCsmaDequeue", MakeBoundCallback (&MyCsmaHelper::DequeueDetailCallback, file, nd));
	nd->TraceConnectWithoutContext("MyCsmaDrop", MakeBoundCallback (&MyCsmaHelper::DropDetailCallback, file, nd));
	nd->TraceConnectWithoutContext("RdmaQpDequeue", MakeBoundCallback (&MyCsmaHelper::QpDequeueCallback, file, nd));
	
}


void MyCsmaHelper::GetTraceFromPacket(TraceFormat &tr, Ptr<MyCsmaNetDevice> dev, Ptr<const Packet> p, uint32_t qidx, Event event, bool hasL2){
	CustomHeader hdr((hasL2?CustomHeader::L2_Header:0) | CustomHeader::L3_Header | CustomHeader::L4_Header);
	p->PeekHeader(hdr);

	tr.event = event;
	tr.node = dev->GetNode()->GetId();
	tr.nodeType = dev->GetNode()->GetNodeType();
	tr.intf = dev->GetIfIndex();
	tr.qidx = qidx;
	tr.time = Simulator::Now().GetTimeStep();
	tr.sip = hdr.sip;
	tr.dip = hdr.dip;
	tr.l3Prot = hdr.l3Prot;
	tr.ecn = hdr.m_tos & 0x3;
	switch (hdr.l3Prot){
		case 0x6:
			tr.data.sport = hdr.tcp.sport;
			tr.data.dport = hdr.tcp.dport;
			break;
		case 0x11:
			tr.data.sport = hdr.udp.sport;
			tr.data.dport = hdr.udp.dport;
			tr.data.payload = p->GetSize() - hdr.GetSerializedSize();
			// SeqTsHeader
			tr.data.seq = hdr.udp.seq;
			tr.data.ts = hdr.udp.ih.GetTs();
			tr.data.pg = hdr.udp.pg;
			break;
		case 0xFC:
		case 0xFD:
			tr.ack.sport = hdr.ack.sport;
			tr.ack.dport = hdr.ack.dport;
			tr.ack.flags = hdr.ack.flags;
			tr.ack.pg = hdr.ack.pg;
			tr.ack.seq = hdr.ack.seq;
			tr.ack.ts = hdr.ack.ih.GetTs();
			break;
		case 0xFE:
			tr.pfc.time = hdr.pfc.time;
			tr.pfc.qlen = hdr.pfc.qlen;
			tr.pfc.qIndex = hdr.pfc.qIndex;
			break;
		case 0xFF:
			tr.cnp.fid = hdr.cnp.fid;
			tr.cnp.qIndex = hdr.cnp.qIndex;
			tr.cnp.qfb = hdr.cnp.qfb;
			tr.cnp.ecnBits = hdr.cnp.ecnBits;
			tr.cnp.total = hdr.cnp.total;
			break;
		default:
			break;
	}
	tr.size = p->GetSize();//hdr.m_payloadSize;
	tr.qlen = dev->GetQueue()->GetNBytes(qidx);
}


void MyCsmaHelper::PacketEventCallback(FILE *file, Ptr<MyCsmaNetDevice> dev, Ptr<const Packet> p, uint32_t qidx, Event event, bool hasL2){
	TraceFormat tr;
	GetTraceFromPacket(tr, dev, p, qidx, event, hasL2); 
	tr.Serialize(file); 
}

void MyCsmaHelper::MacRxDetailCallback (FILE* file, Ptr<MyCsmaNetDevice> dev, Ptr<const Packet> p){
	PacketEventCallback(file, dev, p, 0, Recv, true);
}

void MyCsmaHelper::EnqueueDetailCallback(FILE* file, Ptr<MyCsmaNetDevice> dev, Ptr<const Packet> p, uint32_t qidx){
	PacketEventCallback(file, dev, p, qidx, Enqu, true);
}

void MyCsmaHelper::DequeueDetailCallback(FILE* file, Ptr<MyCsmaNetDevice> dev, Ptr<const Packet> p, uint32_t qidx){
	PacketEventCallback(file, dev, p, qidx, Dequ, true);
}

void MyCsmaHelper::DropDetailCallback(FILE* file, Ptr<MyCsmaNetDevice> dev, Ptr<const Packet> p, uint32_t qidx){
	PacketEventCallback(file, dev, p, qidx, Drop, true);
}

void MyCsmaHelper::QpDequeueCallback(FILE *file, Ptr<MyCsmaNetDevice> dev, Ptr<const Packet> p, Ptr<MyRdmaQueuePair> qp){
	TraceFormat tr;
	GetTraceFromPacket(tr, dev, p, qp->m_pg, Dequ, true); 
	tr.Serialize(file); 
}

} // namespace ns3
