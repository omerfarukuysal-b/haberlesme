#pragma once
#include <netinet/in.h>
#include <atomic>
#include <cstdint>
#include <string>
#include <mutex>
#include <vector>
#include "../common/runtime.h"
#include "../common/node_config.h"

class CommandHandler;
class TelemetryCollector; 

class UdpAgent
{
public:
    bool open_and_bind(uint16_t udpPort);
    void close();

    // Mesh mode: sadece heartbeat gönder, diğer node'lar ve web'den komut al
    void run_mesh(uint8_t myId, const mesh::MeshNetwork &network, CommandHandler &handler,
                  TelemetryCollector &collector, // <--- EKLENDİ
                  const std::string &webServerIp = "", uint16_t webServerPort = 0);

    // Master-slave mode (eski): worker -> master iletişimi
    void run(uint8_t myId, const sockaddr_in &masterAddr, CommandHandler &handler,
             TelemetryCollector &collector);

    // Belirli bir node'a komut gönder
    bool send_command(const sockaddr_in &dst, uint8_t senderId, uint32_t seq,
                      const std::string &payload);

private:
    int sock_ = -1;
};
