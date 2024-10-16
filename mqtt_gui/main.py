import paho.mqtt.client as mqtt
import serial
import serial.tools.list_ports
import time
import json
from datetime import datetime
import threading
import numpy as np
import platform
from collections import defaultdict
import requests  # For making HTTP requests to fileserver.py
import yaml
import subprocess
import signal
import sys

# Define the range of boards
SWITCH_BOARD_RANGE = range(4, 7) #DRIVERS
ANAL_BOARD_RANGE = range(1,4) #DATA

# IGNITION FLAG
ignition_in_progress = False

# MQTT configuration
#mqtt_broker_address = "169.254.32.191"
mqtt_broker_address = "localhost"
mqtt_topic_serial = "serial_data"


# Dynamically generate MQTT topics for switch states (for boards 4, 5, 6)
mqtt_switch_states_update = {}
mqtt_switch_states_status = {}

for board_num in SWITCH_BOARD_RANGE:  # Boards 4, 5, and 6
    mqtt_switch_states_update[board_num] = f"switch_states_update_{board_num}"
    mqtt_switch_states_status[board_num] = f"switch_states_status_{board_num}"


# URL of the hosted fileserver
fileserver_url = "http://localhost:8000/files"

# Function to terminate the fileserver subprocess
def stop_fileserver(fileserver_process):
    print("Stopping Fileserver")
    fileserver_process.terminate()
    fileserver_process.wait()

    # Function to fetch file data from the file server
def fetch_file_data(filename):
    try:
        response = requests.get(f"{fileserver_url}/{filename}")
        response.raise_for_status()  # Raise an error if the request failed
        return response.json()  # Assuming the file is JSON formatted
    except requests.RequestException as e:
        print(f"Error fetching {filename}: {e}")
        return None

# Function to start the fileserver as a subprocess
def start_fileserver():
    print("Starting Fileserver")
    return subprocess.Popen([sys.executable, 'fileserver.py'])

# Start the fileserver in a separate process
fileserver_process = start_fileserver()

# Fetch conversion factor config from file server
conv_configs = fetch_file_data('conversion_factor_config.json')

if conv_configs is None:
    print("Failed to load conversion factor config. Exiting setup.")
    exit(1)  # Exit if the file could not be fetched

# Dynamically generate conversion factors and add factors from JSON config
board_id_to_conv_factor = {}
board_id_to_conv_offset = {}

for board_num in range(1, 7):  # Boards from B1 to B6
    board_key = f"B{board_num}"
    
    # Extract factors and offsets from the JSON config for each sensor type
    board_id_to_conv_factor[str(board_num)] = [
        conv_configs[f'{board_key}_Conv_Factor_ADS1015'],
        conv_configs[f'{board_key}_Conv_Factor_ADS1115'],
        conv_configs[f'{board_key}_Thermocouple']
    ]
    
    board_id_to_conv_offset[str(board_num)] = [
        conv_configs[f'{board_key}_1015_ADD'],
        conv_configs[f'{board_key}_1115_ADD'],
        conv_configs[f'{board_key}_Thermocouple_ADD']
    ]

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1,"propdaq")

# Port Management

ports = [None] * 5

def open_serial_ports():
    system = platform.system()  # Detect the OS
    available_ports = list(serial.tools.list_ports.comports())  # Get a list of available serial ports
    port_list = []

    # Define the possible port names based on the system
    if system == "Darwin":  # macOS
        port_list = ['/dev/cu.usbserial-0001', '/dev/cu.usbserial-3', '/dev/cu.usbserial-4', '/dev/cu.usbmodem56292564361', '/dev/cu.usbmodem56292564362']  # Example ports
    elif system == "Windows":
        port_list = ['COM6', 'COM5', 'COM4']  # Example ports
    elif system == "Linux":
        port_list = ['/dev/ttyUSB0', '/dev/ttyUSB1', '/dev/ttyUSB2']  # Example ports

    # Try to open up to 5 ports
    for i, port_info in enumerate(port_list):
        if i >= 5:
            break  # Limit to 5 ports
        try:
            for available_port in available_ports:
                if available_port.device == port_info:
                    ports[i] = serial.Serial(port_info, 921600)
                    print(f"Opened port: {port_info}")
                    break  # Stop checking after opening the port
        except Exception as e:
            print(f"Port error on {port_info}: {e}")

    if all(port is None for port in ports):
        print("No ports were opened.")



# Unused but can be used to check what's being published to server for frontend
gui_log_file = open('gui_serial.txt', 'w')

# Mapped to boards
raw_log_file = open('raw_serial_log.txt', 'a')
raw_log_file_2 = open('raw_serial_log_2.txt', 'a')
raw_log_file_3 = open('raw_serial_log_3.txt', 'a')
raw_log_file_4 = open('raw_serial_log_4.txt', 'a')
raw_log_file_5 = open('raw_serial_log_5.txt', 'a')
raw_log_file_6 = open('raw_serial_log_6.txt', 'a')


# Switch Case Dicts
board_to_log_file_dict = {"Board 1": raw_log_file, "Board 2": raw_log_file_2, "Board 3": raw_log_file_3, "Board 4": raw_log_file_4, "Board 5": raw_log_file_5, "Board 6": raw_log_file_6}
b_to_solenoid_status_topic_dict = {"Board 4": 'switch_states_status_4', "Board 5": 'switch_states_status_5', "Board 6": 'switch_states_status_6'}

number_to_sensor_type = {"1": "ADS 1015", "2": "ADS 1115", "3": "TC", "4": "1116"}
number_to_sensor_type_publish = {"1": "1015", "2": "1115", "3": "TC", "4": "1116"}

bit_to_V_factor = {"1": 2048, "2": 32768, "3": 1}

# AUTO-IGNITION TOPIC
ignition_topic = "AUTO"    # expects message "IGNITE" when pressed




# Fast changing lists storing data from Board DAQ
"""

Data at each index
0 - Formatted data for local file logging 
1 - Sensor Type ('ADS1015' / 'ADS1115')
2 - Publish JSON (To publish to mqtt for frontend of GUI)
3 - Board ID ('Board 1', 'Board 2', 'Board 3', 'Board 4', 'Board 5')

"""

data_lock = threading.Lock()


def on_connect(client, userdata, flags, rc):
    print("Connected to MQTT broker with result code " + str(rc))
    for board_num in SWITCH_BOARD_RANGE:  # Boards 4, 5, and 6
        client.subscribe(mqtt_switch_states_update[board_num])
        client.subscribe(mqtt_switch_states_status[board_num])
    client.subscribe(ignition_topic)
    


def on_message(client, userdata, message):
    global ignition_in_progress
    # MAP SOLENOID BOARDS HERE
    #print(f"Received message on topic '{message.topic}': {message.payload.decode('utf-8')}")
    message_payload = message.payload.decode('utf-8')
    
    if (message.topic == "AUTO"):
        if message_payload == "IGNITE":
            print("AUTO IGNITE...")
            t3 = threading.Thread(target=auto_ignite)
            t3.start()
        
        if message_payload == "ABORT":
            ignition_in_progress = False
            print("ABORTING...")
            t4 = threading.Thread(target=abort_process)
            t4.start()
            
    
    for board_num in SWITCH_BOARD_RANGE:  # Boards 4, 5, and 6
        if message.topic == mqtt_switch_states_update[board_num]:
            command = f"{board_num}{message_payload}\n"
            send_command = command.encode('utf-8')
            for port in ports:
                if port:
                    port.write(send_command)
                    port.flush()
                    print(send_command)



def extract_board_id(board_id_hex):
    try:
        # Convert the hex string to an integer, then perform boardID // 16 (equivalent to /10 in hex)
        return int(board_id_hex, 16) // 16
    except ValueError as e:
        #print(f"ValueError: Invalid hex value '{board_id_hex}'. Skipping this line.")
        return None
    except TypeError as e:
        return None

def process_data(data):
    try:
        # Extract sensor type and sensors
        sensor_type = int(data["SensorType"])
        sensors = data["Sensors"]
    except ValueError as e:
        #print(f"KeyError: Missing key {e} in data. Skipping this line.")
        return None, None
    except KeyError as e:
        #print(f"KeyError: Missing key {e} in data. Skipping this line.")
        return None, None
    except TypeError as e:
        return None, None
    return sensor_type, sensors

# Dictionary to store counts for each board's sensor types
boards_data = defaultdict(lambda: defaultdict(int))

# Store the last time we calculated frequencies
interval = 2.0  # Time interval in seconds to calculate frequency

class Board_DAQ():

    def __init__(self, port_index):
        self.port_index = port_index
        self.data_lock = threading.Lock()
        self.publish_dict = {}
        self.last_time = time.time()
    
    def publish_data(self):
        while True:
            time.sleep(0.05)
            for topic in self.publish_dict:
                client.publish(topic,self.publish_dict[topic])

    def read_serial_and_log_high_freq(self):
        while True:
            try:
                data = ports[self.port_index].readline().decode('utf-8').strip()
                data_dict = json.loads(data)

                # print(data_dict)
                # Extract the board ID, skip if invalid hex or key is missing
                try:
                    extractdata = data_dict.get("BoardID", "")
                    board_id = extract_board_id(extractdata)
                    if board_id is None:
                        continue

                    # Process sensor data, skip if there's an error
                    sensor_type, sensors = process_data(data_dict)
                    if sensor_type is None or sensors is None:
                        continue

                    # Update count of sensor types for this specific board ID
                    boards_data[board_id][sensor_type] += 1
                    
                    # Check if interval has passed
                    current_time = time.time()
                    if True: #printing frequencies
                        if current_time - self.last_time >= interval:
                            print(f"\nData summary for the past {interval} second(s):")

                            # Print data for each board ID
                            for board, sensor_data in boards_data.items():
                                print(f"\nBoard ID: {board}")
                                print("Sensor Type Frequencies (per second):")
                                for stype, count in sensor_data.items():
                                    print(f"  Sensor Type {stype}: {count / interval} Hz")
                            
                            # Reset counts and timer
                            boards_data.clear()
                            self.last_time = current_time
                except AttributeError as e:
                    print(e)
                    continue

                # processing starts:
                Board_ID = data_dict['BoardID'][0]
                if data_dict['BoardID'] == 'Board 6':
                    Board_ID = '6'
                Board_ID_worded = "Board " +  Board_ID
                sensor_type = data_dict['SensorType'] 

                file_to_write = board_to_log_file_dict[Board_ID_worded]                
                
                data_formatted = (
                    str(datetime.now())[11:] 
                    + " "
                    + str(Board_ID_worded)
                    + "  "
                    + str(number_to_sensor_type[sensor_type])
                    + "  ")

                # SOLENOID BOARDS 
                if int(Board_ID) in SWITCH_BOARD_RANGE:
                    raw_byte_array = data_dict['Sensors']
                    publish_array = raw_byte_array[0:5]
                    publish_json_dict = {"time": str(datetime.now())[11:22], "sensor_readings": publish_array}
                    publish_json = json.dumps(publish_json_dict)
                    #print(Board_ID + " " + publish_json)

                    if int(Board_ID) in SWITCH_BOARD_RANGE:  # Check if the board is in the defined switch range
                        self.publish_dict[mqtt_switch_states_status[int(Board_ID)]] = publish_json

                    
                    file_to_write.write(publish_json + "\n")
                    file_to_write.flush()  

                    
                # DAQ BOARDS
                elif (int(Board_ID) in ANAL_BOARD_RANGE):        
                    raw_byte_array = data_dict['Sensors']
                    converted_array = np.array([],dtype=np.uint16)

                    
                    if (data_dict['SensorType']  == "3"):
                        for i in range(0, len(raw_byte_array), 2):
                            #print(raw_byte_array[i],raw_byte_array[i+1])
                            value_to_append = np.uint16(0)
                            value_to_append += np.uint16(raw_byte_array[i])*np.uint16(256) + np.uint16(raw_byte_array[i+1])
                            #value_to_append = float(np.array(np.uint16(value_to_append)).astype(np.float16))
                            converted_array = np.append(converted_array,value_to_append)
                        converted_array = converted_array.view(np.float16)
                        for i in range(len(converted_array)):     
                            if (np.isnan(converted_array[i])):
                                converted_array[i] = 0
                    else:
                        for i in range(0, len(raw_byte_array), 2):
                            value_to_append = np.uint16(0)
                            value_to_append += np.uint16(raw_byte_array[i])*np.uint16(256) + np.uint16(raw_byte_array[i+1])
                            #value_to_append += raw_byte_array[i]*256 + raw_byte_array[i+1]
                            #value_to_append = float(np.array(np.uint16(value_to_append)).astype(np.int16))
                            converted_array = np.append(converted_array,value_to_append)
                        converted_array = converted_array.view(np.int16)
                        for i in range(len(converted_array)):     
                            converted_array[i] *= 6.144/5 #ADS PGA voltage 6.144V max range / 0-5V operational range

                    converted_array = converted_array.tolist()
                    #print(converted_array)
                    data_formatted += "V: "
                    for i in range(len(converted_array)):
                        data_formatted += str(converted_array[i]) + " "

                    #print(converted_array)
                    conversion_factor_V = bit_to_V_factor[sensor_type]
                    for i in range(len(converted_array)):
                        converted_array[i] /= conversion_factor_V
        
                    #CONFIG CONVERSIONS
                    final_values = []

                    int_sensor_type = int(sensor_type)
                    conv_factor = board_id_to_conv_factor[Board_ID][int_sensor_type - 1]
                    offset = board_id_to_conv_offset[Board_ID][int_sensor_type - 1]
                    for i in range(len(converted_array)):     
                        final_values.append(round((converted_array[i] * conv_factor[i]) + offset[i], 2))

                    data_formatted += "Converted: "
                    for i in range(len(final_values)):
                        data_formatted += str(final_values[i]) + " "

                    data_formatted += "\n"

                    file_to_write.write(data_formatted)
                    file_to_write.flush()  

                    
                    # PUBLISH 

                    publish_json_dict = {"time": str(datetime.now())[11:22], "sensor_readings": final_values}
                    publish_json = json.dumps(publish_json_dict)

                    topic = "b" + Board_ID + "_log_data_" + number_to_sensor_type_publish[sensor_type]

                    self.publish_dict[topic] = publish_json
            
            except Exception as e:
                print(f"Serial read error: {e}")
                # print(data_dict['BoardID'])
                # print(Board_ID)
                # , {data}")
        
                    
    def solenoid_write(self, message):
        ports[self.port_index].write(message)

class TaskManager:
    def __init__(self, sequence, ports, port_index=0, force = False):
        self.tasks = {}
        self.ports = ports
        self.port_index = port_index
        self.load_tasks(sequence)
        self.force = force
        self.threads = []  # To collect threads for repeat tasks

    def load_tasks(self, sequence):
        for step in sequence:
            task = Task(
                name=step['name'],
                command=step['command'],
                delay=step['delay'],
                repeat=step.get('repeat', False),
                port=self.ports,
                port_index=self.port_index,
                next_id=step.get('next_id', []),
                task_id=step['id']
            )
            self.tasks[step['id']] = task

    def run_task_by_id(self, task_id):
        if task_id not in self.tasks:
            return
        
        task = self.tasks[task_id]
        task.start_time = time.time()

        task.execute(self.force)
        while not task.is_completed():
            time.sleep(0.1)
            task.update(self.force)

        for next_task_id in task.next_id:
            self.run_task_by_id(next_task_id)

    def run(self, starting_id = 1):
        self.run_task_by_id(starting_id)

    def join_all_threads(self):
        for thread in self.threads:
            thread.join()
        print("All background threads have been joined.")


class Task:
    def __init__(self, name, command, delay, repeat, port, port_index=0, next_id=None, task_id=None):
        self.name = name
        self.command = command + "\n"
        self.delay = delay
        self.repeat = repeat
        self.port = port
        self.port_index = port_index
        self.start_time = time.time()
        self.completed = False
        self.next_id = next_id if next_id else []
        self.task_id = task_id

    def execute(self, force = False):
        send_command = self.command.encode('utf-8')
        if ignition_in_progress or force:
            for port in ports:
                if port:
                    port.write(send_command)
                    port.flush()
            print(f"Executed {self.name}: {send_command}")
            if time.time() - self.start_time >= self.delay:
                print(f"Completed {self.name}: {send_command}")
                self.completed = True
        else:
            print(f"Aborted {self.name}: {send_command}")
            self.completed = True
    
    def update(self, force = False): #execute but silent
        send_command = self.command.encode('utf-8')
        if ignition_in_progress or force:
            '''
            for port in ports:
                if port:
                    port.write(send_command)
                    port.flush()
                time.sleep(0.01) #command response
            print(f"Executed {self.name}: {send_command}") 
            '''
            if time.time() - self.start_time >= self.delay:
                print(f"Completed {self.name}: {send_command}")
                self.completed = True
        else:
            print(f"Aborted {self.name}: {send_command}")
            self.completed = True
        
               
    def is_completed(self):
        return self.completed

# Helper function to load YAML configuration
def load_yaml(file_path):
    with open(file_path, 'r') as file:
        return yaml.safe_load(file)

automation_config = load_yaml('automation_config.yaml')
automanager = TaskManager(automation_config['automation_sequence'], ports)

abort_config = load_yaml('abort_config.yaml')
abort_manager = TaskManager(abort_config['abort_sequence'], ports, force=True)

def auto_ignite():
    global ignition_in_progress
    automanager = TaskManager(automation_config['automation_sequence'], ports)
    ignition_in_progress = True
    automanager.run()

# Abort process that also runs abort tasks
def abort_process():
    abort_manager = TaskManager(abort_config['abort_sequence'], ports, force=True)
    global ignition_in_progress
    ignition_in_progress = False
    print("Abort triggered! Completing current tasks and running abort sequence.")
    automanager.join_all_threads()

    abort_manager.run()


def main():


    try:
        client.on_connect = on_connect
        client.on_message = on_message

        client.connect(mqtt_broker_address, 1884, 60)
        client.loop_start()

        open_serial_ports()

        t1 = [None] * 5
        t2 = [None] * 5

        for i, port in enumerate(ports):
            if port:
                port_ = Board_DAQ(i)
                t1[i] = threading.Thread(target=port_.read_serial_and_log_high_freq)
                t2[i] = threading.Thread(target=port_.publish_data)
                t1[i].start()
                t2[i].start()
         # Wait for all threads to finish using join()
        for i in range(len(ports)):
            if t1[i]:
                t1[i].join()  # Ensure read_serial_and_log_high_freq finishes
            if t2[i]:
                t2[i].join()  # Ensure publish_data finishes

    finally:
        # Ensure the fileserver process is stopped when exiting
        stop_fileserver(fileserver_process)
        


if __name__ == '__main__':
    main()
