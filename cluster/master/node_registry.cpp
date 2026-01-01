#include "node_registry.h"
#include <sstream>

// hic yoksa kaydeder
void NodeRegistry::update_heartbeat(uint8_t senderId, const std::string& heartbeatJson,
                                    uint64_t nowMs, const sockaddr_in& from) {
  std::lock_guard<std::mutex> lk(mu_);
  auto& st = nodes_[senderId];
  st.id = senderId;
  st.lastHeartbeatJson = heartbeatJson;
  st.lastSeenMs = nowMs;
  st.lastAddr = from;
}

bool NodeRegistry::get_last_addr(uint8_t targetId, sockaddr_in& out) const {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = nodes_.find(targetId);
  if (it == nodes_.end()) return false;
  if (it->second.lastSeenMs == 0) return false;
  out = it->second.lastAddr;
  return true;
}

std::string NodeRegistry::to_nodes_json(uint64_t nowMs) const {
  std::lock_guard<std::mutex> lk(mu_);
  std::ostringstream oss;
  oss << "{\"ok\":true,\"nodes\":[";
  bool first = true;
  for (const auto& kv : nodes_) {
    const auto& st = kv.second;
    if (!first) oss << ",";
    first = false;
    const uint64_t age = (nowMs >= st.lastSeenMs) ? (nowMs - st.lastSeenMs) : 999999;
    oss << "{"
        << "\"id\":" << (int)st.id << ","
        << "\"lastSeenMsAgo\":" << age << ","
        << "\"heartbeat\":" << (st.lastHeartbeatJson.empty() ? "{}" : st.lastHeartbeatJson)
        << "}";
  }
  oss << "]}";
  return oss.str();
}
