import socket
from fastapi import FastAPI
from fastapi.responses import HTMLResponse
from fastapi.staticfiles import StaticFiles

APP_HOST = "127.0.0.1"
APP_PORT = 7000  # IPC port to master_router

app = FastAPI()

app.mount("/static", StaticFiles(directory="static"), name="static")


def ipc_request(json_line: str) -> str:
    with socket.create_connection((APP_HOST, APP_PORT), timeout=2) as s:
        s.sendall((json_line.strip() + "\n").encode("utf-8"))
        data = b""
        while not data.endswith(b"\n"):
            chunk = s.recv(4096)
            if not chunk:
                break
            data += chunk
        return data.decode("utf-8").strip()


@app.get("/", response_class=HTMLResponse)
def index():
    with open("static/index.html", "r", encoding="utf-8") as f:
        return f.read()


@app.get("/api/nodes")
def api_nodes():
    resp = ipc_request('{"op":"nodes"}')
    # FastAPI dict'e çevirmek yerine JSON string dönmek istemiyoruz; ama minimal olsun diye eval yok.
    # Bu endpointi doğrudan JSON string olarak döndürmek için:
    return __import__("json").loads(resp)


@app.post("/api/command")
def api_command(target: int, action: str, arg: str = ""):
    req = f'{{"op":"command","target":{target},"action":"{action}","arg":"{arg}"}}'
    resp = ipc_request(req)
    return __import__("json").loads(resp)

@app.get("/api/telemetry/{node_id}")
def api_telemetry(node_id: int):
    resp = ipc_request(f'{{"op":"telemetry","id":{node_id}}}')
    return __import__("json").loads(resp)