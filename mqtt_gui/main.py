import paho.mqtt.client as mqtt
import serial
import time
import json
from datetime import datetime
import threading
import numpy as np


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

# ADD FACTORS LISTS
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

board_id_to_conv_factor = {"1": [b1_cf_1015, b1_cf_1115, b1_thermocouple],
                           "2": [b2_cf_1015, b2_cf_1115, b2_thermocouple],
                           "3": [b3_cf_1015, b3_cf_1115, b3_thermocouple]
                        }

board_id_to_conv_offset = {"1": [b1_cf_1015_add, b1_cf_1115_add, b1_thermocouple_add],
                           "2": [b2_cf_1015_add, b2_cf_1115_add, b2_thermocouple_add],
                           "3": [b3_cf_1015_add, b3_cf_1115_add, b3_thermocouple_add]
                        }


client = mqtt.Client("propdaq")

# Port Management

ports = [False, False, False, False, False]

def open_serial_ports():
    try:
        #MAC
        #ports[0] = serial.Serial('/dev/cu.usbserial-0001', 921600)  
        #WINDOWS
        ports[0] = serial.Serial('COM6', 921600,timeout=1)
        #TC
        #ports[0] = serial.Serial('COM6', 921600) 
    except Exception as e:
            print(f"Port error: {e}")
        
    try:
        #FLOWMETER BOARD DON'T CHANGE PORT AND PORT INDEX
        """ MAKE SURE THIS IS PORT OF FLOWMETER BOARD """
        #MAC
        #ports[1] = serial.Serial('/dev/cu.usbmodem56292564361', 921600)  
        #WINDOWS
        ports[1] = serial.Serial('COM5', 921600) 
    except Exception as e:
            print(f"Port error: {e}")
    
    try:
        #FLOWMETER BOARD DON'T CHANGE PORT AND PORT INDEX
        """ MAKE SURE THIS IS PORT OF FLOWMETER BOARD """
        #MAC
        #ports[1] = serial.Serial('/dev/cu.usbmodem56292564361', 921600)  
        #WINDOWS
        ports[2] = serial.Serial('COM4', 921600) 
    except Exception as e:
            print(f"Port error: {e}")



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
b_to_solenoid_status_topic_dict = {"Board 4": 'switch_states_status_4', "Board 5": 'switch_states_status_5'}

b1015_conv_factor_dict = {"Board 1": b1_cf_1015, "Board 2": b2_cf_1015, "Board 3": b3_cf_1015, "Board 4": b4_cf_1015, "Board 5": b5_cf_1015}
b1115_conv_factor_dict = {"Board 1": b1_cf_1115, "Board 2": b2_cf_1115, "Board 3": b3_cf_1115, "Board 4": b4_cf_1115, "Board 5": b5_cf_1115}
bTC_conv_factor_dict = {"Board 1": b1_thermocouple, "Board 2": b2_thermocouple, "Board 3": b3_thermocouple, "Board 4": b4_thermocouple, "Board 5": b5_thermocouple}

b1015_add_factor_dict = {"Board 1": b1_cf_1015_add, "Board 2": b2_cf_1015_add, "Board 3": b3_cf_1015_add, "Board 4": b4_cf_1015_add, "Board 5": b5_cf_1015_add}
b1115_add_factor_dict = {"Board 1": b1_cf_1115_add, "Board 2": b2_cf_1115_add, "Board 3": b3_cf_1115_add, "Board 4": b4_cf_1115_add, "Board 5": b5_cf_1115_add}



number_to_sensor_type = {"1": "ADS 1015", "2": "ADS 1115", "3": "TC"}
number_to_sensor_type_publish = {"1": "1015", "2": "1115", "3": "TC"}

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
            
        
    
    if (message.topic == "switch_states_update_4"):
        command = "4" + message_payload + "\n"
        send_command = command.encode('utf-8')
        ports[0].write(send_command)
        ports[0].flush()
        print(send_command)
        
    if (message.topic == "switch_states_update_5"):
        command = "5" + message_payload + "\n"
        send_command = command.encode('utf-8')
        ports[0].write(send_command)
        ports[0].flush()
        print(send_command)

    if (message.topic == "switch_states_update_6"):
        command = "6" + message_payload + "\n"
        send_command = command.encode('utf-8')
        ports[2].write(send_command)
        ports[2].flush()
        print(send_command) 


class Board_DAQ():

    def __init__(self, port_index):
        self.port_index = port_index
        self.data_lock = threading.Lock()
        self.publish_dict = {}
    
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

    
                Board_ID = data_dict['BoardID'][0]
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
                    elif (Board_ID == '5'):
                        self.publish_dict[mqtt_switch_states_status_5] = publish_json
                    elif (Board_ID == '6'):
                        print("Trying to publish")
                        self.publish_dict[mqtt_switch_states_status_6] = publish_json

                    
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
                # , {data}")
        
                    
    def solenoid_write(self, message):
        ports[self.port_index].write(message)


def auto_ignite():
    try:
        global ignition_in_progress
        ignition_in_progress = True

        ematch_open = "401" + "\n"
        ematch_open_send_command = ematch_open.encode('utf-8')
        ematch_close = "400" + "\n"
        ematch_close_send_command = ematch_close.encode('utf-8')
        ox_main_open = "411" + "\n"
        ox_main_open_send_command = ox_main_open.encode('utf-8')
        ox_main_close = "410" + "\n"
        ox_main_close_send_command = ox_main_close.encode('utf-8')
        fuel_main_open = "421" + "\n"
        fuel_main_open_send_command = fuel_main_open.encode('utf-8')
        fuel_main_close = "420" + "\n"
        fuel_main_close_send_command = fuel_main_close.encode('utf-8')
        ox_purge_open = "431" + "\n"
        ox_purge_open_command = ox_purge_open.encode('utf-8')
        ox_purge_close = "430" + "\n"
        ox_purge_close_command = ox_purge_close.encode('utf-8')
        fuel_purge_open = "441" + "\n"
        fuel_purge_open_command = fuel_purge_open.encode('utf-8')
        fuel_purge_close = "440" + "\n"
        fuel_purge_close_command = fuel_purge_close.encode('utf-8')

        ports[0].write(ematch_open_send_command)
        ports[0].flush()

        if not ignition_in_progress: 
            t4 = threading.Thread(target=abort_process)
            t4.start()
            print("Ignition aborted!")
            return

        time.sleep(0.500)

        ports[0].write(ox_purge_open_command)
        ports[0].flush()
        ports[0].write(fuel_purge_open_command)
        ports[0].flush()

        if not ignition_in_progress: 
            t4 = threading.Thread(target=abort_process)
            t4.start()
            print("Ignition aborted!")
            return

        time.sleep(2.700)

        if not ignition_in_progress: 
            t4 = threading.Thread(target=abort_process)
            t4.start()
            print("Ignition aborted!")
            return

        ports[0].write(ox_main_open_send_command)
        ports[0].flush()

        if not ignition_in_progress: 
            t4 = threading.Thread(target=abort_process)
            t4.start()
            print("Ignition aborted!")
            return

        time.sleep(0.500)

        if not ignition_in_progress: 
            t4 = threading.Thread(target=abort_process)
            t4.start()
            print("Ignition aborted!")
            return

        ports[0].write(fuel_main_open_send_command)
        ports[0].flush()

        if not ignition_in_progress: 
            t4 = threading.Thread(target=abort_process)
            t4.start()
            print("Ignition aborted!")
            return

        time.sleep(0.100)

        if not ignition_in_progress: 
            t4 = threading.Thread(target=abort_process)
            t4.start()
            print("Ignition aborted!")
            return

        ports[0].write(ox_purge_close_command)
        ports[0].flush()
        ports[0].write(fuel_purge_close_command)
        ports[0].flush()

        if not ignition_in_progress: 
            t4 = threading.Thread(target=abort_process)
            t4.start()
            print("Ignition aborted!")
            return

        time.sleep(2.700)

        if not ignition_in_progress: 
            t4 = threading.Thread(target=abort_process)
            t4.start()
            print("Ignition aborted!")
            return

        ports[0].write(fuel_main_close_send_command)
        ports[0].flush()

        if not ignition_in_progress: 
            t4 = threading.Thread(target=abort_process)
            t4.start()
            print("Ignition aborted!")
            return

        time.sleep(0.250)

        if not ignition_in_progress: 
            t4 = threading.Thread(target=abort_process)
            t4.start()
            print("Ignition aborted!")
            return

        ports[0].write(ox_main_close_send_command)
        ports[0].flush()
        
        # E-Match Reset
        ports[0].write(ematch_close_send_command)
        ports[0].flush()

        client.publish("AUTO", "IGNITION SUCCESSFUL")
        print("IGNITION SUCCESSFUL")
    except Exception as e:
        print(e)
        print("IGNITION UNSUCCESSFUL")
    

def abort_process():
    try:
        ematch_close = "400" + "\n"
        ematch_close_send_command = ematch_close.encode('utf-8')
        ox_main_close = "410" + "\n"
        ox_main_close_send_command = ox_main_close.encode('utf-8')
        fuel_main_close = "420" + "\n"
        fuel_main_close_send_command = fuel_main_close.encode('utf-8')

        ports[0].write(fuel_main_close_send_command)
        ports[0].flush()
        time.sleep(0.050)
        ports[0].write(ox_main_close_send_command)
        ports[0].flush()

        # E-Match Reset
        ports[0].write(ematch_close_send_command)
        ports[0].flush()

        client.publish("AUTO", "ABORT SUCCESSFUL")
        print("ABORT SUCCESSFUL")
    except Exception as e:
        print(e)
        print("ABORT UNSUCCESSFUL")

        

def main():
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(mqtt_broker_address, 1884, 60)
    client.loop_start()

    open_serial_ports()

    port0 = Board_DAQ(0)

    t1 = threading.Thread(target=port0.read_serial_and_log_high_freq)
    t2 = threading.Thread(target=port0.publish_data)


    if (ports[0]):
        print("CAN BUS connection active")
        t1.start()
        t2.start()
    

if __name__ == '__main__':
    main()
