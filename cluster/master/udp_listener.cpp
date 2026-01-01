#include "udp_listener.h"
#include "node_registry.h"
#include "command_broker.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <vector>


bool UdpListener::open_and_bind(uint16_t udpPort) {
  udpSock_ = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (udpSock_ < 0) { std::perror("udp socket"); return false; }

  sockaddr_in local{};
  local.sin_family = AF_INET;
  local.sin_port = htons(udpPort);
  local.sin_addr.s_addr = INADDR_ANY;

  if (::bind(udpSock_, (sockaddr*)&local, sizeof(local)) < 0) {
    std::perror("udp bind"); return false;
  }
  return true;
}

void UdpListener::close() {
  if (udpSock_ >= 0) ::close(udpSock_);
  udpSock_ = -1;
}

bool UdpListener::send_to(const sockaddr_in& dst, const std::vector<uint8_t>& bytes) {
  ssize_t s = ::sendto(udpSock_, bytes.data(), bytes.size(), 0, (sockaddr*)&dst, sizeof(dst));
  return s >= 0;
}

void UdpListener::run(NodeRegistry& registry, CommandBroker& broker) {
  std::vector<uint8_t> buf(8192);

  while (g_running) {
    sockaddr_in remote{};
    socklen_t rlen = sizeof(remote);
    ssize_t n = ::recvfrom(udpSock_, buf.data(), buf.size(), 0, (sockaddr*)&remote, &rlen);
    if (n <= 0) continue;

    proto::Packet pkt;
    if (!proto::decode(buf.data(), (size_t)n, pkt)) continue;

    const auto type = (proto::MsgType)pkt.hdr.type;
    if (type == proto::MsgType::Heartbeat) {
      registry.update_heartbeat(pkt.hdr.senderId, pkt.payload, proto::now_ms(), remote);
    } else if (type == proto::MsgType::Response) {
      broker.store_response(pkt.hdr.replyToSeq, pkt.payload);
    }
  }
}
