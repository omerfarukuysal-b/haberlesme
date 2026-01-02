import socket
import json
import struct
import threading
from fastapi import FastAPI
from fastapi.responses import HTMLResponse
from fastapi.staticfiles import StaticFiles
from typing import Dict, List
from datetime import datetime
from pathlib import Path

# Config dosyasından node bilgilerini oku
CONFIG_FILE = Path(__file__).parent / "config.json"
with open(CONFIG_FILE, "r") as f:
    config = json.load(f)

NODES_CONFIG = {node["id"]: node for node in config["nodes"]}
WEB_CONFIG = config.get("web_server", {"host": "0.0.0.0", "port": 8000})
HEARTBEAT_PORT = config.get("heartbeat_port", 9000)  # Heartbeat dinleme portu

# Listening node (Node 1'e bağlan)
LISTENING_NODE = NODES_CONFIG[1]  # Node 1 heartbeat'leri web'e gönderecek

app = FastAPI()
app.mount("/static", StaticFiles(directory="static"), name="static")

# Online node'ları track et
online_nodes: Dict[int, Dict] = {}
nodes_lock = threading.Lock()

# UDP socket
udp_socket = None

def init_socket():
    """UDP socket'i başlat"""
    global udp_socket
    if udp_socket is None:
        udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        udp_socket.settimeout(2)

def start_heartbeat_listener():
    """Node 1'den heartbeat'leri dinle (ayrı thread'te)"""
    init_socket()
    listening_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    listening_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    listening_socket.bind(("0.0.0.0", HEARTBEAT_PORT))
    listening_socket.settimeout(1)
    
    print(f"[Web App] Listening for heartbeats on port {HEARTBEAT_PORT}...")
    
    while True:
        try:
            data, addr = listening_socket.recvfrom(4096)
            if len(data) >= 26:
                # Header'i aç
                header_data = struct.unpack("<IBBBBIIIHH", data[:26])
                magic, version, msg_type, sender_id, reserved, seq, reply_to_seq, ts_ms, payload_len = header_data
                
                # Type: 10 = Heartbeat
                if msg_type == 10 and magic == 0x52504934:  # "RPI4"
                    print(f"[Web App] Heartbeat received from Node {sender_id} ({addr[0]}:{addr[1]}) - Size: {len(data)} bytes")
                    payload_str = data[26:26+payload_len].decode("utf-8", errors="ignore")
                    try:
                        payload = json.loads(payload_str)
                        
                        # Online node'ları güncelle
                        with nodes_lock:
                            node_info = NODES_CONFIG.get(sender_id)
                            if node_info:
                                online_nodes[sender_id] = {
                                    "id": sender_id,
                                    "hostname": node_info["hostname"],
                                    "ip": node_info["ip"],
                                    "port": node_info["port"],
                                    "lastSeen": datetime.now().isoformat(),
                                    "telemetry": payload.get("telemetry", {})
                                }
                                print(f"[Web App] Node {sender_id} data updated. Telemetry: {payload.get('telemetry', {})}")
                    except Exception as e:
                        print(f"[Web App] Error parsing heartbeat from Node {sender_id}: {e}")
        except socket.timeout:
            pass
        except Exception as e:
            print(f"[Web App] Heartbeat listener error: {e}")
            import time
            time.sleep(0.1)

# Heartbeat listener thread'ini başlat
heartbeat_thread = threading.Thread(target=start_heartbeat_listener, daemon=True)
heartbeat_thread.start()

def send_udp_command(target_id: int, action: str, arg: str = "") -> str:
    """Node'a UDP üzerinden komut gönder ve yanıt al"""
    init_socket()
    
    if target_id not in NODES_CONFIG:
        return json.dumps({"ok": False, "error": f"Node {target_id} not found in config"})
    
    node = NODES_CONFIG[target_id]
    
    try:
        # Komut payload'ını oluştur
        cmd_payload = {
            "cmdId": 1,
            "action": action,
            "arg": arg
        }
        payload_str = json.dumps(cmd_payload)
        payload_bytes = payload_str.encode("utf-8")
        
        # Header oluştur
        magic = 0x52504934  # "RPI4"
        version = 1
        msg_type = 20  # Command
        sender_id = 0  # Web app
        reserved = 0
        seq = 1
        reply_to_seq = 0
        ts_ms = 0
        payload_len = len(payload_bytes)
        
        header = struct.pack("<IBBBBIIIHH",
                           magic, version, msg_type, sender_id, reserved,
                           seq, reply_to_seq, ts_ms, payload_len)
        
        packet = header + payload_bytes
        
        # Node'a gönder
        print(f"[Web App] Sending command to Node {target_id} ({node['ip']}:{node['port']}): {action}")
        udp_socket.sendto(packet, (node["ip"], node["port"]))
        
        # Yanıt al
        try:
            data, _ = udp_socket.recvfrom(4096)
            if len(data) >= 26:
                response_payload = data[26:]
                response_str = response_payload.decode("utf-8", errors="ignore")
                print(f"[Web App] Response received from Node {target_id}: {response_str}")
                return response_str
        except socket.timeout:
            print(f"[Web App] Timeout waiting for response from Node {target_id}")
            return json.dumps({"ok": False, "error": "Timeout waiting for response"})
        
    except Exception as e:
        print(f"[Web App] Error sending command to Node {target_id}: {e}")
        return json.dumps({"ok": False, "error": str(e)})


@app.get("/", response_class=HTMLResponse)
def index():
    """Ana sayfa"""
    with open("static/index.html", "r", encoding="utf-8") as f:
        return f.read()


@app.get("/api/nodes")
def api_nodes():
    """Online olan node'ları döndür (heartbeat ile dinamik güncellenir)"""
    with nodes_lock:
        nodes_list = []
        current_time = datetime.now()
        
        # Tüm config'deki node'ları döndür
        for node_id, node_config in NODES_CONFIG.items():
            if node_id in online_nodes:
                node_data = online_nodes[node_id]
                last_seen = datetime.fromisoformat(node_data["lastSeen"])
                time_diff = (current_time - last_seen).total_seconds()
                status = "online" if time_diff < 5 else "offline"
            else:
                status = "offline"
                node_data = None
            
            nodes_list.append({
                "id": node_id,
                "hostname": node_config["hostname"],
                "ip": node_config["ip"],
                "port": node_config["port"],
                "status": status,
                "telemetry": node_data.get("telemetry", {}) if node_data else {}
            })
        
        return {"nodes": sorted(nodes_list, key=lambda x: x["id"])}


@app.post("/api/command")
def api_command(target: int, action: str, arg: str = ""):
    """Hedef node'a komut gönder"""
    response = send_udp_command(target, action, arg)
    try:
        return json.loads(response)
    except:
        return {"ok": False, "error": "Invalid response from node"}


@app.get("/api/telemetry/{node_id}")
def api_telemetry(node_id: int):
    """Node'dan saklı telemetry bilgisini döndür"""
    with nodes_lock:
        if node_id not in online_nodes:
            return {"ok": False, "error": f"Node {node_id} not online"}
        return online_nodes[node_id].get("telemetry", {})


if __name__ == "__main__":
    import uvicorn
    host = WEB_CONFIG.get("host", "0.0.0.0")
    port = WEB_CONFIG.get("port", 8000)
    print(f"\n[Web App] Web server starting on {host}:{port}")
    print(f"[Web App] Listening for heartbeats from Node 1 on UDP port {HEARTBEAT_PORT}")
    print(f"[Web App] Node 1 configuration: {LISTENING_NODE['ip']}:{LISTENING_NODE['port']}")
    print(f"[Web App] Make sure Node 1 is running with: --web-server <YOUR_IP> --web-port {HEARTBEAT_PORT}\n")
    uvicorn.run(app, host=host, port=port)