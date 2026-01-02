#include "node_config.h"
#include <algorithm>

namespace mesh {

void MeshNetwork::add_node(uint8_t id, const std::string& hostname,
                            const std::string& ip, uint16_t port) {
  // Aynı ID'li node varsa güncelle, yoksa ekle
  auto it = std::find_if(nodes_.begin(), nodes_.end(),
                         [id](const NodeInfo& n) { return n.id == id; });
  if (it != nodes_.end()) {
    *it = NodeInfo(id, hostname, ip, port);
  } else {
    nodes_.emplace_back(id, hostname, ip, port);
  }
}

bool MeshNetwork::get_node(uint8_t id, NodeInfo& out) const {
  auto it = std::find_if(nodes_.begin(), nodes_.end(),
                         [id](const NodeInfo& n) { return n.id == id; });
  if (it != nodes_.end()) {
    out = *it;
    return true;
  }
  return false;
}

std::vector<NodeInfo> MeshNetwork::get_all_nodes() const {
  return nodes_;
}

std::vector<NodeInfo> MeshNetwork::get_other_nodes(uint8_t exclude_id) const {
  std::vector<NodeInfo> result;
  for (const auto& n : nodes_) {
    if (n.id != exclude_id) {
      result.push_back(n);
    }
  }
  return result;
}

} // namespace mesh
