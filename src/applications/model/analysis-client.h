#ifndef ANALYSIS_CLIENT_H // 确保头文件内容只被编译一次
#define ANALYSIS_CLIENT_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/socket.h"
#include "ns3/packet.h"
#include "packet-loss-counter.h"

namespace ns3{

class Socket;
class Packet;


class AnalysisClient: public Application {
public:
    static TypeId GetTypeId();
    AnalysisClient();
    virtual ~AnalysisClient();

    void SetRemote(Ipv4Address ip, uint16_t port); // 设置分析服务器的地址
    void SetRemote(Address ip, uint16_t port); // 设置分析服务器的地址
    void SetSignalInterval(Time interval);         // 设置发送信号的间隔

private:
    virtual void StartApplication(void);
    virtual void StopApplication();

    void SendSignalToAnalysisServer();   // 向分析服务器发送信号
    void ScheduleNextSignal();           // 调度下一次信号发送

    uint16_t m_serverPort;
    Address m_serverAddress;     // 分析服务器的地址
    Ptr<Socket> m_socket;          // 用于发送信号的Socket
    Time m_signalInterval;               // 发送信号的间隔
    EventId m_sendEvent;                 // 事件ID，用于调度
};

}

#endif
