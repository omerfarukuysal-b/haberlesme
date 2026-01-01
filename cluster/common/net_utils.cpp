#include "net_utils.h"
#include <arpa/inet.h>
#include <sstream>

namespace netu {

sockaddr_in make_ipv4_addr(const std::string& ip, uint16_t port) {
  sockaddr_in a{};
  a.sin_family = AF_INET;
  a.sin_port = htons(port);
  inet_pton(AF_INET, ip.c_str(), &a.sin_addr);
  return a;
}

std::string addr_to_string(const sockaddr_in& a) {
  char buf[INET_ADDRSTRLEN]{};
  inet_ntop(AF_INET, &a.sin_addr, buf, sizeof(buf));
  std::ostringstream oss;
  oss << buf << ":" << ntohs(a.sin_port);
  return oss.str();
}

} // namespace netu
