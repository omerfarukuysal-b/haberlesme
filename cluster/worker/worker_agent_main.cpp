#include "udp_agent.h"
#include "command_handler.h"
#include "../common/net_utils.h"
#include "../common/node_config.h"
#include "telemetry_collector.h"
#include <atomic>
#include <csignal>
#include <iostream>

std::atomic<bool> g_running{true};
static void on_sig(int) { g_running = false; }

int main(int argc, char **argv)
{
    std::signal(SIGINT, on_sig);
    std::signal(SIGTERM, on_sig);

    int id = 0;
    std::string masterIp;
    int port = 50000;
    bool meshMode = false;
    std::string webServerIp;
    int webServerPort = 0;

    // Mesh mode'ta tüm node'ların bilgileri gerekli
    mesh::MeshNetwork network;

    for (int i = 1; i < argc; i++)
    {
        std::string a = argv[i];
        if (a == "--id" && i + 1 < argc)
            id = std::stoi(argv[++i]);
        else if (a == "--master" && i + 1 < argc)
            masterIp = argv[++i];
        else if (a == "--port" && i + 1 < argc)
            port = std::stoi(argv[++i]);
        else if (a == "--mesh")
            meshMode = true;
        else if (a == "--web-server" && i + 1 < argc)
            webServerIp = argv[++i];
        else if (a == "--web-port" && i + 1 < argc)
            webServerPort = std::stoi(argv[++i]);
        else if (a == "--add-node" && i + 2 < argc)
        {
            // Format: --add-node <id> <hostname>:<ip>:<port>
            uint8_t nodeId = (uint8_t)std::stoi(argv[++i]);
            std::string nodeInfo = argv[++i];
            // Parse: hostname:ip:port
            size_t colon1 = nodeInfo.find(':');
            size_t colon2 = nodeInfo.rfind(':');
            if (colon1 != std::string::npos && colon2 != std::string::npos && colon1 != colon2)
            {
                std::string hostname = nodeInfo.substr(0, colon1);
                std::string nodeIp = nodeInfo.substr(colon1 + 1, colon2 - colon1 - 1);
                uint16_t nodePort = (uint16_t)std::stoi(nodeInfo.substr(colon2 + 1));
                network.add_node(nodeId, hostname, nodeIp, nodePort);
            }
        }
    }

    if (id <= 0 || id > 255)
    {
        std::cerr << "Usage: " << argv[0] << " --id <1..255> [--port 50000]\n";
        std::cerr << "       [--mesh [--add-node <id> <hostname>:<ip>:<port>]...]\n";
        std::cerr << "       [--web-server <ip> --web-port <port>]\n";
        std::cerr << "       [--master <ip> (master-slave mode)]\n\n";
        std::cerr << "Examples:\n";
        std::cerr << "  Mesh Node 1 (heartbeats go to web server):\n";
        std::cerr << "    ./worker_agent --id 1 --port 50000 --mesh \\\n";
        std::cerr << "      --add-node 2 node2:10.0.0.2:50000 \\\n";
        std::cerr << "      --add-node 3 node3:10.0.0.3:50000 \\\n";
        std::cerr << "      --web-server 10.0.0.10 --web-port 8000\n\n";
        std::cerr << "  Mesh Node 2 (no web server needed):\n";
        std::cerr << "    ./worker_agent --id 2 --port 50000 --mesh \\\n";
        std::cerr << "      --add-node 1 node1:10.0.0.1:50000 \\\n";
        std::cerr << "      --add-node 3 node3:10.0.0.3:50000\n";
        return 1;
    }

    TelemetryCollector collector;
    UdpAgent agent;
    if (!agent.open_and_bind((uint16_t)port))
        return 1;

    CommandHandler handler;

    if (meshMode)
    {
        if (network.node_count() == 0)
        {
            std::cerr << "Mesh mode için en az bir başka node eklenmesi gerekiyor (--add-node)\n";
            return 1;
        }
        std::cout << "\n========================================\n";
        std::cout << "Node " << id << " - MESH MODE\n";
        std::cout << "========================================\n";
        std::cout << "Local Port: " << port << "\n";
        std::cout << "Mesh Nodes:\n";
        for (const auto &n : network.get_all_nodes())
        {
            if (n.id != id)
            {
                std::cout << "  - Node " << (int)n.id << ": " << n.ip << ":" << n.port << "\n";
            }
        }
        if (!webServerIp.empty())
        {
            std::cout << "Web Server: " << webServerIp << ":" << webServerPort << "\n";
        }
        std::cout << "========================================\n\n";
        agent.run_mesh((uint8_t)id, network, handler, collector, webServerIp, (uint16_t)webServerPort);
    }
    else
    {
        if (masterIp.empty())
        {
            std::cerr << "Master-slave mode için --master <master_ip> gerekli\n";
            return 1;
        }
        sockaddr_in masterAddr = netu::make_ipv4_addr(masterIp, (uint16_t)port);
        std::cout << "Worker started. id=" << id << " port=" << port << " master=" << masterIp << "\n";
        agent.run((uint8_t)id, masterAddr, handler, collector);
    }

    agent.close();
    return 0;
}
