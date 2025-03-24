
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
#include "analysis-server.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AnalysisServer");
NS_OBJECT_ENSURE_REGISTERED (AnalysisServer);

TypeId AnalysisServer::GetTypeId (void)
{
    	static TypeId tid = TypeId ("ns3::AnalysisServer")
    	.SetParent<Application> ()
    	.AddConstructor<AnalysisServer>()
    	.AddAttribute ("Port",
                   "Port on which we listen for incoming packets.",
                   UintegerValue (100),
                   MakeUintegerAccessor (&AnalysisServer::m_port),
                   MakeUintegerChecker<uint16_t> ())
    	.AddAttribute ("PacketWindowSize",
                   "The size of the window used to compute the packet loss. This value should be a multiple of 8.",
                   UintegerValue (32),
                   MakeUintegerAccessor (&AnalysisServer::GetPacketWindowSize,
                                         &AnalysisServer::SetPacketWindowSize),
                   MakeUintegerChecker<uint16_t> (8,256))
    	;
    	return tid;
}

AnalysisServer::AnalysisServer() : m_lossCounter(0)
{
	NS_LOG_FUNCTION(this);
	m_received = 0;
}

AnalysisServer::~AnalysisServer() 
{
	NS_LOG_FUNCTION(this);
}

uint32_t AnalysisServer::GetLost (void) const 
{
  	return m_lossCounter.GetLost ();
}

uint32_t AnalysisServer::GetReceived (void) const 
{
  	return m_received;
}

uint16_t AnalysisServer::GetPacketWindowSize () const 
{
  	return m_lossCounter.GetBitMapSize ();
}

void AnalysisServer::SetPacketWindowSize (uint16_t size)
{
  	m_lossCounter.SetBitMapSize (size);
}

void AnalysisServer::SetLocal (Ipv4Address ip, uint16_t port)
{
  	m_local = ip;
  	m_port = port;
}

void AnalysisServer::DoDispose (void) 
{
  	NS_LOG_FUNCTION(this);
	Application::DoDispose ();
}

void AnalysisServer::StartApplication() 
{

	NS_LOG_FUNCTION(this);
	
	// 创建Socket并绑定到本地地址
    	if (m_socket == 0) {
    	        TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      	        m_socket = Socket::CreateSocket (GetNode (), tid);
      		InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      		m_socket->Bind(local);
    	}

    	// 设置接收回调
    	m_socket->SetRecvCallback(MakeCallback(&AnalysisServer::HandleRead, this));
}

void AnalysisServer::StopApplication() 
{
    	
    	NS_LOG_FUNCTION(this);
    	
    	if (m_socket) {
        	m_socket->Close();
    	}
}

void AnalysisServer::HandleRead(Ptr<Socket> socket) 
{
    	Ptr<Packet> packet;
    	Address from;
    	uint32_t src;
    	printf("Analysis Server receive a packet\n");
    	while ((packet = socket->RecvFrom(from))) {
        	
        	// 处理接收到的数据包
        	uint8_t buffer[1024];
        	packet->CopyData(buffer, packet->GetSize());
        	std::string data((char*)buffer, packet->GetSize());
        	std::cout << "Analysis Server received signal: " << data << std::endl;

    
       		
       		// TODO:这里可以添加数据分析逻辑
       		// 读取src的遥测数据，写入root.txt
       		char telemetry_path[100];  
		sprintf(telemetry_path, "mix/telemetry_%d.txt", src);
		FILE *telemetry = fopen(telemetry_path, "r");
		FILE *rootf = fopen("mix/root.txt", "w"); 
		
		fprintf(rootf, "recv packet\n");
		
		fclose(rootf);
        
    	}
    
}


}
