#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace mesh {

struct NodeInfo {
  uint8_t id;
  std::string hostname;
  std::string ip;
  uint16_t port;
  
  NodeInfo() : id(0), port(0) {}
  NodeInfo(uint8_t id_, const std::string& hostname_, 
           const std::string& ip_, uint16_t port_)
    : id(id_), hostname(hostname_), ip(ip_), port(port_) {}
  
  sockaddr_in to_sockaddr() const {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    return addr;
  }
};

class MeshNetwork {
public:
  // Node listesine ek ekle
  void add_node(uint8_t id, const std::string& hostname,
                const std::string& ip, uint16_t port);
  
  // ID'ye göre node bilgisi al
  bool get_node(uint8_t id, NodeInfo& out) const;
  
  // Tüm node'ları al
  std::vector<NodeInfo> get_all_nodes() const;
  
  // Tüm node'ların sayısı
  size_t node_count() const { return nodes_.size(); }
  
  // Node'un kendisi hariç diğer tüm node'ları al
  std::vector<NodeInfo> get_other_nodes(uint8_t exclude_id) const;

private:
  std::vector<NodeInfo> nodes_;
};

} // namespace mesh
