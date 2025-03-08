#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"
#include "SlowlorisApp.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SlowlorisSimulation");

Ptr<PacketSink> serverSink;
AnimationInterface* anim; // Pointer to NetAnim

// Function to trace packet reception and simulate server overload
void PacketReceivedTracer(Ptr<const Packet> packet, const Address &from) {
    NS_LOG_INFO("Packet received at: " << Simulator::Now().GetSeconds() << "s, Size: " << packet->GetSize());

    static uint32_t packetCount = 0;
    packetCount++;

    if (packetCount > 100) {  // Simulate server crash after 100 packets
        NS_LOG_ERROR("\U0001F6A8 Server Overloaded! Stopping Server Application...");

        // Change the server node color to RED in NetAnim
        anim->UpdateNodeColor(1, 255, 0, 0);

        // Stop the server application
        Simulator::ScheduleNow([]() {
            Simulator::Stop();
        });
    }
}

int main(int argc, char *argv[]) {
    #define NS3_LOG_DISABLE 1
    #define NS3_DEPRECATED_DISABLE
    
    LogComponentEnable("SlowlorisSimulation", LOG_LEVEL_INFO);
    LogComponentEnable("SlowlorisApp", LOG_LEVEL_INFO);

    NS_LOG_INFO("Starting Simulation...");

    NodeContainer nodes;
    nodes.Create(2);

    // Assign a mobility model to each node to remove NetAnim warnings
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    InternetStackHelper internet;
    internet.Install(nodes);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices = p2p.Install(nodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    NS_LOG_INFO("Setting up Server on Node 1...");

    // Setup server application
    uint16_t port = 8080;
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer serverApp = sinkHelper.Install(nodes.Get(1));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(20.0));

    // Store server sink for later use
    serverSink = serverApp.Get(0)->GetObject<PacketSink>();

    // Attach packet reception tracer
    serverApp.Get(0)->TraceConnectWithoutContext("Rx", MakeCallback(&PacketReceivedTracer));

    NS_LOG_INFO("Setting up Slowloris Attacker on Node 0...");

    // Setup Slowloris attacker
    Ptr<SlowlorisApp> slowlorisApp = CreateObject<SlowlorisApp>();
    slowlorisApp->Setup(interfaces.GetAddress(1), port, 200);
    nodes.Get(0)->AddApplication(slowlorisApp);
    slowlorisApp->SetStartTime(Seconds(2.0));
    slowlorisApp->SetStopTime(Seconds(18.0));

    // 🔹 NetAnim Visualization Setup
    AnimationInterface animation("slowloris-attack.xml");
    anim = &animation;
    animation.SetMaxPktsPerTraceFile(500000);

    // Set Initial Colors
    animation.UpdateNodeColor(0, 255, 0, 255);  // Attacker (Purple)
    animation.UpdateNodeColor(1, 0, 255, 0);    // Server (Green - Normal)

    // Set Node Positions for Better Visualization
    animation.SetConstantPosition(nodes.Get(0), 10.0, 30.0);  // Attacker (Left)
    animation.SetConstantPosition(nodes.Get(1), 60.0, 30.0);  // Server (Right)

    // Enable Packet Metadata & Protocol Counters
    animation.EnablePacketMetadata();
    animation.EnableIpv4L3ProtocolCounters(Seconds(0), Seconds(20));

    // 🔹 Change Color Before Crash
    Simulator::Schedule(Seconds(3.0), []() {
        anim->UpdateNodeColor(1, 255, 165, 0);  // Server turns Orange (Under attack)
    });

    // 🔹 Change Color When the Server Crashes
    Simulator::Schedule(Seconds(6.0), []() {
        anim->UpdateNodeColor(1, 255, 0, 0);  // Server turns Red (Crashed)
    });

    Simulator::Run();

    // Log final results
    uint64_t totalRx = serverSink->GetTotalRx();
    std::cout << "Total Bytes Received by Server: " << totalRx << std::endl;

    if (totalRx < 5000) {
        std::cout << "\U0001F6A8 SERVER CRASHED DUE TO SLOWLORIS ATTACK! NO MORE REQUESTS ACCEPTED! \U0001F6A8" << std::endl;
    }

    Simulator::Destroy();

    NS_LOG_INFO("Simulation Complete.");
    return 0;
}
