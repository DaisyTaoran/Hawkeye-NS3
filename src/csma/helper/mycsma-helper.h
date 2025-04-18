
#ifndef MyCsma_HELPER_H
#define MyCsma_HELPER_H

#include <string>

#include "ns3/object-factory.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/deprecated.h"
#include "ns3/trace-helper.h"
#include "ns3/trace-format.h"
#include "ns3/mycsma-net-device.h"

#include "ns3/mycsma-channel.h"
//#include "ns3/attribute.h"

namespace ns3 {

class Packet;

class MyCsmaHelper : public PcapHelperForDevice, public AsciiTraceHelperForDevice
{
public:
  MyCsmaHelper ();
  virtual ~MyCsmaHelper () {}

  void SetQueue (std::string type,
                 std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
                 std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
                 std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
                 std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue ());

 
  void SetDeviceAttribute (std::string n1, const AttributeValue &v1);

  void SetChannelAttribute (std::string n1, const AttributeValue &v1);

  NetDeviceContainer Install (Ptr<Node> node) const;

  NetDeviceContainer Install (std::string name) const;

  NetDeviceContainer Install (Ptr<Node> a, Ptr<Node> b) const;
  
  NetDeviceContainer Install (Ptr<Node> node, Ptr<MyCsmaChannel> channel) const;

  NetDeviceContainer Install (Ptr<Node> node, std::string channelName) const;

  NetDeviceContainer Install (std::string nodeName, Ptr<MyCsmaChannel> channel) const;

  NetDeviceContainer Install (std::string nodeName, std::string channelName) const;

  NetDeviceContainer Install (const NodeContainer &c) const;

  NetDeviceContainer Install (const NodeContainer &c, Ptr<MyCsmaChannel> channel) const;

  NetDeviceContainer Install (const NodeContainer &c, std::string channelName) const;

  int64_t AssignStreams (NetDeviceContainer c, int64_t stream);
  
  
  void EnableTracingDevice(FILE *file, Ptr<MyCsmaNetDevice>);   

  void EnableTracing(FILE *file, NodeContainer node_container);
  
  static void GetTraceFromPacket(TraceFormat &tr, Ptr<MyCsmaNetDevice> dev, Ptr<const Packet> p, uint32_t qidx, Event event, bool hasL2);
  
  static void PacketEventCallback(FILE *file, Ptr<MyCsmaNetDevice>, Ptr<const Packet>, uint32_t qidx, Event event, bool hasL2); 
  static void MacRxDetailCallback (FILE* file, Ptr<MyCsmaNetDevice>, Ptr<const Packet> p);                 
  static void EnqueueDetailCallback(FILE* file, Ptr<MyCsmaNetDevice>, Ptr<const Packet> p, uint32_t qidx); 
  static void DequeueDetailCallback(FILE* file, Ptr<MyCsmaNetDevice>, Ptr<const Packet> p, uint32_t qidx); 
  static void DropDetailCallback(FILE* file, Ptr<MyCsmaNetDevice>, Ptr<const Packet> p, uint32_t qidx);    
  static void QpDequeueCallback(FILE *file, Ptr<MyCsmaNetDevice>, Ptr<const Packet>, Ptr<MyRdmaQueuePair>); 

private:
  Ptr<NetDevice> InstallPriv (Ptr<Node> node, Ptr<MyCsmaChannel> channel) const;

  virtual void EnablePcapInternal (std::string prefix, Ptr<NetDevice> nd, bool promiscuous, bool explicitFilename);

  virtual void EnableAsciiInternal (Ptr<OutputStreamWrapper> stream, 
                                    std::string prefix, 
                                    Ptr<NetDevice> nd,
                                    bool explicitFilename);
                                    

  ObjectFactory m_queueFactory;
  ObjectFactory m_deviceFactory;
  ObjectFactory m_channelFactory;
};

} // namespace ns3

#endif /* MyCsma_HELPER_H */
