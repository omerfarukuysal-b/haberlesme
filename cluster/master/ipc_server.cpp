#include "ipc_server.h"
#include "node_registry.h"
#include "command_broker.h"
#include "udp_listener.h"
#include "../common/mini_json.h"
#include "../common/protocol.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <iostream>
#include <sstream>


bool IpcServer::listen_local(uint16_t port) {
  serverFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
  if (serverFd_ < 0) { std::perror("ipc socket"); return false; }

  int reuse=1;
  setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

  if (::bind(serverFd_, (sockaddr*)&addr, sizeof(addr)) < 0) { std::perror("ipc bind"); return false; }
  if (::listen(serverFd_, 16) < 0) { std::perror("ipc listen"); return false; }
  return true;
}

void IpcServer::close() {
  if (serverFd_ >= 0) ::close(serverFd_);
  serverFd_ = -1;
}

std::string IpcServer::read_line(int fd) {
  std::string s;
  char ch;
  while (true) {
    ssize_t n = ::recv(fd, &ch, 1, 0);
    if (n <= 0) break;
    if (ch == '\n') break;
    s.push_back(ch);
    if (s.size() > 100000) break;
  }
  return s;
}

void IpcServer::write_line(int fd, const std::string& line) {
  std::string out = line + "\n";
  ::send(fd, out.data(), out.size(), 0);
}

std::string IpcServer::handle_request(const std::string& line,
                                      NodeRegistry& registry,
                                      CommandBroker& broker,
                                      UdpListener& udp) {
  const std::string op = minijson::get(line, "op");

  if (op == "nodes") {
    return registry.to_nodes_json(proto::now_ms());
  }

  if (op == "command") {
    const std::string targetStr = minijson::get(line, "target");
    const std::string action    = minijson::get(line, "action");
    const std::string arg       = minijson::get(line, "arg");

    if (targetStr.empty() || action.empty()) {
      return "{\"ok\":false,\"error\":\"bad request\"}";
    }

    int target = 0;
    try { target = std::stoi(targetStr); } catch (...) { target = 0; }
    if (target <= 0 || target > 255) return "{\"ok\":false,\"error\":\"bad target\"}";

    sockaddr_in dst{};
    if (!registry.get_last_addr((uint8_t)target, dst)) {
      return "{\"ok\":false,\"error\":\"target not seen\"}";
    }

    // Create command payload JSON
    static std::atomic<uint32_t> cmdSeq{100};
    const uint32_t seq = cmdSeq++;

    std::ostringstream payload;
    payload << "{"
            << "\"cmdId\":" << seq << ","
            << "\"action\":\"" << action << "\","
            << "\"arg\":\"" << arg << "\""
            << "}";

    auto bytes = proto::encode(proto::MsgType::Command, /*senderId*/1, seq, 0, payload.str());
    if (!udp.send_to(dst, bytes)) {
      return "{\"ok\":false,\"error\":\"udp send failed\"}";
    }

    // Wait response (1.5s)
    std::string resp = broker.wait_response(seq, 1500);
    if (resp.empty()) return "{\"ok\":false,\"error\":\"timeout\"}";
    return std::string("{\"ok\":true,\"response\":") + resp + "}";
  }
  if (op == "telemetry") {
    const std::string idStr = minijson::get(line, "id");
    if (idStr.empty()) return "{\"ok\":false,\"error\":\"missing id\"}";
    int id = 0; try { id = std::stoi(idStr); } catch(...) { id = 0; }
    if (id <= 0 || id > 255) return "{\"ok\":false,\"error\":\"bad id\"}";

    std::string tel;
    uint64_t age = 0;
    // (burada NodeRegistry'den çek)
    // örnek: registry.get_telemetry_json((uint8_t)id, tel, age, proto::now_ms());

    // Basit versiyon (getter'ı ona göre ayarla):
    // if (!registry.get_telemetry_json((uint8_t)id, tel, age)) ...

    if (!registry.get_telemetry_json((uint8_t)id, tel, age, proto::now_ms())) {
        return "{\"ok\":false,\"error\":\"no telemetry\"}";
    }
    return std::string("{\"ok\":true,\"id\":") + std::to_string(id) +
            ",\"ageMs\":" + std::to_string(age) +
            ",\"telemetry\":" + tel + "}";
    }


  return "{\"ok\":false,\"error\":\"unknown op\"}";
}

void IpcServer::run(NodeRegistry& registry, CommandBroker& broker, UdpListener& udp) {
  while (g_running) {
    sockaddr_in client{};
    socklen_t clen = sizeof(client);
    int c = ::accept(serverFd_, (sockaddr*)&client, &clen);
    if (c < 0) continue;

    std::string line = read_line(c);
    if (line.empty()) {
      write_line(c, "{\"ok\":false,\"error\":\"empty\"}");
      ::close(c);
      continue;
    }

    std::string resp = handle_request(line, registry, broker, udp);
    write_line(c, resp);
    ::close(c);
  }
}
