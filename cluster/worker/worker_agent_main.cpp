#include "udp_agent.h"
#include "command_handler.h"
#include "../common/net_utils.h"
#include "../common/node_config.h"

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
  bool meshMode = false;

  // Mesh mode'ta tüm node'ların bilgileri gerekli
  mesh::MeshNetwork network;

  for (int i=1;i<argc;i++){
    std::string a=argv[i];
    if (a=="--id" && i+1<argc) id = std::stoi(argv[++i]);
    else if (a=="--master" && i+1<argc) masterIp = argv[++i];
    else if (a=="--port" && i+1<argc) port = std::stoi(argv[++i]);
    else if (a=="--mesh") meshMode = true;
    else if (a=="--add-node" && i+2<argc) {
      // Format: --add-node <id> <hostname>:<ip>:<port>
      uint8_t nodeId = (uint8_t)std::stoi(argv[++i]);
      std::string nodeInfo = argv[++i];
      // Parse: hostname:ip:port
      size_t colon1 = nodeInfo.find(':');
      size_t colon2 = nodeInfo.rfind(':');
      if (colon1 != std::string::npos && colon2 != std::string::npos && colon1 != colon2) {
        std::string hostname = nodeInfo.substr(0, colon1);
        std::string nodeIp = nodeInfo.substr(colon1+1, colon2-colon1-1);
        uint16_t nodePort = (uint16_t)std::stoi(nodeInfo.substr(colon2+1));
        network.add_node(nodeId, hostname, nodeIp, nodePort);
      }
    }
  }

  if (id <= 0 || id > 255) {
    std::cerr << "Usage: " << argv[0] << " --id <1..255> [--master <master_ip>] [--port 50000] [--mesh] [--add-node <id> <hostname>:<ip>:<port>]\n";
    std::cerr << "  --mesh: Mesh mode'u etkinleştir (her node'un diğer node'lara komut göndermesi)\n";
    std::cerr << "  --add-node: Mesh'e node ekle (birden fazla kez kullanılabilir)\n";
    return 1;
  }

  UdpAgent agent;
  if (!agent.open_and_bind((uint16_t)port)) return 1;

  CommandHandler handler;

  if (meshMode) {
    if (network.node_count() == 0) {
      std::cerr << "Mesh mode için en az bir başka node eklenmesi gerekiyor (--add-node)\n";
      return 1;
    }
    std::cout << "Mesh Network started. id=" << id << " udpPort=" << port 
              << " nodes=" << network.node_count() << "\n";
    agent.run_mesh((uint8_t)id, network, handler);
  } else {
    if (masterIp.empty()) {
      std::cerr << "Master-slave mode için --master <master_ip> gerekli\n";
      return 1;
    }
    sockaddr_in masterAddr = netu::make_ipv4_addr(masterIp, (uint16_t)port);
    std::cout << "Worker started. id=" << id << " udpPort=" << port << " master=" << masterIp << "\n";
    agent.run((uint8_t)id, masterAddr, handler);
  }

  agent.close();
  return 0;
}
