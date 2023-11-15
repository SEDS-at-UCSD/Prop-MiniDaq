import paho.mqtt.client as mqtt
import serial
import time
import json
from datetime import datetime
import threading
import multiprocessing



# MQTT configuration
mqtt_broker_address = "localhost"
mqtt_topic_serial = "serial_data"

# MQTT Topics
b1_mqtt_log_1015 = "b1_log_data_1015"
b1_mqtt_log_1115 = "b1_log_data_1115"
b2_mqtt_log_1015 = "b2_log_data_1015"
b2_mqtt_log_1115 = "b2_log_data_1115"
b3_mqtt_log_1015 = "b3_log_data_1015"
b3_mqtt_log_1115 = "b3_log_data_1115"
b4_mqtt_log_1015 = "b4_log_data_1015"
b4_mqtt_log_1115 = "b4_log_data_1115"
b5_mqtt_log_1015 = "b5_log_data_1015"
b5_mqtt_log_1115 = "b5_log_data_1115"

mqtt_switch_states_update = "switch_states_update"
mqtt_switch_states_status = "switch_states_status"


with open('cf_config.json', 'r') as json_file:
    conv_configs = json.load(json_file)

cf_1015 = conv_configs['Conv_Factor_ADS1015']
cf_1115 = conv_configs['Conv_Factor_ADS1115']


client = mqtt.Client("propdaq")

# Port Management

ports = [False, False, False, False, False]

def open_serial_ports():
    try:
        ports[0] = serial.Serial('/dev/cu.usbserial-0001', 921600)  
    except Exception as e:
            print(f"Port error: {e}")
        
    try:
        ports[1] = serial.Serial('/dev/cu.usbserial-1', 921600)  
    except Exception as e:
            print(f"Port error: {e}")
    
    try:
        ports[2] = serial.Serial('/dev/cu.usbserial-3', 921600)  
    except Exception as e:
            print(f"Port error: {e}")
    
    try:
        ports[3] = serial.Serial('/dev/cu.usbserial-4', 921600)  
    except Exception as e:
            print(f"Port error: {e}")
    
    try:
        ports[4] = serial.Serial('/dev/cu.usbserial-2', 921600)  
    except Exception as e:
            print(f"Port error: {e}")


# Open text files for appending

# Unused but can be used to check what's being published to server for frontend
gui_log_file = open('gui_serial.txt', 'w')

# Mapped to boards
raw_log_file = open('raw_serial_log.txt', 'a')
raw_log_file_2 = open('raw_serial_log_2.txt', 'a')
raw_log_file_3 = open('raw_serial_log_3.txt', 'a')
raw_log_file_4 = open('raw_serial_log_4.txt', 'a')
raw_log_file_5 = open('raw_serial_log_5.txt', 'a')


# Switch Case Dicts
board_to_log_file_dict = {"Board 1": raw_log_file, "Board 2": raw_log_file_2, "Board 3": raw_log_file_3, "Board 4": raw_log_file_4, "Board 5": raw_log_file_5}
b1015_to_topic_dict = {"Board 1": b1_mqtt_log_1015, "Board 2": b2_mqtt_log_1015, "Board 3": b3_mqtt_log_1015, "Board 4": b4_mqtt_log_1015, "Board 5": b5_mqtt_log_1015}
b1115_to_topic_dict = {"Board 1": b1_mqtt_log_1115, "Board 2": b2_mqtt_log_1115, "Board 3": b3_mqtt_log_1115, "Board 4": b4_mqtt_log_1115, "Board 5": b5_mqtt_log_1115}


# Fast changing lists storing data from Board DAQ
"""

Data at each index
0 - Formatted data for local file logging 
1 - Sensor Type ('ADS1015' / 'ADS1115')
2 - Publish JSON (To publish to mqtt for frontend of GUI)
3 - Board ID ('Board 1', 'Board 2', 'Board 3', 'Board 4', 'Board 5')

"""
datatopass = ["", "", "", ""]
datatopass2 = ["", "", "", ""]
datatopass3 = ["", "", "", ""]
datatopass4 = ["", "", "", ""]
datatopass5 = ["", "", "", ""]
data_lock = threading.Lock()



def on_connect(client, userdata, flags, rc):
    print("Connected to MQTT broker with result code "+str(rc))
    client.subscribe(mqtt_topic_serial)
    client.subscribe(mqtt_switch_states_update)
    

def on_message(client, userdata, message):
    print(f"Received message on topic '{message.topic}': {message.payload.decode('utf-8')}")
    command = message.payload.decode('utf-8')
    if (message.topic == "switch_states_update"):
        print(command)
        ports[0].read_all()  # Read one line (you can also use ser.read() for binary data)
        ports[0].write(command.encode())
        print("Command sent:", command)


def read_serial_and_log_high_freq(port, data_list):
    while True:
        try:
            # Read a line of data from the Serial Monitor
            data = port.readline().decode('utf-8').strip()
            data_dict = json.loads(data)

            data_list[3] = data_dict['BoardID']
            file_to_write = board_to_log_file_dict[data_dict['BoardID']]
            
            #print(data_dict)
            # Log the data to a text file
            
            data_formatted = (
                str(datetime.now())[11:] 
                + " "
                + str(data_dict['BoardID'])
                + "  "
                + str(data_dict['SensorType'])
                + "  ")
            for i in range(len(data_dict['Sensors'])):
                data_formatted += str(data_dict['Sensors'][i]) + "  "
            
            data_formatted += "\n"
 
            #print (data_formatted)
            #print (publish_json)
            #print(data_dict)
            #print (datatopass)                
            
            with data_lock:
                data_list[0] = data_formatted
                data_list[1] = data_dict['SensorType']
                publish_json = (
                "{\"time\": \"" + str(datetime.now())[11:22] 
                + "\","
                + "\"sensor_readings\": "
                )
                converted_values = []
                for i in range(len(data_dict['Sensors'])):
                    if data_list[1] == 'ADS1015':
                        converted_values.append(round(data_dict['Sensors'][i] * cf_1015, 2))
                    else:
                        converted_values.append(round(data_dict['Sensors'][i] * cf_1115, 2))
            
                publish_json += str(converted_values)
                publish_json += '}'
                data_list[2] = publish_json

                file_to_write.write(data_formatted)
                file_to_write.flush()  # Flush the buffer to ensure data is written immediately

        except Exception as e:
            print(f"Serial read error: {e}")

    #, {data}")


def publish_data(data_list):
    while True:
        time.sleep(0.05)
        
        # Publish data to correct topic based on which board and sensor
        board_topic_1015 = b1015_to_topic_dict[data_list[3]]
        board_topic_1115 = b1115_to_topic_dict[data_list[3]]

        with data_lock:
            if data_list[1] == 'ADS1015':
                client.publish(board_topic_1015, data_list[2])
                #print(data_list)
            if data_list[1] == 'ADS1115':
                client.publish(board_topic_1115, data_list[2])
                #print(data_list)            
          

def main():
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(mqtt_broker_address, 1883, 60)
    client.loop_start()

    open_serial_ports()


    t1 = multiprocessing.Process(target=read_serial_and_log_high_freq, args=(ports[0], datatopass))
    t2 = multiprocessing.Process(target=read_serial_and_log_high_freq, args=(ports[1], datatopass2))
    t3 = multiprocessing.Process(target=read_serial_and_log_high_freq, args=(ports[2], datatopass3))
    t4 = multiprocessing.Process(target=read_serial_and_log_high_freq, args=(ports[3], datatopass4))
    t5 = multiprocessing.Process(target=read_serial_and_log_high_freq, args=(ports[4], datatopass5))


    t6 = multiprocessing.Process(target=publish_data, args=(datatopass))
    t7 = multiprocessing.Process(target=publish_data, args=(datatopass2))
    t8 = multiprocessing.Process(target=publish_data, args=(datatopass3))
    t9 = multiprocessing.Process(target=publish_data, args=(datatopass4))
    t10 = multiprocessing.Process(target=publish_data, args=(datatopass5))


    if (ports[0]):
        t1.start()
        t6.start()

    if (ports[1]):
        t2.start()
        t7.start()

    if (ports[2]):
        t3.start()
        t8.start()
    
    if (ports[3]):
        t4.start()
        t9.start()
    
    if (ports[4]):
        t5.start()
        t10.start()
    

if __name__ == '__main__':
    main()
