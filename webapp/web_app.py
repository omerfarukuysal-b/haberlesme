import socket
import json
import struct
from fastapi import FastAPI
from fastapi.responses import HTMLResponse, JSONResponse
from fastapi.staticfiles import StaticFiles
from typing import Dict, List

APP_HOST = "127.0.0.1"
APP_PORT = 8000  # Web server port

# Mesh network node konfigürasyonu
# Format: { "id": { "hostname": "...", "ip": "...", "port": ... } }
MESH_NODES: Dict[int, Dict] = {
    1: {"hostname": "node1", "ip": "127.0.0.1", "port": 50001},
    2: {"hostname": "node2", "ip": "127.0.0.1", "port": 50002},
    3: {"hostname": "node3", "ip": "127.0.0.1", "port": 50003},
}

app = FastAPI()
app.mount("/static", StaticFiles(directory="static"), name="static")

# UDP socket'i global olarak tut
udp_socket = None

def init_socket():
    """UDP socket'i başlat"""
    global udp_socket
    if udp_socket is None:
        udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        udp_socket.settimeout(2)

def send_udp_command(target_id: int, action: str, arg: str = "") -> str:
    """
    Node'a UDP üzerinden komut gönder ve yanıt al
    
    Protocol:
    - Header: magic(4) | version(1) | type(1) | senderId(1) | reserved(1) | seq(4) | replyToSeq(4) | tsMs(8) | payloadLen(2)
    - Payload: JSON
    """
    init_socket()
    
    if target_id not in MESH_NODES:
        return json.dumps({"ok": False, "error": f"Node {target_id} not found"})
    
    node = MESH_NODES[target_id]
    
    try:
        # Komut payload'ını oluştur
        cmd_payload = {
            "cmdId": 1,
            "action": action,
            "arg": arg
        }
        payload_str = json.dumps(cmd_payload)
        payload_bytes = payload_str.encode("utf-8")
        
        # Basit header: magic | version | type | senderId | reserved | seq | replyToSeq | tsMs | payloadLen
        # Type: 20 = Command, 21 = Response
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
        udp_socket.sendto(packet, (node["ip"], node["port"]))
        
        # Yanıt al (Response mesajı)
        try:
            data, _ = udp_socket.recvfrom(4096)
            # Header'i aç
            if len(data) >= 26:  # Header minimum uzunluğu
                header_data = data[:26]
                response_payload = data[26:]
                
                # Response payload'ı parse et
                response_str = response_payload.decode("utf-8", errors="ignore")
                return response_str
        except socket.timeout:
            return json.dumps({"ok": False, "error": "Timeout waiting for response"})
        
    except Exception as e:
        return json.dumps({"ok": False, "error": str(e)})


@app.get("/", response_class=HTMLResponse)
def index():
    """Ana sayfa - HTML arayüzü döndür"""
    with open("static/index.html", "r", encoding="utf-8") as f:
        return f.read()


@app.get("/api/nodes")
def api_nodes():
    """Tüm mesh node'larını döndür"""
    nodes_list = []
    for node_id, node_info in MESH_NODES.items():
        nodes_list.append({
            "id": node_id,
            "hostname": node_info["hostname"],
            "ip": node_info["ip"],
            "port": node_info["port"],
            "status": "online"  # Gelecekte heartbeat ile kontrol edilebilir
        })
    return {"nodes": nodes_list}


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
    """Node'dan telemetry bilgisi al"""
    response = send_udp_command(node_id, "telemetry", "")
    try:
        return json.loads(response)
    except:
        return {"ok": False, "error": f"Could not get telemetry from node {node_id}"}


if __name__ == "__main__":
    import uvicorn
    init_socket()
    uvicorn.run(app, host=APP_HOST, port=APP_PORT)