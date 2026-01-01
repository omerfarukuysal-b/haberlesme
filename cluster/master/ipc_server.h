#pragma once
#include <cstdint>
#include <string>
#include "common/runtime.h"


class NodeRegistry;
class CommandBroker;
class UdpListener;

class IpcServer {
public:
  bool listen_local(uint16_t port);   // 127.0.0.1:port
  void close();
  void run(NodeRegistry& registry, CommandBroker& broker, UdpListener& udp);

private:
  int serverFd_ = -1;

  static std::string read_line(int fd);
  static void write_line(int fd, const std::string& line);

  std::string handle_request(const std::string& line,
                             NodeRegistry& registry,
                             CommandBroker& broker,
                             UdpListener& udp);
};
