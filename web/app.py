import os
from flask import Flask, request, render_template, jsonify
import subprocess

app = Flask(__name__)

# Dosya yollarını dinamik olarak ayarla
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
CLIENT_EXECUTABLE = os.path.join(BASE_DIR, "../compiled/ping_client")
SERVER_EXECUTABLE = os.path.join(BASE_DIR, "../compiled/ping_server")

@app.route("/")
def index():
    return render_template("index2.html")

@app.route("/start_ping", methods=["POST"])
def start_ping():
    server_ip = request.form["server_ip"]
    dest_ip = request.form["dest_ip"]

    # Server'ı başlat
    server_process = subprocess.Popen(    #asenkron çalışmaya devam eder
        ["sudo", SERVER_EXECUTABLE], stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )

    # Client'ı çalıştır
    client_output = subprocess.run(
        [CLIENT_EXECUTABLE, server_ip, dest_ip],
        capture_output=True,
        text=True     #Yakalanan çıktının byte formatında değil, bir metin (string) olarak döndürülmesini sağlar.
    )

    return jsonify({
        "client_output": client_output.stdout,
        "server_log": server_process.stdout.read().decode()
    })

if __name__ == "__main__":
    app.run(debug=True)
