#pragma once
#include <netinet/in.h>
#include <atomic>
#include <cstdint>
#include <string>
#include "common/runtime.h"

class CommandHandler;

class UdpAgent {
public:
  bool open_and_bind(uint16_t udpPort);
  void close();

  void run(uint8_t myId, const sockaddr_in& masterAddr, CommandHandler& handler);

private:
  int sock_ = -1;
};
