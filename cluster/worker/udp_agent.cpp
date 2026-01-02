#include "udp_agent.h"
#include "command_handler.h"
#include "heartbeat.h"
#include "telemetry_collector.h"
#include "../common/protocol.h"

#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

bool UdpAgent::open_and_bind(uint16_t udpPort) {
  sock_ = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (sock_ < 0) { std::perror("socket"); return false; }

  sockaddr_in local{};
  local.sin_family = AF_INET;
  local.sin_port = htons(udpPort);
  local.sin_addr.s_addr = INADDR_ANY;

  if (::bind(sock_, (sockaddr*)&local, sizeof(local)) < 0) {
    std::perror("bind");
    return false;
  }
  return true;
}

void UdpAgent::close() {
  if (sock_ >= 0) ::close(sock_);
  sock_ = -1;
}

bool UdpAgent::send_command(const sockaddr_in& dst, uint8_t senderId, uint32_t seq,
                            const std::string& payload) {
  auto bytes = proto::encode(proto::MsgType::Command, senderId, seq, 0, payload);
  ssize_t s = ::sendto(sock_, bytes.data(), bytes.size(), 0, (sockaddr*)&dst, sizeof(dst));
  return s >= 0;
}

void UdpAgent::run_mesh(uint8_t myId, const mesh::MeshNetwork& network, CommandHandler& handler,
                        TelemetryCollector& collector, // <--- Parametre
                        const std::string& webServerIp, uint16_t webServerPort) {
  std::atomic<uint32_t> seq{1};
  
  // Heartbeat thread - diğer node'lara ve web app'e heartbeat gönder
  std::thread hb([&]{
    using namespace std::chrono_literals;
    auto other_nodes = network.get_other_nodes(myId);
    
    while (g_running) {
      std::string p = heartbeat::make_payload(collector);
      auto bytes = proto::encode(proto::MsgType::Heartbeat, myId, seq++, 0, p);
      
      // Tüm diğer node'lara heartbeat gönder
      for (const auto& node : other_nodes) {
        sockaddr_in addr = node.to_sockaddr();
        ::sendto(sock_, bytes.data(), bytes.size(), 0, (sockaddr*)&addr, sizeof(addr));
        std::cout << "[Node " << (int)myId << "] Heartbeat sent to Node " << (int)node.id 
                  << " (" << node.ip << ":" << node.port << ") - Size: " << bytes.size() << " bytes\n";
      }
      
      // Web app'e de heartbeat gönder (eğer IP verilmişse)
      if (!webServerIp.empty() && webServerPort > 0) {
        sockaddr_in web_addr{};
        web_addr.sin_family = AF_INET;
        web_addr.sin_port = htons(webServerPort);
        inet_pton(AF_INET, webServerIp.c_str(), &web_addr.sin_addr);
        ::sendto(sock_, bytes.data(), bytes.size(), 0, (sockaddr*)&web_addr, sizeof(web_addr));
        std::cout << "[Node " << (int)myId << "] Heartbeat sent to Web App (" 
                  << webServerIp << ":" << webServerPort << ") - Size: " << bytes.size() << " bytes\n";
      }
      
      std::this_thread::sleep_for(1000ms);
    }
  });

  // UDP server: gelen komutları ve heartbeat'leri dinle
  std::vector<uint8_t> buf(8192);
  while (g_running) {
    sockaddr_in remote{};
    socklen_t rlen = sizeof(remote);
    ssize_t n = ::recvfrom(sock_, buf.data(), buf.size(), 0, (sockaddr*)&remote, &rlen);
    if (n <= 0) continue;

    proto::Packet pkt;
    if (!proto::decode(buf.data(), (size_t)n, pkt)) continue;

    const auto type = (proto::MsgType)pkt.hdr.type;
    
    // Paket türüne göre log yaz
    char remote_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &remote.sin_addr, remote_ip, INET_ADDRSTRLEN);
    uint16_t remote_port = ntohs(remote.sin_port);
    
    if (type == proto::MsgType::Heartbeat) {
      std::cout << "[Node " << (int)myId << "] Heartbeat from Node " << (int)pkt.hdr.senderId 
                << " (" << remote_ip << ":" << remote_port << ") - Size: " << n << " bytes\n";

      // --- BAŞLANGIÇ: BURAYI EKLEYİN ---
      // Gelen heartbeat'i Web Sunucusuna da yönlendir (Relay)
      if (!webServerIp.empty() && webServerPort > 0) {
        sockaddr_in web_addr{};
        web_addr.sin_family = AF_INET;
        web_addr.sin_port = htons(webServerPort);
        inet_pton(AF_INET, webServerIp.c_str(), &web_addr.sin_addr);
        
        // Gelen ham paketi (buf) olduğu gibi web sunucusuna ilet
        ::sendto(sock_, buf.data(), n, 0, (sockaddr*)&web_addr, sizeof(web_addr));
      }
      // --- BİTİŞ ---

    } else if (type == proto::MsgType::Command) {
      std::cout << "[Node " << (int)myId << "] Command from Node " << (int)pkt.hdr.senderId 
                << " (" << remote_ip << ":" << remote_port << "): " << pkt.payload << "\n";
      // Komut aldık, işle ve yanıt gönder
      std::string respPayload = handler.handle(pkt.payload);
      auto respBytes = proto::encode(proto::MsgType::Response, myId, seq++, pkt.hdr.seq, respPayload);
      ::sendto(sock_, respBytes.data(), respBytes.size(), 0, (sockaddr*)&remote, sizeof(remote));
      std::cout << "[Node " << (int)myId << "] Response sent to Node " << (int)pkt.hdr.senderId << "\n";
    } else if (type == proto::MsgType::Response) {
      std::cout << "[Node " << (int)myId << "] Response from Node " << (int)pkt.hdr.senderId 
                << " (" << remote_ip << ":" << remote_port << "): " << pkt.payload << "\n";
    }
  }

  hb.join();
}

void UdpAgent::run(uint8_t myId, const sockaddr_in& masterAddr, CommandHandler& handler) {
  std::atomic<uint32_t> seq{1};

  TelemetryCollector collector;

  // Heartbeat thread
  std::thread hb([&]{
    using namespace std::chrono_literals;
    while (g_running) {
      std::string p = heartbeat::make_payload(collector);
      auto bytes = proto::encode(proto::MsgType::Heartbeat, myId, seq++, 0, p);
      ::sendto(sock_, bytes.data(), bytes.size(), 0, (sockaddr*)&masterAddr, sizeof(masterAddr));
      std::this_thread::sleep_for(1000ms);
    }
  });

  std::vector<uint8_t> buf(8192);
  while (g_running) {
    sockaddr_in remote{};
    socklen_t rlen = sizeof(remote);
    ssize_t n = ::recvfrom(sock_, buf.data(), buf.size(), 0, (sockaddr*)&remote, &rlen);
    if (n <= 0) continue;

    proto::Packet pkt;
    if (!proto::decode(buf.data(), (size_t)n, pkt)) continue;

    const auto type = (proto::MsgType)pkt.hdr.type;
    if (type == proto::MsgType::Command) {
      std::string respPayload = handler.handle(pkt.payload);
      auto respBytes = proto::encode(proto::MsgType::Response, myId, seq++, pkt.hdr.seq, respPayload);
      ::sendto(sock_, respBytes.data(), respBytes.size(), 0, (sockaddr*)&masterAddr, sizeof(masterAddr));
    }
  }

  hb.join();
}
