import paho.mqtt.client as mqtt
import serial
import time
import json
from datetime import datetime
import threading
from threading import Event

# MQTT configuration
mqtt_broker_address = "localhost"
mqtt_topic_serial = "serial_data"
mqtt_topic_log = "log_data"

client = mqtt.Client("web_server")

# Replace 'COMX' with the actual COM port where your Arduino or device is connected.
ser = serial.Serial('/dev/cu.usbserial-0001', 921600)  # Adjust the baud rate as needed.

# Open a text file for appending
gui_log_file = open('gui_serial.txt', 'w')
raw_log_file = open('raw_serial_log.txt', 'a')


event = Event()
read_event = Event()

datatopass = [""]
data_lock = threading.Lock()

def on_connect(client, userdata, flags, rc):
    print("Connected to MQTT broker with result code "+str(rc))
    client.subscribe(mqtt_topic_serial)

def on_message(client, userdata, msg):
    data = msg.payload.decode('utf-8')
    if msg.topic == mqtt_topic_serial:
        with data_lock:
            datatopass[0] = data

def read_serial_and_log_high_freq():
    while True:
        try:
            # Read a line of data from the Serial Monitor
            data = ser.readline().decode('utf-8').strip()
            data_dict = json.loads(data)
            # Log the data to a text file
            data_formatted = (
                str(datetime.now())[11:]
                + " "
                + str(data_dict['BoardID'])
                + "  "
                + str(data_dict['Sensors'][0])
                + "  "
                + str(data_dict['Sensors'][1])
                + "  "
                + str(data_dict['Sensors'][2])
                + "  "
                + str(data_dict['Sensors'][3])
                + "\r\n"
            )
            with data_lock:
                datatopass[0] = data_formatted

            raw_log_file.write(data_formatted)
            raw_log_file.flush()  # Flush the buffer to ensure data is written immediately

        except Exception as e:
            print(f"Serial read error: {e}")

def publish_data():
    while True:
        with data_lock:
            if datatopass[0]:
                client.publish(mqtt_topic_log, datatopass[0])
        time.sleep(0.5)

def main():
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(mqtt_broker_address, 1883, 60)
    client.loop_start()

    t1 = threading.Thread(target=read_serial_and_log_high_freq)
    t2 = threading.Thread(target=publish_data)

    t1.start()
    t2.start()

if __name__ == '__main__':
    main()
