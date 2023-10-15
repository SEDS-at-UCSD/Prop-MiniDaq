import threading
import subprocess

from flask import Flask, render_template, jsonify
from datetime import datetime

app = Flask(__name__)

def run_daq_serial():
    subprocess.run(["python", "DAQserial.py"])

def run_daq_server():
    subprocess.run(["python", "DAQserver.py"])

if __name__ == '__main__':
    daq_serial_thread = threading.Thread(target=run_daq_serial)
    daq_server_thread = threading.Thread(target=run_daq_server)

    daq_serial_thread.daemon = True
    daq_server_thread.daemon = True

    daq_serial_thread.start()
    daq_server_thread.start()

    try:
        while True:
            pass  # Keep the main thread running
    except KeyboardInterrupt:
        print("Exiting...")
        daq_serial_thread.join()
        daq_server_thread.join()

