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

void UdpAgent::run_mesh(uint8_t myId, const mesh::MeshNetwork& network, CommandHandler& handler) {
  std::atomic<uint32_t> seq{1};
  TelemetryCollector collector;
  
  // Heartbeat thread - tüm node'lara heartbeat gönder (mesh mode)
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
    if (type == proto::MsgType::Command) {
      // Komut aldık, işle ve yanıt gönder
      std::string respPayload = handler.handle(pkt.payload);
      auto respBytes = proto::encode(proto::MsgType::Response, myId, seq++, pkt.hdr.seq, respPayload);
      ::sendto(sock_, respBytes.data(), respBytes.size(), 0, (sockaddr*)&remote, sizeof(remote));
    }
    // Heartbeat mesajları işleme gerek yok, sadece alındığını biliyoruz
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
