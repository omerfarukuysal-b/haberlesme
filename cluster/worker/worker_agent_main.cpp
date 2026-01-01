#include "udp_agent.h"
#include "command_handler.h"
#include "common/net_utils.h"

#include <atomic>
#include <csignal>
#include <iostream>

std::atomic<bool> g_running{true};
static void on_sig(int){ g_running=false; }

int main(int argc, char** argv) {
  std::signal(SIGINT, on_sig);
  std::signal(SIGTERM, on_sig);

  int id = 0;
  std::string masterIp;
  int port = 50000;

  for (int i=1;i<argc;i++){
    std::string a=argv[i];
    if (a=="--id" && i+1<argc) id = std::stoi(argv[++i]);
    else if (a=="--master" && i+1<argc) masterIp = argv[++i];
    else if (a=="--port" && i+1<argc) port = std::stoi(argv[++i]);
  }

  if (id <= 0 || id > 255 || masterIp.empty()) {
    std::cerr << "Usage: " << argv[0] << " --id <1..255> --master <master_ip> [--port 50000]\n";
    return 1;
  }

  sockaddr_in masterAddr = netu::make_ipv4_addr(masterIp, (uint16_t)port);

  UdpAgent agent;
  if (!agent.open_and_bind((uint16_t)port)) return 1;

  CommandHandler handler;

  std::cout << "Worker started. id=" << id << " udpPort=" << port << " master=" << masterIp << "\n";
  agent.run((uint8_t)id, masterAddr, handler);

  agent.close();
  return 0;
}
