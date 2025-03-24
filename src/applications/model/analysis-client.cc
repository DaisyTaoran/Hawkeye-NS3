
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "packet-loss-counter.h"
#include "analysis-client.h"

namespace ns3 {

//NS_LOG_COMPONENT_DEFINE ("AnalysisClient");
//NS_OBJECT_ENSURE_REGISTERED (AnalysisClient);

TypeId AnalysisClient::GetTypeId() {
    static TypeId tid = TypeId("AnalysisClient")
        .SetParent<Application>()
        .AddConstructor<AnalysisClient>()
    	.AddAttribute ("RemoteAddress",
			"The destination Address of the outbound packets",
			AddressValue (),
			MakeAddressAccessor (&AnalysisClient::m_serverAddress),
			MakeAddressChecker ())
    	.AddAttribute ("RemotePort", "The destination port of the outbound packets",
                   	UintegerValue (200),
                   	MakeUintegerAccessor (&AnalysisClient::m_serverPort),
                   	MakeUintegerChecker<uint16_t> ())
        .AddAttribute ("Interval",
                   	"The time to wait between send packets", 
                   	TimeValue (Seconds (1.0)),
                   	MakeTimeAccessor (&AnalysisClient::m_signalInterval),
                   	MakeTimeChecker ())
          
        ;
    return tid;
}

AnalysisClient::AnalysisClient() : m_socket(0), m_signalInterval(Seconds(1)) {}
AnalysisClient::~AnalysisClient() {}

void AnalysisClient::SetRemote(Ipv4Address ip, uint16_t port) {
    m_serverAddress = Address(ip);
    m_serverPort = port;
}

void AnalysisClient::SetRemote(Address ip, uint16_t port) {
    m_serverAddress = ip;
    m_serverPort = port;
}

void AnalysisClient::SetSignalInterval(Time interval) {
    m_signalInterval = interval;
}

void AnalysisClient::StartApplication(void) {
    // 创建用于发送信号的Socket
    if(m_socket == 0){
    	    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      	    m_socket = Socket::CreateSocket (GetNode (), tid);
	    m_socket->Bind();    
            m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_serverAddress), m_serverPort));
    }

    // 开始发送信号
    ScheduleNextSignal();
}

void AnalysisClient::StopApplication() {
    if (m_socket != 0) {
        m_socket->Close();
        m_socket = 0;
    }
    
    Simulator::Cancel(m_sendEvent);
    
}

void AnalysisClient::SendSignalToAnalysisServer() {
    if (!m_socket) return;

    // 构造信号数据
    std::string signalData = "Switch is sending a signal!";
    Ptr<Packet> packet = Create<Packet>((uint8_t*)signalData.c_str(), signalData.size());

    // printf("Send to Analysis Server\n");
    // 发送信号到分析服务器
    m_socket->SendTo(packet, 0, m_serverAddress);

    // 调度下一次信号发送
    ScheduleNextSignal();
}

void AnalysisClient::ScheduleNextSignal() {
    m_sendEvent = Simulator::Schedule(m_signalInterval, &AnalysisClient::SendSignalToAnalysisServer, this);
}


}
