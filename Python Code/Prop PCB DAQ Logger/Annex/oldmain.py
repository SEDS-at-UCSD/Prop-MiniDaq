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
comport = '/dev/cu.usbserial-0001'
readcountpersecond = [0];

def read_serial():
    try:
        ser = serial.Serial(comport, 115200)
    except serial.serialutil.SerialException as e:
        print("SerialException:", e)
        raise e  # Raising the exception to stop the main thread

    csv_filename = os.path.join('log', 'data_log.csv')
    if not os.path.exists(csv_filename):
        with open(csv_filename, 'w', newline='') as csv_file:
            csv_writer = csv.writer(csv_file)
            header = ['Timestamp']
            for chip in ['ADS1015_1', 'ADS1015_2', 'ADS1115_1', 'ADS1115_2']:
                for channel in channels:
                    header.extend([f'{chip} {channel} Raw', f'{chip} {channel} Mean'])
            csv_writer.writerow(header)

    for item in ['0 1', '1 1', '2 1', '3 1']:
        ser.write(('printADS'+item+'\n').encode())
        try:
            line = ser.readline().decode().strip()
            print(line)
        except UnicodeDecodeError:
            continue  # Skip decoding errors
    print("Finished requesting print ADS")
    time.sleep(0.5)

    while True:
        try:
            line = ser.readline().decode().strip()
        except UnicodeDecodeError:
            continue  # Skip decoding errors
        if line.startswith('--- start ADS read ---'):
            readcountpersecond[0] = readcountpersecond[0] + 1;
            #print("Processing ADS!")
            data = {}
            for chip in ['ADS1015_1', 'ADS1015_2', 'ADS1115_1', 'ADS1115_2']:
                for channel in channels:
                    data[f'{chip} {channel} Raw'] = math.nan
                    data[f'{chip} {channel} Mean'] = math.nan
            while True:
                try:
                    line = ser.readline().decode().strip()
                    #print(line)
                except UnicodeDecodeError:
                    continue  # Skip decoding errors
                
                if line.startswith('--- end ADS read ---'):
                    break
                if not line.startswith('ADS'):
                    continue  # Skip lines not starting with 'ADS'
                parts = line.split()
                
                if len(parts) >= 5:
                    chip = parts[0]
                    channel = parts[1]
                    try:
                        raw_value = float(parts[2])
                        mean_value = float(parts[4])
                    except ValueError:
                        print(f"Corrupted Print Skipping line: {line}")
                        continue  # Skip processing this line
                else:
                    print(f"Skipping line: {line}")
                    continue  # Skip processing this line

                #print(raw_value,mean_value)
                data[f'{chip} {channel} Raw'] = raw_value
                data[f'{chip} {channel} Mean'] = mean_value

                #print(data)
            data['Timestamp'] = datetime.utcnow().isoformat(sep=' ', timespec='milliseconds')
            with open(csv_filename, 'a', newline='') as csv_file:
                csv_writer = csv.writer(csv_file)
                row = [data['Timestamp']]
                for chip in ['ADS1015_1', 'ADS1015_2', 'ADS1115_1', 'ADS1115_2']:
                    for channel in channels:
                        row.extend([data[f'{chip} {channel} Raw'], data[f'{chip} {channel} Mean']])
                csv_writer.writerow(row)

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
    serial_thread = threading.Thread(target=read_serial)
    csv_thread = threading.Thread(target=read_csv_data)
    serial_thread.daemon = True
    csv_thread.daemon = True
    serial_thread.start()
    csv_thread.start()

    try:
        start_time = time.time();
        while True:
            curr_time = time.time();
            if (curr_time - start_time >= 1):
                print("Read Count Per Second:", readcountpersecond[0]/(curr_time - start_time));
                readcountpersecond[0] = 0;
                start_time = curr_time;
            time.sleep(0.1);
            pass  # Keep the main thread running
    except KeyboardInterrupt:
        serial_thread.join()
        serial_thread.join()
        print("Exiting...")
    app.run(debug=True)
