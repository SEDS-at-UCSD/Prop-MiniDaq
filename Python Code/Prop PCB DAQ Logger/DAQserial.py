import threading
import serial
import csv
import time
import os
import math
from datetime import datetime

data = {}
channels = ['Ch0:', 'Ch1:', 'Ch2:', 'Ch3:']
comport = '/dev/cu.usbserial-0001'
#comport = '/dev/cu.usbserial-8'
readcountpersecond = [0];

def read_serial():
    print("Beginning Serial Process")
    try:
        ser = serial.Serial(comport, 921600)
        print("Serial Port Connected")
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
    time.sleep(0.1)

    while True:
        try:
            line = ser.readline().decode().strip()
            #print(line)
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
                #print(line)
                if line.startswith('--- end ADS read ---'):
                    break
                
                if line.startswith('Temp:'):
                    print(line)
                if not line.startswith('ADS'):
                    continue  # Skip lines not starting with 'ADS'
                parts = line.split()
                #print(parts)
                
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

if __name__ == '__main__':
    os.makedirs('log', exist_ok=True)
    serial_thread = threading.Thread(target=read_serial)
    serial_thread.daemon = True
    serial_thread.start()

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
        print("Exiting...")
        serial_thread.join()
