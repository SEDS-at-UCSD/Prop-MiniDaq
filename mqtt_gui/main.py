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
import yaml
import time



# IGNITION FLAG
ignition_in_progress = False

# MQTT configuration
#mqtt_broker_address = "169.254.32.191"
mqtt_broker_address = "localhost"
mqtt_topic_serial = "serial_data"


mqtt_switch_states_update_4 = "switch_states_update_4"
mqtt_switch_states_status_4 = "switch_states_status_4"
mqtt_switch_states_update_5 = "switch_states_update_5"
mqtt_switch_states_status_5 = "switch_states_status_5"
mqtt_switch_states_update_6 = "switch_states_update_6"
mqtt_switch_states_status_6 = "switch_states_status_6"



with open('conversion_factor_config.json', 'r') as json_file:
    conv_configs = json.load(json_file)

# CONV FACTORS LISTS
b1_cf_1015 = conv_configs['B1_Conv_Factor_ADS1015']
b1_cf_1115 = conv_configs['B1_Conv_Factor_ADS1115']
b1_thermocouple = conv_configs['B1_Thermocouple']
b1_thermocouple_add = conv_configs['B1_Thermocouple_ADD']

b2_cf_1015 = conv_configs['B2_Conv_Factor_ADS1015']
b2_cf_1115 = conv_configs['B2_Conv_Factor_ADS1115']
b2_thermocouple = conv_configs['B2_Thermocouple']
b2_thermocouple_add = conv_configs['B2_Thermocouple_ADD']

b3_cf_1015 = conv_configs['B3_Conv_Factor_ADS1015']
b3_cf_1115 = conv_configs['B3_Conv_Factor_ADS1115']
b3_thermocouple = conv_configs['B3_Thermocouple']
b3_thermocouple_add = conv_configs['B3_Thermocouple_ADD']

b4_cf_1015 = conv_configs['B4_Conv_Factor_ADS1015']
b4_cf_1115 = conv_configs['B4_Conv_Factor_ADS1115']
b4_thermocouple = conv_configs['B4_Thermocouple']
b4_thermocouple_add = conv_configs['B4_Thermocouple_ADD']

b5_cf_1015 = conv_configs['B5_Conv_Factor_ADS1015']
b5_cf_1115 = conv_configs['B5_Conv_Factor_ADS1115']
b5_thermocouple = conv_configs['B5_Thermocouple']
b5_thermocouple_add = conv_configs['B5_Thermocouple_ADD']

b6_cf_1015 = conv_configs['B6_Conv_Factor_ADS1015']
b6_cf_1115 = conv_configs['B6_Conv_Factor_ADS1115']
b6_thermocouple = conv_configs['B6_Thermocouple']
b6_thermocouple_add = conv_configs['B6_Thermocouple_ADD']

# ADD ACTORS LISTS
b1_cf_1015_add = conv_configs['B1_1015_ADD']
b1_cf_1115_add = conv_configs['B1_1115_ADD']
b2_cf_1015_add = conv_configs['B2_1015_ADD']
b2_cf_1115_add = conv_configs['B2_1115_ADD']
b3_cf_1015_add = conv_configs['B3_1015_ADD']
b3_cf_1115_add = conv_configs['B3_1115_ADD']
b4_cf_1015_add = conv_configs['B4_1015_ADD']
b4_cf_1115_add = conv_configs['B4_1115_ADD']
b5_cf_1015_add = conv_configs['B5_1015_ADD']
b5_cf_1115_add = conv_configs['B5_1115_ADD']
b6_cf_1015_add = conv_configs['B6_1015_ADD']
b6_cf_1115_add = conv_configs['B6_1115_ADD']

board_id_to_conv_factor = {"1": [b1_cf_1015, b1_cf_1115, b1_thermocouple],
                           "2": [b2_cf_1015, b2_cf_1115, b2_thermocouple],
                           "3": [b3_cf_1015, b3_cf_1115, b3_thermocouple]
                        }

board_id_to_conv_offset = {"1": [b1_cf_1015_add, b1_cf_1115_add, b1_thermocouple_add],
                           "2": [b2_cf_1015_add, b2_cf_1115_add, b2_thermocouple_add],
                           "3": [b3_cf_1015_add, b3_cf_1115_add, b3_thermocouple_add]
                        }


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

b1015_conv_factor_dict = {"Board 1": b1_cf_1015, "Board 2": b2_cf_1015, "Board 3": b3_cf_1015, "Board 4": b4_cf_1015, "Board 5": b5_cf_1015, "Board 6": b6_cf_1015}
b1115_conv_factor_dict = {"Board 1": b1_cf_1115, "Board 2": b2_cf_1115, "Board 3": b3_cf_1115, "Board 4": b4_cf_1115, "Board 5": b5_cf_1115, "Board 6": b6_cf_1115}
bTC_conv_factor_dict = {"Board 1": b1_thermocouple, "Board 2": b2_thermocouple, "Board 3": b3_thermocouple, "Board 4": b4_thermocouple, "Board 5": b5_thermocouple}

b1015_add_factor_dict = {"Board 1": b1_cf_1015_add, "Board 2": b2_cf_1015_add, "Board 3": b3_cf_1015_add, "Board 4": b4_cf_1015_add, "Board 5": b5_cf_1015_add, "Board 6": b6_cf_1015_add}
b1115_add_factor_dict = {"Board 1": b1_cf_1115_add, "Board 2": b2_cf_1115_add, "Board 3": b3_cf_1115_add, "Board 4": b4_cf_1115_add, "Board 5": b5_cf_1115_add, "Board 6": b6_cf_1115_add}



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
    client.subscribe(mqtt_switch_states_update_4)
    client.subscribe(mqtt_switch_states_update_5)
    client.subscribe(mqtt_switch_states_update_6)
    client.subscribe(ignition_topic)
    


def on_message(client, userdata, message):
    global ignition_in_progress
    # MAP SOLENOID BOARDS HERE
    print(f"Received message on topic '{message.topic}': {message.payload.decode('utf-8')}")
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
            
            
        
    
    if (message.topic == "switch_states_update_4"):
        command = "4" + message_payload + "\n"
        send_command = command.encode('utf-8')
        for port in ports:
            if port:
                port.write(send_command)
                port.flush()
                print(send_command)
        
    if (message.topic == "switch_states_update_5"):
        command = "5" + message_payload + "\n"
        send_command = command.encode('utf-8')
        for port in ports:
            if port:
                port.write(send_command)
                port.flush()
                print(send_command)

    if (message.topic == "switch_states_update_6"):
        command = "6" + message_payload + "\n"
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
                if int(Board_ID) in range(4, 7):
                    raw_byte_array = data_dict['Sensors']
                    publish_array = raw_byte_array[0:5]
                    publish_json_dict = {"time": str(datetime.now())[11:22], "sensor_readings": publish_array}
                    publish_json = json.dumps(publish_json_dict)
                    #print(Board_ID + " " + publish_json)

                    if (Board_ID == '4'):
                        self.publish_dict[mqtt_switch_states_status_4] = publish_json
                        #print("Trying to publish to 4")
                    elif (Board_ID == '5'):
                        self.publish_dict[mqtt_switch_states_status_5] = publish_json
                        #print("Trying to publish to 5")
                    elif (Board_ID == '6'):
                        self.publish_dict[mqtt_switch_states_status_6] = publish_json
                        #print("Trying to publish to 6")
                    
                    file_to_write.write(publish_json + "\n")
                    file_to_write.flush()  

                    
                # DAQ BOARDS
                elif (int(Board_ID) in range(1,4)):        
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
    

if __name__ == '__main__':
    main()
