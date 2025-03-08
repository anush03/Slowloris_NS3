#include "SlowlorisApp.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-address.h"
#include "ns3/packet.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("SlowlorisApp");

TypeId SlowlorisApp::GetTypeId() {
    static TypeId tid = TypeId("ns3::SlowlorisApp")
        .SetParent<Application>()
        .SetGroupName("Applications")
        .AddConstructor<SlowlorisApp>();
    return tid;
}

SlowlorisApp::SlowlorisApp() : m_running(false), m_numSockets(0) {}

SlowlorisApp::~SlowlorisApp() {}

void SlowlorisApp::Setup(Address address, uint16_t port, uint32_t numSockets) {
    m_address = address;
    m_port = port;
    m_numSockets = numSockets;
}

void SlowlorisApp::StartApplication() {
    m_running = true;
    for (uint32_t i = 0; i < m_numSockets; i++) {
        Ptr<Socket> socket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
        socket->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(m_address), m_port));
        m_sockets.push_back(socket);
    }
    Simulator::Schedule(Seconds(2.0), &SlowlorisApp::SendPartialHeaders, this);
}

void SlowlorisApp::StopApplication() {
    m_running = false;
    for (auto& socket : m_sockets) {
        socket->Close();
    }
    m_sockets.clear();
}

void SlowlorisApp::SendPartialHeaders() {
    if (!m_running) return;

    std::string partialHeader = "GET / HTTP/1.1\r\nHost: target.com\r\nUser-Agent: Slowloris\r\n";
    Ptr<Packet> packet = Create<Packet>((uint8_t*)partialHeader.c_str(), partialHeader.size());

    for (auto& socket : m_sockets) {
        socket->Send(packet);
    }

    Simulator::Schedule(Seconds(10.0), &SlowlorisApp::KeepConnectionsAlive, this);
}

void SlowlorisApp::KeepConnectionsAlive() {
    if (!m_running) return;

    std::string keepAliveData = "X-a: keep-alive\r\n";
    Ptr<Packet> packet = Create<Packet>((uint8_t*)keepAliveData.c_str(), keepAliveData.size());

    for (auto& socket : m_sockets) {
        socket->Send(packet);
    }

    Simulator::Schedule(Seconds(10.0), &SlowlorisApp::KeepConnectionsAlive, this);
}

} // namespace ns3
