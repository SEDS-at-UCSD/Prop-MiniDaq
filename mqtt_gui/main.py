import paho.mqtt.client as mqtt
import serial
import serial.tools.list_ports
import time
import json
from datetime import datetime
import multiprocessing
import threading
import numpy as np
import platform
from collections import defaultdict
import requests  # For making HTTP requests to fileserver.py
import yaml
import subprocess
import signal
import sys
from fastapi import FastAPI, Request
from fastapi.middleware.cors import CORSMiddleware
from fastapi.concurrency import run_in_threadpool
import socket
import math
import random

app = FastAPI()


def update_global_time(namespace):
    """
    Function to update the global timer continuously.
    """
    while True:
        namespace.global_timer = time.time()  # Update the timer in the shared namespace
        #print(f"[Timer Process] Updated Global Timer: {namespace.global_timer}")
        time.sleep(0.01)  # Update every 10ms


"""def get_global_time(global_timer):
    
    Function to safely read the global time.
    
    with global_timer.get_lock():  # Acquire lock to safely read the timer
        return global_timer.value"""
    


# Add CORS middleware
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # React
    allow_credentials=True,
    allow_methods=["*"],  # Allow all methods (GET, POST, etc.)
    allow_headers=["*"],  # Allow all headers
)

# Define the range of boards
SWITCH_BOARD_RANGE = [4,5,8,9,10] #DRIVERS
ANAL_BOARD_RANGE = [1,2,3,7] #DATA

# Dynamically generate conversion factors and add factors from JSON config
board_id_to_ranges = {}
board_id_to_topics = {}

# Dynamically generate MQTT topics for switch states (for boards 4, 5, 6)
mqtt_switch_states_update = {}
mqtt_switch_states_status = {}

# Update function
def update_conversion_factors():
    global board_id_to_ranges, board_id_to_topics
    # Reset ranges and topics
    board_id_to_ranges = {}
    board_id_to_topics = {}

    # For each board, iterate through the data field to dynamically assign ranges and topics
    for board_num in (SWITCH_BOARD_RANGE + ANAL_BOARD_RANGE):  # Assuming boards are B1 to B6
        board_key = f"B{board_num}"
        board_id_to_ranges[str(board_num)] = {}
        board_id_to_topics[str(board_num)] = {}

        # Iterate through each sensor or solenoid in the data array
        for sensor_data in conv_configs[board_key]["data"]:
            sensor_type = sensor_data.get("sensor_type", "")
            sensors = sensor_data.get("sensors", [])
            topic = sensor_data.get("topic", "")

            if sensor_type.lower() == "solenoids":
                mqtt_switch_states_update[board_num] = f"update_{board_key.lower()}_switch_states_status"
                mqtt_switch_states_status[board_num] = f"{board_key.lower()}_switch_states_status"

            if sensor_type and sensors:
                board_id_to_ranges[str(board_num)][sensor_type] = sensors  # Store ranges for sensors
                board_id_to_topics[str(board_num)][sensor_type] = topic    # Store topics for sensors
    #print(board_id_to_ranges)
    #print("\n")
    #print(board_id_to_topics)


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

def fetch_file_data_yaml(filename):
    try:
        response = requests.get(f"{fileserver_url}/{filename}")
        response.raise_for_status()  # Raise an error if the request failed
        yaml_data = yaml.safe_load(response.text)
        return yaml_data  # Return the parsed YAML as a dictionary
    except requests.RequestException as e:
        print(f"Error fetching {filename}: {e}")
        return None

# Function to start the fileserver as a subprocess
def start_fileserver():
    print("Starting Fileserver")
    return subprocess.Popen([sys.executable, 'fileserver.py'])

# IGNITION FLAG
ignition_in_progress = False

# MQTT configuration
#mqtt_broker_address = "169.254.32.191"
mqtt_broker_address = "localhost"
mqtt_topic_serial = "serial_data"

# URL of the hosted fileserver
fileserver_url = "http://localhost:8000/files"

# Global variables for configurations
conv_configs = {}
automation_config = {}
abort_config = {}
channel_calc_config = {}

# Function to reload configuration files
def reload_config(filename):
    global conv_configs, automation_config, abort_config, channel_calc_config
    if filename == "conversion_factor_config.json":
        with open(filename, 'r') as file:
            conv_configs = fetch_file_data(filename)
            update_conversion_factors()
            print("Conversion factor config updated")

    if filename == "calculation_config.yaml":
        with open(filename, 'r') as file:
            channel_calc_dict = fetch_file_data_yaml(filename)
            channel_calc_config = channel_calc_dict
            """print(channel_calc_config)
            print("Channel calculations updated")"""
    
    else:
        print(f"Unknown config file: {filename}")

# Create a thread-safe event flag
reload_flag = threading.Event()

# Endpoint to update the configs when notified by fileserver.py
@app.post("/update_config/{filename}")
async def update_config(filename: str):
    #reload_config(filename)
    reload_flag.set()
    return {"message": f"{filename} reloaded successfully"}

def get_all_ips():
    ip_addresses = []
    # Get all IPv4 addresses for the hostname, excluding loopback addresses like 127.0.0.1
    addresses = socket.getaddrinfo(socket.gethostname(), None, socket.AF_INET)
    for addr in addresses:
        ip = addr[4][0]
        # Exclude loopback addresses (127.x.x.x range)
        if not ip.startswith("127.") and ip not in ip_addresses:
            ip_addresses.append(ip)
    return ip_addresses

@app.get("/get-server-ip")
async def get_server_ip():
    ips = get_all_ips()
    return {"ips": ips}

# Function to run FastAPI in a separate thread
def start_fastapi():
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8001)

# Start FastAPI server in a separate thread
fastapi_thread = threading.Thread(target=start_fastapi)
fastapi_thread.daemon = True
fastapi_thread.start()

# Start the fileserver in a separate process
fileserver_process = start_fileserver()
time.sleep(1)

# Fetch conversion factor config from file server
conv_configs = fetch_file_data('conversion_factor_config.json')
channel_calc_config = fetch_file_data_yaml('calculation_config.yaml')

if conv_configs is None:
    print("Failed to load conversion factor config. Exiting setup.")
    exit(1)  # Exit if the file could not be fetched

update_conversion_factors()

#client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1,"propdaq")

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
        port_list = ['COM6', 'COM5', 'COM4', 'COM3']  # Example ports
    elif system == "Linux":
        port_list = ['/dev/ttyUSB0', '/dev/ttyUSB1', '/dev/ttyUSB2']  # Example ports

    # Try to open up to 5 ports
    for i, port_info in enumerate(port_list):
        if i >= 5:
            break  # Limit to 5 ports
        try:
            for available_port in available_ports:
                if available_port.device == port_info:
                    ports[i] = port_info
                    print(f"Opened port: {port_info}")
                    break  # Stop checking after opening the port
        except Exception as e:
            print(f"Port error on {port_info}: {e}")

    if all(port is None for port in ports):
        print("No ports were opened.")



# Unused but can be used to check what's being published to server for frontend
gui_log_file = open('gui_serial.txt', 'w')

# Mapped to boards
raw_log_file = {}
for i in ( SWITCH_BOARD_RANGE + ANAL_BOARD_RANGE ): #DATA
    raw_log_file[i] = open(f'raw_serial_log_{i}.txt', 'a')


# Switch Case Dicts
#board_to_log_file_dict = {"Board 1": raw_log_file, "Board 2": raw_log_file_2, "Board 3": raw_log_file_3, "Board 4": raw_log_file_4, "Board 5": raw_log_file_5, "Board 6": raw_log_file_6}

number_to_sensor_type = {"0": "Boot", "1": "ADS1015", "2": "ADS1115", "3": "TC", "4": "ADS1256"}
number_to_sensor_type_publish = {"0": "Alive", "1": "1015", "2": "1115", "3": "TC", "4": "1115"} #TEMP FOR ADS 1256, USING ADS 1115 configs >= 16 bit, so rn show it as 1115 

bit_to_V_factor = {"1": 2048, "2": 32768, "3": 1, "4": 32768} #TEMP FOR ADS 1256, USING ADS 1115 configs full unsigned 16bit

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

    

def extract_board_id(board_id_hex):
    try:
        # Convert the hex string to an integer, then perform boardID // 16 (equivalent to /10 in hex)
        return int(board_id_hex, 16) // 16
    except ValueError as e:
        #print(f"ValueError: Invalid hex value '{board_id_hex}'. Skipping this line.")
        return None
    except TypeError as e:
        return None
    
import re

def convert_c_to_final_values(text):
    # Regular expression to find C1, C2, C3, etc.
    text = text.lower().strip()
    return re.sub(r'c(\d+)', lambda match: f'final_values[{int(match.group(1)) - 1}]', text)


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

    def __init__(self, port,  port_index, mqtt_client, port_name, namespace, automation_event, abort_event):
        self.port_index = port_index
        self.port = port
        self.data_lock = threading.Lock()
        self.publish_dict = {}
        self.last_time = time.time()
        self.mqtt_client = mqtt_client
        self.mqtt_client.on_connect = self.on_connect 
        self.mqtt_client.on_message = self.on_message
        self.port_name = port_name
        self.namespace = namespace
        # Initialize TaskManager
        self.automanager = TaskManager(automation_config['automation_sequence'], port, namespace, port_index=port_index)
        self.abort_manager = TaskManager(abort_config['abort_sequence'], port, namespace, port_index=port_index, force=True)
        self.automation_event = automation_event
        self.abort_event = abort_event

    def on_connect(self, client, userdata, flags, rc):
        print("Connected to MQTT broker with result code " + str(rc))
        #print("UHUFRHIFURHFIUHFIUHFIRFUHRIFUHRI")
        for board_num in SWITCH_BOARD_RANGE:  # Boards 4, 5, and 6
            client.subscribe(mqtt_switch_states_update[board_num])
            client.subscribe(mqtt_switch_states_status[board_num])
            client.subscribe(ignition_topic)
            #print("SUBSCRIBED")
    
    def on_message(self, client, userdata, message):
        #global ignition_in_progress
        # MAP SOLENOID BOARDS HERE
        #print(f"Received message on topic '{message.topic}': {message.payload.decode('utf-8')}")
        #print("MESSAGE")
        message_payload = message.payload.decode('utf-8')
        
        if (message.topic == "AUTO"):
            if message_payload == "IGNITE":
                print("AUTO IGNITE...")
                t3 = threading.Thread(target=self.auto_ignite)
                t3.start()
            
            if message_payload == "ABORT":
                #ignition_in_progress = False
                print("ABORTING...")
                t4 = threading.Thread(target=self.abort_process)
                t4.start()
                
        
        for board_num in SWITCH_BOARD_RANGE:  # Boards 4, 5, and 6
            if message.topic == mqtt_switch_states_update[board_num]:
                command = f"{board_num}{message_payload}\n"
                send_command = command.encode('utf-8')
                self.port.write(send_command)
                self.port.flush()
                print(send_command)
    
    def publish_data(self):
        while True:
            #print(f"enter {self.port_index}")
            time.sleep(0.05)
            try:
                #reload_config("calculation_config.yaml")
                for topic in self.publish_dict:
                    self.mqtt_client.publish(topic,self.publish_dict[topic])
            except Exception as e:
                print(e)

    def read_serial_and_log_high_freq(self):
        decode_fail_count = 0  # Track consecutive decoding failures
        max_decode_failures = 10  # Threshold for resetting the serial port on json failures
        while True:
            if reload_flag.is_set():
                print("Reload flag set. Reloading configurations...")
                
                # Perform the reload operation
                reload_config("conversion_factor_config.json")
                reload_config("calculation_config.yaml")
                
                # Clear the flag after reloading
                reload_flag.clear()
            try:
                data = self.port.readline().decode('utf-8').strip()
                #print(data)
                data_dict = json.loads(data)
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
                            # print(f"\nData summary for the past {interval} second(s):")

                            # Print data for each board ID
                            """for board, sensor_data in boards_data.items():
                                print(f"\nBoard ID: {board}")
                                print("Sensor Type Frequencies (per second):")
                                for stype, count in sensor_data.items():
                                    print(f"  Sensor Type {stype}: {count / interval} Hz")"""
                            
                            # Reset counts and timer
                            boards_data.clear()
                            self.last_time = current_time
                except AttributeError as e:
                    print(e)
                    continue

                # processing starts:
                Board_ID = str(board_id)
                Board_ID_worded = "Board " +  Board_ID
                sensor_type = data_dict['SensorType'] 

                file_to_write = raw_log_file[board_id] #board_to_log_file_dict[Board_ID_worded]                
                
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
                        #print(Board_ID)
                        board_key = f"B{Board_ID}"
                        #print(board_key)
                        #print(conv_configs[board_key]["data"])

                        # Since conv_configs[board_key]["data"] is a list, we need to loop through the items
                        for sensor_data in conv_configs[board_key]["data"]:
                            # Find the sensor type and topic
                            topic = sensor_data.get("topic", "")
                            if topic:
                               #print(topic)
                                self.publish_dict[topic] = publish_json

                    
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
                    elif (data_dict['SensorType']  == "4"):
                        for i in range(0, len(raw_byte_array), 2):
                            value_to_append = np.uint16(0)
                            value_to_append += np.uint16(raw_byte_array[i])*np.uint16(256) + np.uint16(raw_byte_array[i+1])
                            #value_to_append += raw_byte_array[i]*256 + raw_byte_array[i+1]
                            #value_to_append = float(np.array(np.uint16(value_to_append)).astype(np.int16))
                            converted_array = np.append(converted_array,value_to_append)
                        converted_array = converted_array.view(np.int16) #UINT 16
                        for i in range(len(converted_array)):     
                            converted_array[i] *= 1.00 #ADS PGA voltage 5.0 max range / 0-5V operational range 
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
            
                    converted_array = converted_array.tolist() #here inherent max range 32768 = 5V
                    #print(converted_array)
                    data_formatted += "V: "
                    for i in range(len(converted_array)):
                        data_formatted += str(converted_array[i]) + " "

                    #print(converted_array)
                    conversion_factor_V = bit_to_V_factor[sensor_type]
                    for i in range(len(converted_array)):
                        converted_array[i] /= conversion_factor_V

                    #print(converted_array)
                    
                    # CONFIG CONVERSIONS
                    final_values = []
                    int_sensor_type = int(sensor_type)

                    # Example block where sensor data processing happens
                    for i in range(len(converted_array)):
                        if (data_dict['SensorType']  == "3"):
                            converted_value = converted_array[i] 
                        else:
                            # Fetch min/max values dynamically from config
                            if int_sensor_type:  # ADS1015
                                sensor_range = board_id_to_ranges[str(int(board_id))][number_to_sensor_type[sensor_type]][i]
                            else:
                                continue  # Skip if neither ADS1015 nor ADS1115

                            # Perform dynamic conversion using the min/max values and readings
                            min_reading = float(sensor_range["min_reading"])
                            max_reading = float(sensor_range["max_reading"])
                            min_value = float(sensor_range["min_value"])
                            max_value = float(sensor_range["max_value"])

                            # Apply conversion formula for 5.0V
                            converted_value = ((5.0*converted_array[i] - min_reading) / (max_reading - min_reading)) * (max_value - min_value) + min_value
                        final_values.append(round(converted_value, 2))  # Append converted value to final list

                    
                    ## Inter Channel Calc
                    calculations = channel_calc_config[f'B{Board_ID}']["calculations"]
                    #print(calculations)
                    if calculations:
                        #print('entered')
                        interchannel_calc = []
                        for calc in calculations:
                            val_to_calc = float(eval(convert_c_to_final_values(calc)))
                            interchannel_calc.append(val_to_calc)

                        publish_calc_dict = {"time": str(datetime.now())[11:22], "sensor_readings": interchannel_calc}
                        publish_calc_json = json.dumps(publish_calc_dict)
                        calc_topic = f"b{Board_ID}_calc"
                        self.publish_dict[calc_topic] = publish_calc_json


                    data_formatted += "Converted: "
                    for i in range(len(final_values)):
                        data_formatted += str(final_values[i]) + " "

                    data_formatted += "\n"

                    file_to_write.write(data_formatted)
                    file_to_write.flush()  

                    
                    # PUBLISH 
                    publish_json_dict = {"time": str(datetime.now())[11:22], "sensor_readings": final_values}
                    publish_json = json.dumps(publish_json_dict)
                    board_key = f"B{int(board_id)}"

                    # Get the relevant sensor type for this board (e.g., ADS1015, ADS1115)
                    for sensor_config in conv_configs[board_key]["data"]:
                        sensor_config_type = sensor_config.get("sensor_type")
                        
                        # Check the sensor type (it could be ADS1015, ADS1115, or others)
                        #print(sensor_config_type,number_to_sensor_type[sensor_type])
                        if sensor_config_type == number_to_sensor_type[sensor_type]:
                            topic = sensor_config["topic"]  
                            self.publish_dict[topic] = publish_json
                            #print(f"Published data to topic {topic}")
            
            except serial.SerialException as e:
                if "Device not configured" in str(e):
                    print(f"Serial read error: {e}. Retrying in 0.5 seconds...")
                    time.sleep(0.5)
                else:
                    print(f"Serial read error: {e}")
                # Close the problematic port
                try:
                    self.port.close()
                    print(f"Closed port {self.port_name} due to an error.")
                except Exception as close_error:
                    print(f"Error while closing port {self.port_name}: {close_error}")

                # Reopen the port
                try:
                    port_info = self.port  # Retrieve the port name
                    self.port = serial.Serial(port_info, 921600)
                    print(f"Reopened port {port_info}.")
                except Exception as reopen_error:
                    print(f"Failed to reopen port {port_info}: {reopen_error}. Retrying in 1 second...")
                    time.sleep(1)  # Wait before retrying
            except json.JSONDecodeError as e:
                print(f"JSON decode error: {e}. Possible buffer corruption.")
                decode_fail_count += 1
                
                # Reset the port if decoding failures persist
                if decode_fail_count >= max_decode_failures:
                    decode_fail_count = 0
                    self.port.reset_input_buffer()
                    # Close the problematic port
                    try:
                        self.port.close()
                        print(f"Closed port {self.port_name} due to an error.")
                    except Exception as close_error:
                        print(f"Error while closing port {self.port_name}: {close_error}")

                    # Reopen the port
                    try:
                        port_name = self.port_name # Retrieve the port name
                        self.port = serial.Serial(port_name, 921600)
                        print(f"Reopened port {port_name}.")
                    except Exception as reopen_error:
                        print(f"Failed to reopen port {port_name}: {reopen_error}. Retrying in 1 second...")
                        time.sleep(1)  # Wait before retrying
                    
            except Exception as e:
                print(f"Serial read error: {e}")
                print(f"Exception type: {type(e)}")
                #print(extractdata)
        

    def auto_ignite(self):
        #global ignition_in_progress
        #automanager = TaskManager(automation_config['automation_sequence'], global_timer=global_timer)
        #ignition_in_progress = True
        if not self.automation_event.is_set():
            self.automation_event.set()
            print("Starting automation sequence")
            self.automanager.stop_event.clear()
            self.automanager.run()
        else:
            print("Automation already running in another process. Skipping...")

    # Abort process that also runs abort tasks
    def abort_process(self):
        if not self.abort_event.is_set():
            self.abort_event.set()
            self.automanager.halt_all_threads()
            self.automanager.reset_tasks()
            print("Abort triggered! Completing current tasks and running abort sequence.")
            self.abort_manager.run()
        else:
            print("Abort already initiated in another process. Skipping...")



class TaskManager:
    def __init__(self, sequence, port, namespace, port_index=0, force = False):
        self.tasks = {}
        self.port = port
        self.port_index = port_index
        self.force = force
        self.threads = []  # To collect threads for repeat tasks
        self.namespace = namespace
        self.load_tasks(sequence)
        self.tasklist = []
        self.sequence = sequence
        self.stop_event = threading.Event()
        

    def load_tasks(self, sequence):
        for step in sequence:
            task = Task(
                name=step['name'],
                command=step['command'],
                delay=step['delay'],
                repeat=step.get('repeat', False),
                port=self.port,
                port_index=self.port_index,
                next_id=step.get('next_id', []),
                task_id=step['id'],
                namespace=self.namespace
            )
            self.tasks[step['id']] = task

    def reset_tasks(self):
        self.tasks = {}
        self.load_tasks(self.sequence)


    def run_task_by_id(self, task_id):
        if not self.stop_event.is_set():
            self.tasklist.append(task_id)

            if task_id not in self.tasks:
                return
            
            task = self.tasks[task_id]
            task.start_time = self.get_global_time()

            task.execute(self.force)
            while not task.is_completed():
                time.sleep(0.1)
                task.update(self.force)

            for next_task_id in task.next_id:
                self.run_task_by_id(next_task_id)
            self.tasklist.remove(task_id)
            if not self.tasklist:
                self.reset_tasks()

    def run(self, starting_id = 1):
        self.run_task_by_id(starting_id)

    def halt_all_threads(self):
        self.stop_event.set()
        for thread in self.threads:
            thread.join()
        print("All background threads have been joined.")
    
    def get_global_time(self):
        timer_value = self.namespace.global_timer
        print(f"[Task] Timer Value: {timer_value}")
        return timer_value


class Task:
    def __init__(self, name, command, delay, repeat, port, namespace, port_index=0, next_id=None, task_id=None,):
        self.name = name
        self.command = command + "\n"
        self.delay = delay
        self.repeat = repeat
        self.port = port
        self.port_index = port_index
        self.namespace = namespace
        self.start_time = self.get_global_time()
        self.completed = False
        self.next_id = next_id if next_id else []
        self.task_id = task_id
        self.ignition_in_progress = True

    def execute(self, force = False):
        send_command = self.command.encode('utf-8')
        if self.ignition_in_progress or force:
            if self.port:
                self.port.write(send_command)
                self.port.flush()
            #print(f"Executed {self.name}: {send_command}")
            #print(f"Read Global Timer: {self.get_global_time()}")  # Ensure the timer is being read correctly
            if self.get_global_time() - self.start_time >= self.delay:
                print(f"Completed {self.name}: {send_command}")
                self.completed = True
        else:
            print(f"Aborted {self.name}: {send_command}")
            self.completed = True
    
    def update(self, force = False): #execute but silent
        send_command = self.command.encode('utf-8')
        if self.ignition_in_progress or force:
            '''
            for port in ports:
                if port:
                    port.write(send_command)
                    port.flush()
                time.sleep(0.01) #command response
            print(f"Executed {self.name}: {send_command}") 
            '''
            #print(self.get_global_time(global_timer))
            #print(self.start_time)
            ###print(f"Read Global Timer: {self.get_global_time()}")  # Ensure the timer is being read correctly
            if self.get_global_time() - self.start_time >= self.delay:
                print(f"Completed {self.name}: {send_command}")
                self.completed = True
        else:
            print(f"Aborted {self.name}: {send_command}")
            self.completed = True
        
               
    def is_completed(self):
        return self.completed
    
    def get_elapsed_time(self):
        with self.global_timer.get_lock():
            return self.namespace.global_timer - self.start_time

    def get_global_time(self):
        #print(self.namespace.global_timer)
        return self.namespace.global_timer

# Helper function to load YAML configuration
def load_yaml(file_path):
    with open(file_path, 'r') as file:
        return yaml.safe_load(file)

automation_config = load_yaml('automation_config.yaml')
#automanager = TaskManager(automation_config['automation_sequence'], ports, global_timer)

abort_config = load_yaml('abort_config.yaml')
#abort_manager = TaskManager(abort_config['abort_sequence'], ports, global_timer, force=True)




def process_for_port(port_name, port_index, namespace, automation_event, abort_event):
    """
    Function to handle a specific serial port in a separate process.
    """
    try:
        # Open the serial port in the child process
        print(f"PORT INDEX: {port_index}")
        try:
            port = serial.Serial(port_name, 921600, timeout=0.1)
            print(f"Opened port {port_name} in process.")
        except Exception as e:
            print("Retrying port {port_name}")
            port = serial.Serial(port_name, 921600, timeout=0.1)

        # Create an instance of Board_DAQ with the opened port

        local_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1, f"propdaq_client_{port_name}")

        # Initialize TaskManager
        #automanager = TaskManager(automation_config['automation_sequence'], port, namespace, port_index=port_index)
        #abort_manager = TaskManager(abort_config['abort_sequence'], port, namespace, port_index=port_index, force=True)
        print("initboarddaq")
        port_ = Board_DAQ(port, port_index, local_client, port_name, namespace, automation_event, abort_event)
        port_.mqtt_client.connect(mqtt_broker_address, 1884, 60)
        port_.mqtt_client.loop_start()

        #automanager.run()
        
        try:
        # Create threads for read and publish functions
            read_thread = threading.Thread(target=port_.read_serial_and_log_high_freq)
            publish_thread = threading.Thread(target=port_.publish_data)
            #write_thread = threading.Thread(target=port_.write_commands)

            # Start the threads
            read_thread.start()
            publish_thread.start()

        except Exception as e:
            print(f"Thread encountered an error: {e}. Restarting...")


    except Exception as e:
        print(f"Error in process for port {port_name}: {e}")


"""def open_serial_ports():
    
    Detect and list available serial ports.
    
    available_ports = list(serial.tools.list_ports.comports())
    return [port.device for port in available_ports]  # Return port names"""


def main():
    try:
        #multiprocessing.set_start_method('spawn')

        # Shared timer for synchronization
        #manager = multiprocessing.Manager()

        with multiprocessing.Manager() as manager:
        # Create a shared namespace for storing the global timer
            namespace = manager.Namespace()
            namespace.global_timer = 0.0
            automation_event = manager.Event()  # Event to signal if automation started
            abort_event = manager.Event() 

        #global_timer = multiprocessing.Value('d', 0.0)  # 'd' indicates double (for time in seconds)

        # Start the fileserver once in the main process
            global fileserver_process
            fileserver_process = start_fileserver()

            timer_process = multiprocessing.Process(target=update_global_time, args=(namespace,))
            timer_process.daemon = True
            timer_process.start()
            time.sleep(0.1)

            # Initialize MQTT client
            """client.on_connect = on_connect
            client.on_message = on_message
            client.connect(mqtt_broker_address, 1884, 60)
            client.loop_start()"""

            open_serial_ports()

            # Get a list of available serial ports
            if not ports:
                print("No serial ports available.")
                return

            # Create a list to hold processes
            processes = []

            # Start a process for each serial port
            for i, port_name in enumerate(ports):
                if port_name:
                    process = multiprocessing.Process(target=process_for_port, args=(port_name, i, namespace, automation_event, abort_event))
                    process.start()
                    processes.append(process)
                    print(f"Started process for port {port_name}")

            # Wait for all processes to finish
            for process in processes:
                process.join()

    finally:
        # Ensure the fileserver process is stopped when exiting
        stop_fileserver(fileserver_process)


if __name__ == "__main__":
    main()
