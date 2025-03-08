#ifndef SLOWLORIS_APP_H
#define SLOWLORIS_APP_H

#include "ns3/application.h"
#include "ns3/address.h"
#include "ns3/socket.h"
#include <vector>

namespace ns3 {

class SlowlorisApp : public Application {
public:
    static TypeId GetTypeId();
    SlowlorisApp();
    virtual ~SlowlorisApp();

    void Setup(Address address, uint16_t port, uint32_t numSockets);

private:
    virtual void StartApplication() override;
    virtual void StopApplication() override;

    void SendPartialHeaders();
    void KeepConnectionsAlive();
    


    Address m_address;
    uint16_t m_port;
    uint32_t m_numSockets;
    
    bool m_running;
    uint32_t m_activeConnections = 0;  // ✅ Track active connections

    std::vector<Ptr<Socket>> m_sockets;
};

} // namespace ns3

#endif // SLOWLORIS_APP_H
