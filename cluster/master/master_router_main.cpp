#include "udp_listener.h"
#include "ipc_server.h"
#include "node_registry.h"
#include "command_broker.h"

#include <atomic>
#include <csignal>
#include <iostream>
#include <thread>
#include "../common/runtime.h"


std::atomic<bool> g_running{true};

static void on_sig(int){ g_running=false; }

int main(int argc, char** argv) {
  std::signal(SIGINT, on_sig);
  std::signal(SIGTERM, on_sig);

  int udpPort = 50000;
  int ipcPort = 7000;

  for (int i=1;i<argc;i++){
    std::string a=argv[i];
    if (a=="--udp" && i+1<argc) udpPort = std::stoi(argv[++i]);
    else if (a=="--ipc" && i+1<argc) ipcPort = std::stoi(argv[++i]);
  }

  NodeRegistry registry;
  CommandBroker broker;
  UdpListener udp;
  IpcServer ipc;

  if (!udp.open_and_bind((uint16_t)udpPort)) return 1;
  if (!ipc.listen_local((uint16_t)ipcPort)) return 1;

  std::cout << "MasterRouter started. UDP:" << udpPort << " IPC:127.0.0.1:" << ipcPort << "\n";

  std::thread udpThread([&]{ udp.run(registry, broker); });
  std::thread ipcThread([&]{ ipc.run(registry, broker, udp); });

  udpThread.join();
  ipcThread.join();

  ipc.close();
  udp.close();
  return 0;
}
