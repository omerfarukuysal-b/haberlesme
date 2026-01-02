#include "command_dispatcher.h"
#include "protocol.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>
#include <chrono>

void CommandDispatcher::set_network(const mesh::MeshNetwork& network, uint8_t myId, int udpSock) {
  network_ = &network;
  my_id_ = myId;
  udp_sock_ = udpSock;
}

bool CommandDispatcher::send_command_to(uint8_t targetId, const std::string& actionJson) {
  if (!network_ || udp_sock_ < 0) return false;
  
  mesh::NodeInfo target;
  if (!network_->get_node(targetId, target)) {
    return false; // Hedef node bulunamadı
  }
  
  uint32_t cmdSeq = seq_++;
  auto bytes = proto::encode(proto::MsgType::Command, my_id_, cmdSeq, 0, actionJson);
  sockaddr_in dst = target.to_sockaddr();
  
  ssize_t s = ::sendto(udp_sock_, bytes.data(), bytes.size(), 0, (sockaddr*)&dst, sizeof(dst));
  return s >= 0;
}

void CommandDispatcher::store_response(uint32_t replyToSeq, const std::string& responseJson) {
  std::lock_guard<std::mutex> lk(mu_);
  responses_[replyToSeq] = responseJson;
}

std::string CommandDispatcher::wait_response(uint32_t cmdSeq, uint64_t timeoutMs) {
  const uint64_t start = proto::now_ms();
  while (proto::now_ms() - start < timeoutMs) {
    {
      std::lock_guard<std::mutex> lk(mu_);
      auto it = responses_.find(cmdSeq);
      if (it != responses_.end()) {
        std::string out = it->second;
        responses_.erase(it);
        return out;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return "";
}
