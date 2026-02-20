from http.server import BaseHTTPRequestHandler, HTTPServer
import json
import socket
import threading

#run 'python bridgebrige.py' 
#open html live server from vscode rightclick

# configuration
ESP_IP = ""
ESP_PORT = 1234
LOG = []

# global socket object
esp_sock = None

def log(message):
    """helper function to print AND log to the UI."""
    print(message)
    LOG.append(message)
    if len(LOG) > 200:  # keep the log size smal
        LOG.pop(0)

# handle commands from browser
class Handler(BaseHTTPRequestHandler):
    def _set_ok(self):
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()

    def do_OPTIONS(self):
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def do_POST(self):
        global esp_sock
        if self.path != "/command":
            self.send_error(404)
            return

        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length).decode("utf-8").strip().lower()

        LOG.append(body)
        print(f"UI sent command: {body}")

        # map command to byte codes
        # WHATS GETTING SENT OVER TO ESP32 
        command_map = {
            "open": b"open\n",
            "close": b"close\n",
            "shutdown": b"shutdown\n",
            "boom_open": b"boom_open\n",
            "boom_close": b"boom_close\n",
            "traffic_green": b"traffic_green\n",
            "traffic_yellow": b"traffic_yellow\n",
            "traffic_red": b"traffic_red\n",
            "traffic_off": b"traffic_off\n",
            "mode_manual": b"mode_manual\n", 
            "mode_auto": b"mode_auto\n"
        }

        # send to ESP if connected
        if esp_sock:
            try:
                if body in command_map:
                    esp_sock.sendall(command_map[body])
                    print(f"Sent to ESP32: {command_map[body].decode().strip()}")
                    log(f"Sent to ESP32: {command_map[body].decode().strip()}")
                else:
                    print("Unknown command")
                    log("Unknown command from UI")
            except Exception as e:
                print("Error sending to ESP32:", e)
                log(f"Error sending to ESP32: {e}")
                
        else:
            print("ESP32 not connected")
            log("ESP32 not connected yet")

        self._set_ok()
        self.wfile.write(json.dumps({"ok": True, "received": body}).encode("utf-8"))

    def do_GET(self):
        if self.path == "/log":
            self._set_ok()
            self.wfile.write(json.dumps({"log": LOG}).encode("utf-8"))
        else:
            self.send_error(404)

# connect to ESP32 in separate thread
def connect_to_esp():
    global esp_sock
    try:
        sock = socket.create_connection((ESP_IP, ESP_PORT), timeout=5)
        esp_sock = sock
        print(f"Connected to ESP32 at {ESP_IP}:{ESP_PORT}")
        log(f"Connected to ESP32 at {ESP_IP}:{ESP_PORT}")
        
        # optionally send greeting
        sock.sendall(b"hello from python\n")
        
        # read loop (optional)
        with sock.makefile("r") as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                try:
                    msg = json.loads(line)
                    print("Received JSON from ESP32:", msg)
                    log(f"Received JSON from ESP32: {msg}")
                except json.JSONDecodeError:
                    print("Bad JSON from ESP32:", line)
                    log(f"Bad JSON from ESP32: {line}")
    except Exception as e:
        print("Failed to connect to ESP32:", e)
        log(f"Failed to connect to ESP32: {e}")

if __name__ == "__main__":
    # start ESP connection in background
    threading.Thread(target=connect_to_esp, daemon=True).start()

    # start HTTP server for frontend uI 
    print("Starting HTTP server on http://127.0.0.1:5501/main/frontend/index.html")
    HTTPServer(("127.0.0.1", 1234), Handler).serve_forever()
