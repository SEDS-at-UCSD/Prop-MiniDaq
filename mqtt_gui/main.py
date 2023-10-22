from flask import Flask, render_template
from flask_socketio import SocketIO
import serial
import time
import json
from datetime import datetime
import threading 
from threading import Event


app = Flask(__name__)
socketio = SocketIO(app)

# Replace 'COMX' with the actual COM port where your Arduino or device is connected.
ser = serial.Serial('/dev/cu.usbserial-0001', 921600)  # Adjust the baud rate as needed.

# Open a text file for appending
gui_log_file = open('gui_serial.txt', 'w')
gui_log_file_read = open('gui_serial.txt', 'r')
raw_log_file = open('raw_serial_log.txt', 'a')



@socketio.on('connect', namespace='/')
def connect():
    print('Client connected')

@app.route('/')
def index():
    return render_template('index.html')


event = Event()
read_event = Event()

datatopass = [""]



def read_serial_and_log_high_freq():
    while True:
        try:
            # Read a line of data from the Serial Monitor
            data = ser.readline().decode('utf-8').strip()
            data_dict = json.loads(data)
            #print (data_dict)
            # Log the data to a text file
            data_formatted = str(datetime.now())[11:] + " " + str(data_dict['BoardID']) + "  " + str(data_dict['Sensors'][0]) + "  " + str(data_dict['Sensors'][1]) + "  " + str(data_dict['Sensors'][2]) + "  " + str(data_dict['Sensors'][3]) + "\r\n"
            if event.is_set:
                datatopass[0] = data_formatted
                read_event.set
            #print (data_dict)
            raw_log_file.write(data_formatted)
            raw_log_file.flush()  # Flush the buffer to ensure data is written immediately

        except Exception as e:
            print(f"Serial read error: {e}")


def read_serial_and_log():
    while True:
        try:
            event.set
            #while (read_event.is_set != True):
            time.sleep(0.05)
            print(datatopass[0] + '<br>\n')
            gui_log_file.write(datatopass[0])
            gui_log_file.flush()  # Flush the buffer to ensure data is written immediately
            socketio.emit('serial_data', datatopass[0])
            with open('gui_serial.txt', 'r') as log:
                data = log.read().replace('\r\n', '<br>')
                print(data)
            socketio.emit('log_data', data)
            time.sleep(0.5)
            print("data to GUI")
        except Exception as e:
            print(f"Read log file error: {e}")



if __name__ == '__main__':
    #socketio.start_background_task(target=read_serial_and_log)
    #socketio.start_background_task(target=read_log_file)
    #socketio.start_background_task(target=read_serial_and_log_high_freq)

    t1 = threading.Thread(target=read_serial_and_log)
    #t2 = threading.Thread(target=read_log_file)
    t3 = threading.Thread(target=read_serial_and_log_high_freq)
    
    t1.start()
    #t2.start()
    t3.start()
    socketio.run(app, host='0.0.0.0', port=4000)