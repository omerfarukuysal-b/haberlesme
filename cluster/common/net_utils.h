#pragma once
#include <netinet/in.h>
#include <string>

namespace netu {

sockaddr_in make_ipv4_addr(const std::string& ip, uint16_t port);
std::string addr_to_string(const sockaddr_in& a);

} // namespace netu
