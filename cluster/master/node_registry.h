#pragma once
#include <netinet/in.h>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include "../common/mini_json.h"
struct NodeState {
  uint8_t id = 0;
  std::string lastHeartbeatJson;   // komple heartbeat JSON
  std::string lastTelemetryJson;   // sadece telemetry JSON (kolay endpoint için)
  uint64_t lastSeenMs = 0;
  sockaddr_in lastAddr{};
};

class NodeRegistry {
public:
  void update_heartbeat(uint8_t senderId, const std::string& heartbeatJson,
                        uint64_t nowMs, const sockaddr_in& from);

  bool get_last_addr(uint8_t targetId, sockaddr_in& out) const;

  bool get_telemetry_json(uint8_t id, std::string& outTelemetry, uint64_t& outAgeMs, uint64_t nowMs) const;

  std::string to_nodes_json(uint64_t nowMs) const;

private:
  mutable std::mutex mu_;
  std::unordered_map<uint8_t, NodeState> nodes_;
};
