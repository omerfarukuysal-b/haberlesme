#pragma once
#include <netinet/in.h>
#include <atomic>
#include <cstdint>
#include "common/runtime.h"
#include "common/protocol.h"

class NodeRegistry;
class CommandBroker;

class UdpListener {
public:
  bool open_and_bind(uint16_t udpPort);
  int  fd() const { return udpSock_; }
  void close();

  // blocking loop
  void run(NodeRegistry& registry, CommandBroker& broker);

  // send helper
  bool send_to(const sockaddr_in& dst, const std::vector<uint8_t>& bytes);

private:
  int udpSock_ = -1;
};
