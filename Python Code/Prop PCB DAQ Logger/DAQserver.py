import threading
import serial
import csv
import time
from flask import Flask, render_template, jsonify
import os
import math
from datetime import datetime

app = Flask(__name__)

data = {}
channels = ['Ch0:', 'Ch1:', 'Ch2:', 'Ch3:']

def read_csv_data():
    csv_filename = os.path.join('log', 'data_log.csv')
    while True:
        with open(csv_filename, 'r') as csv_file:
            csv_reader = csv.DictReader(csv_file)
            last_row = None
            for row in csv_reader:
                last_row = row
            if last_row:
                data['Timestamp'] = last_row['Timestamp']
                for chip in ['ADS1015_1', 'ADS1015_2', 'ADS1115_1', 'ADS1115_2']:
                    for channel in channels:
                        key = f'{chip} {channel} Mean'
                        data[key] = float(last_row[key])
        time.sleep(0.05)  # Read the CSV file every 50 ms

@app.route('/')
def index():
    return render_template('index.html', channels=channels)

@app.route('/get_data')
def get_data():
    mean_data = {}
    for chip in ['ADS1015_1', 'ADS1015_2', 'ADS1115_1', 'ADS1115_2']:
        chip_mean_data = {}
        for channel in channels:
            key = f'{chip} {channel} Mean'
            chip_mean_data[channel] = data[key]
        mean_data[chip] = chip_mean_data
    return jsonify(mean_data)

if __name__ == '__main__':
    os.makedirs('log', exist_ok=True)
    csv_thread = threading.Thread(target=read_csv_data)
    csv_thread.daemon = True
    csv_thread.start()

    app.run(debug=False)
    try:
        while True:
            pass  # Keep the main thread running
    except KeyboardInterrupt:
        print("Exiting...")
        csv_thread.join()
