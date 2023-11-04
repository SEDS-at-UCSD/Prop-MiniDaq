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
mqtt_log_1015 = "log_data_1015"
mqtt_log_1115 = "log_data_1115"

client = mqtt.Client("propdaq")

# Replace 'COMX' with the actual COM port where your Arduino or device is connected.
ser = serial.Serial('/dev/cu.usbserial-0001', 921600)  # Adjust the baud rate as needed.

# Open a text file for appending
gui_log_file = open('gui_serial.txt', 'w')
raw_log_file = open('raw_serial_log.txt', 'a')


event = Event()
read_event = Event()

datatopass = ["", ""]
data_lock = threading.Lock()

def on_connect(client, userdata, flags, rc):
    print("Connected to MQTT broker with result code "+str(rc))
    client.subscribe(mqtt_topic_serial)

def on_message(client, userdata, message):
    print(f"Received message on topic '{message.topic}': {message.payload.decode('utf-8')}")

def read_serial_and_log_high_freq():
    while True:
        try:
            # Read a line of data from the Serial Monitor
            data = ser.readline().decode('utf-8').strip()
            data_dict = json.loads(data)
            #print(data_dict)
            # Log the data to a text fileß
            data_formatted = (
                str(datetime.now())[11:] 
                + " "
                + str(data_dict['BoardID'])
                + "  "
                + str(data_dict['SensorType'])
                + "  ")
            for i in range(len(data_dict['Sensors'])):
                data_formatted += str(data_dict['Sensors'][i]) + "  "
            #print (data_formatted)
            
            with data_lock:
                datatopass[0] = data_formatted
                datatopass[1] = data_dict['SensorType']
                raw_log_file.write(data_formatted)
                raw_log_file.flush()  # Flush the buffer to ensure data is written immediately

        except Exception as e:
            print(f"Serial read error: {e}, {data}")
            

def publish_data():
    while True:
        with data_lock:
            if datatopass[1] == 'ADS1015':
                client.publish(mqtt_log_1015, datatopass[0])
            elif datatopass[1] == 'ADS1115':
                client.publish(mqtt_log_1115, datatopass[0])
        

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



