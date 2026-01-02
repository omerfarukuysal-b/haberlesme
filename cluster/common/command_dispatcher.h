#pragma once
#include <cstdint>
#include <string>
#include <mutex>
#include <unordered_map>
#include <netinet/in.h>
#include "../common/node_config.h"

class CommandDispatcher {
public:
  // Belirli bir node'a komut gönder
  bool send_command_to(uint8_t targetId, const std::string& actionJson);
  
  // Yanıt al (blocking, timeout ile)
  std::string wait_response(uint32_t cmdSeq, uint64_t timeoutMs);
  
  // Yanıt sakla
  void store_response(uint32_t replyToSeq, const std::string& responseJson);
  
  // Network bilgilerini ayarla
  void set_network(const mesh::MeshNetwork& network, uint8_t myId, int udpSock);
  
private:
  const mesh::MeshNetwork* network_ = nullptr;
  uint8_t my_id_ = 0;
  int udp_sock_ = -1;
  std::atomic<uint32_t> seq_{1};
  
  std::mutex mu_;
  std::unordered_map<uint32_t, std::string> responses_;
};
