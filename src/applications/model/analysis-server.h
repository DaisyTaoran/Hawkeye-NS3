#ifndef ANALYSIS_SERVER_H // 确保头文件内容只被编译一次
#define ANALYSIS_SERVER_H

#include <map>
#include <vector>
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ipv4-address.h"
#include "ns3/socket.h"
#include "ns3/packet.h"
#include "packet-loss-counter.h"
#include "ns3/ptr.h"

namespace ns3{

class Socket;
class Packet;

class AnalysisServer : public Application {
public:
    static TypeId GetTypeId();
    AnalysisServer();
    virtual ~AnalysisServer();
    
    uint32_t GetLost (void) const;
    uint32_t GetReceived (void) const;
    uint16_t GetPacketWindowSize () const;
    void SetPacketWindowSize (uint16_t size);
    void SetLocal (Ipv4Address ip, uint16_t port);
    void SetNextHop(std::map<Ptr<Node>, std::map<Ptr<Node>, std::vector<Ptr<Node>> > > *nexth);
    
protected:
    virtual void DoDispose (void);

private:
    virtual void StartApplication();
    virtual void StopApplication();

    void HandleRead(Ptr<Socket> socket);
    void ScheduleNextRead();
    void ReadFile();

    uint16_t m_port;
    Ptr<Socket> m_socket;
    Address m_local;
    uint32_t m_received;
    PacketLossCounter m_lossCounter;
    Time m_readInterval;
    EventId m_readEvent;                 
    
    std::map<Ptr<Node>, std::map<Ptr<Node>, std::vector<Ptr<Node>> > > *nextHop = NULL;
};


}

#endif
