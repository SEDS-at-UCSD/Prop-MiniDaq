import paho.mqtt.client as mqtt
import serial
import time
import json
from datetime import datetime
import threading


# MQTT configuration
mqtt_broker_address = "169.254.32.191"
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

mqtt_switch_states_update_4 = "switch_states_update_4"
mqtt_switch_states_status_4 = "switch_states_status_4"
mqtt_switch_states_update_5 = "switch_states_update_5"
mqtt_switch_states_status_5 = "switch_states_status_5"



with open('conversion_factor_config.json', 'r') as json_file:
    conv_configs = json.load(json_file)

# CONV FACTORS LISTS
b1_cf_1015 = conv_configs['B1_Conv_Factor_ADS1015']
b1_cf_1115 = conv_configs['B1_Conv_Factor_ADS1115']
b1_thermocouple = conv_configs['B1_Thermocouple']

b2_cf_1015 = conv_configs['B2_Conv_Factor_ADS1015']
b2_cf_1115 = conv_configs['B2_Conv_Factor_ADS1115']
b2_thermocouple = conv_configs['B2_Thermocouple']

b3_cf_1015 = conv_configs['B3_Conv_Factor_ADS1015']
b3_cf_1115 = conv_configs['B3_Conv_Factor_ADS1115']
b3_thermocouple = conv_configs['B3_Thermocouple']

b4_cf_1015 = conv_configs['B4_Conv_Factor_ADS1015']
b4_cf_1115 = conv_configs['B4_Conv_Factor_ADS1115']
b4_thermocouple = conv_configs['B4_Thermocouple']

b5_cf_1015 = conv_configs['B5_Conv_Factor_ADS1015']
b5_cf_1115 = conv_configs['B5_Conv_Factor_ADS1115']
b5_thermocouple = conv_configs['B5_Thermocouple']

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


client = mqtt.Client("propdaq")

# Port Management

ports = [False, False, False, False, False]

def open_serial_ports():
    try:
        #MAC
        #ports[0] = serial.Serial('/dev/cu.usbserial-0001', 921600)  
        #WINDOWS

        #TC
        ports[0] = serial.Serial('COM6', 921600) 
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
        # MAC 
        #ports[2] = serial.Serial('/dev/cu.usbserial-3', 921600)  
        # WINDOWS
        ports[2] = serial.Serial('COM10', 921600) 
    except Exception as e:
            print(f"Port error: {e}")
    
    try:
        # MAC
        #ports[3] = serial.Serial('/dev/cu.usbserial-4', 921600)  
        # WINDOWS
        ports[3] = serial.Serial('COM7', 921600)  
    except Exception as e:
            print(f"Port error: {e}")
    
    try:
        # MAC
        #ports[4] = serial.Serial('/dev/cu.usbserial-5', 921600)  
        # WINDOWS

        ports[4] = serial.Serial('COM11', 921600)  
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
b1015_to_topic_dict = {"Board 1": b1_mqtt_log_1015, "Board 2": b2_mqtt_log_1015, "Board 3": b3_mqtt_log_1015, "Board 4": b4_mqtt_log_1015, "Board 5": b5_mqtt_log_1015}
b1115_to_topic_dict = {"Board 1": b1_mqtt_log_1115, "Board 2": b2_mqtt_log_1115, "Board 3": b3_mqtt_log_1115, "Board 4": b4_mqtt_log_1115, "Board 5": b5_mqtt_log_1115}
b_to_solenoid_status_topic_dict = {"Board 4": 'switch_states_status_4', "Board 5": 'switch_states_status_5'}

b1015_conv_factor_dict = {"Board 1": b1_cf_1015, "Board 2": b2_cf_1015, "Board 3": b3_cf_1015, "Board 4": b4_cf_1015, "Board 5": b5_cf_1015}
b1115_conv_factor_dict = {"Board 1": b1_cf_1115, "Board 2": b2_cf_1115, "Board 3": b3_cf_1115, "Board 4": b4_cf_1115, "Board 5": b5_cf_1115}
bTC_conv_factor_dict = {"Board 1": b1_thermocouple, "Board 2": b2_thermocouple, "Board 3": b3_thermocouple, "Board 4": b4_thermocouple, "Board 5": b5_thermocouple}

b1015_add_factor_dict = {"Board 1": b1_cf_1015_add, "Board 2": b2_cf_1015_add, "Board 3": b3_cf_1015_add, "Board 4": b4_cf_1015_add, "Board 5": b5_cf_1015_add}
b1115_add_factor_dict = {"Board 1": b1_cf_1115_add, "Board 2": b2_cf_1115_add, "Board 3": b3_cf_1115_add, "Board 4": b4_cf_1115_add, "Board 5": b5_cf_1115_add}




# Fast changing lists storing data from Board DAQ
"""

Data at each index
0 - Formatted data for local file logging 
1 - Sensor Type ('ADS1015' / 'ADS1115')
2 - Publish JSON (To publish to mqtt for frontend of GUI)
3 - Board ID ('Board 1', 'Board 2', 'Board 3', 'Board 4', 'Board 5')

"""
datatopass0 = ["", "", "", ""]
datatopass1 = ["", "", "", ""]
datatopass2 = ["", "", "", ""]
datatopass3 = ["", "", "", ""]
datatopass4 = ["", "", "", ""]
data_lock = threading.Lock()


def on_connect(client, userdata, flags, rc):
    print("Connected to MQTT broker with result code " + str(rc))
    client.subscribe(mqtt_switch_states_update_4)
    client.subscribe(mqtt_switch_states_update_5)
    

def write(self, port, message):
        port.write(message)


def on_message(client, userdata, message):
    # MAP SOLENOID BOARDS HERE
    """solenoid_Boards = {'4': ports[0], '5': ports[3]}
    print(f"Received message on topic '{message.topic}': {message.payload.decode('utf-8')}")
    
    if (message.topic == "switch_states_update_4"):
        command = message.payload.decode('utf-8')
        send_command = command.encode('utf-8')
        write(solenoid_write, solenoid_Boards['4'], send_command)
        print(send_command)
    if (message.topic == "switch_states_update_5"):
        command = message.payload.decode('utf-8')
        send_command = command.encode('utf-8')
        write(solenoid_write, solenoid_Boards['5'], send_command)
        print(send_command)"""
        


class Board_DAQ():
    def __init__(self, port_index, data_array):
        self.port_index = port_index
        self.data_array = data_array
        self.data_lock = threading.Lock()
    
    def read_serial_and_log_high_freq(self):
        while True:
            try:
                data = ports[self.port_index].readline().decode('utf-8').strip()
                data_dict = json.loads(data)
                data_array = self.data_array

                Board_ID = data_dict['BoardID']

                file_to_write = board_to_log_file_dict[data_dict['BoardID']]

                # Accesses list of conversion factors per pin
                conv_factor_1015 = b1015_conv_factor_dict[Board_ID]
                conv_factor_1115 = b1015_conv_factor_dict[Board_ID]
                conv_factor_TC = bTC_conv_factor_dict[Board_ID]
                add_factor_1015 = b1015_add_factor_dict[Board_ID]
                add_factor_1115 = b1115_add_factor_dict[Board_ID]
                
                """Update array so can check at time of publish"""
                data_array[3] = Board_ID
                
                data_formatted = (
                    str(datetime.now())[11:] 
                    + " "
                    + str(data_dict['BoardID'])
                    + "  "
                    + str(data_dict['SensorType'])
                    + "  ")

                if data_dict['Sensors'] != "Current" and data_dict['Sensors'] != "Voltage":
                    for i in range(len(data_dict['Sensors'])):
                        data_formatted += str(data_dict['Sensors'][i]) + "  "
                
                data_formatted += "\n"
                
                
                with data_lock:
                    data_array[0] = data_formatted
                    data_array[1] = data_dict['SensorType']

                    converted_values = [0, 0, 0, 0, 0]

                    publish_json_dict = {"time": str(datetime.now())[11:22], "sensor_readings": converted_values}

                    if data_dict['Sensors'] != "Current" and data_dict['Sensors'] != "Voltage":
                        for i in range(len(data_dict['Sensors'])):
                            if data_array[1] == 'ADS1015':
                                converted_values[i] = (round((data_dict['Sensors'][i] * conv_factor_1015[i]) + add_factor_1015[i], 1))
                            elif data_array[1] == 'ADS1115':
                                converted_values[i] = (round((data_dict['Sensors'][i] * conv_factor_1115[i]) + add_factor_1115[i], 1))
                            elif data_array[1] == 'Thermocouple':
                                converted_values[i] = (round(data_dict['Sensors'][i] * conv_factor_TC[i], 1))
                            elif data_array[1] == 'Solenoids':
                                converted_values[i] = data_dict['Sensors'][i]
                            else:
                                converted_values[i] = (round(data_dict['Sensors'][i] * -1000, 1))
                        
                

                    publish_json = json.dumps(publish_json_dict)
                    #print(publish_json)

                    data_array[2] = publish_json

                    file_to_write.write(data_formatted)
                    file_to_write.flush()  # Flush the buffer to ensure data is written immediately

            except Exception as e:
                print(f"Serial read error: {e}")  
                # , {data}")


    def publish_data(self):
        curr_time = time.time()
        data_array = self.data_array
        value1015 = ""
        value1115 = ""
        valueSolenoidUpdates = ""

        ## REFERENCE DOESN'T ACTUALLY ASSIGN 
        
        while True:

            time.sleep(0.001)
            toPublish = True
            Board_ID = data_array[3]
            if (Board_ID in b1015_to_topic_dict):
                board_topic_1015 = b1015_to_topic_dict[Board_ID]
                board_topic_1115 = b1115_to_topic_dict[Board_ID]
            if (Board_ID in b_to_solenoid_status_topic_dict):
                board_topic_Solenoid = b_to_solenoid_status_topic_dict[Board_ID]
            else:
                toPublish = False
                

            with data_lock:
                if data_array[1] == 'ADS1015':
                    value1015 = data_array[2]

                elif data_array[1] == 'ADS1115':
                    value1115 = data_array[2]
                    
                elif data_array[1] == 'Thermocouple':
                    value1015 = data_array[2]

                elif data_array[1] == 'Solenoids':
                    valueSolenoidUpdates = data_array[2]


            time_passed = time.time()-curr_time
            if toPublish:
                if (time_passed >= 0.05):
                    curr_time = time.time()
                    #print(data_array)
                    
                    if(valueSolenoidUpdates != ""):
                        client.publish(board_topic_Solenoid, valueSolenoidUpdates)
                        print(valueSolenoidUpdates)
                    else:
                        if(value1015 != ""):
                            client.publish(board_topic_1015, value1015)
                            print(value1015)
                        if(value1115 != ""):
                            client.publish(board_topic_1115, value1115)
                            print(value1115)

                    
                        #print(valueSolenoidUpdates)
                    


def solenoid_write(self, message):
        ports[self.port_index].write(message)
    

# CHANGED FOR FLOWMETER BOARD DON'T CHANGE PORT AND PORT INDEX
def read_serial_and_log_high_freq_flowmeter():
    while True:
        try:
            data = ports[1].readline().decode('utf-8').strip()
            data_dict = json.loads(data) 
            file_to_write = board_to_log_file_dict[data_dict['BoardID']]

            """
            datatopass1[3] = data_dict['BoardID']
            """
            
            # print(data_dict)
                
            # COMMENTED FOR FLOWMETER BOARD TO WORK
            """
            for i in range(len(data_dict['Sensors'])):
                data_formatted += str(data_dict['Sensors'][i]) + ""
            """
                           
            
            with data_lock:
                
                """datatopass1[0] = data_formatted"""
                #datatopass2[1] = data_dict['SensorType']
                """publish_json = (
                "{\"time\": \"" + str(datetime.now())[11:22] 
                + "\","
                + "\"sensor_readings\": "
                )"""

                #converted_values = [0, 0, 0, 0, 0]

                """datatopass1[1] = 'mV'"""
                
                """
                for i in range(len(data_dict['Sensors'])):
            
                publish_json += str(converted_values)
                publish_json += '}'
                datatopass1[2] = publish_json
                """
                #print(publish_json)

                file_to_write.write(str(datetime.now())[11:] + " " + str(data_dict) + "\n")
                file_to_write.flush()  # Flush the buffer to ensure data is written immediately

        except Exception as e:
            print(f"Serial read error - Flowmeter: {e}")
            #, {data}") 



# CHANGED FOR FLOWMETER BOARD DON'T CHANGE PORT AND PORT INDEX
def publish_data_flowmeter():
    curr_time = time.time()
    value1015 = ""
    while True:
        time.sleep(0.001)
        #time.sleep(0.05)
        topublish = True
        if (datatopass1[3] == "Board 1"):
            board_topic_1015 = b1_mqtt_log_1015
        elif (datatopass1[3] == "Board 2"):
            board_topic_1015 = b2_mqtt_log_1015
        elif (datatopass1[3] == "Board 3"):
            board_topic_1015 = b3_mqtt_log_1015
        elif (datatopass1[3] == "Board 4"):
            board_topic_1015 = b4_mqtt_log_1015
        elif (datatopass1[3] == "Board 5"):
            board_topic_1015 = b5_mqtt_log_1015
        else:
            topublish = False
            
        #datatopass1[1] == 'ADS1015'
        with data_lock:

            # MODIFIED FOR FLOWMETER BOARD
            if datatopass1[1] == 'mV':
            #if datatopass2[1] == 'ADS1015':
                value1015 = datatopass2[2]
                #client.publish(board_topic_1015, datatopass3[2])

        time_passed = time.time()-curr_time
        if topublish:
            if (time_passed >= 0.05):
                curr_time = time.time()
                #print(value1015)
                print(value1015)
                """print(value1115)"""
                client.publish(board_topic_1015, value1015)
                """client.publish(board_topic_1115, value1115)"""

            

def main():
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(mqtt_broker_address, 1883, 60)
    client.loop_start()

    open_serial_ports()

    port0 = Board_DAQ(0, datatopass0)
    port2 = Board_DAQ(2, datatopass2)
    port3 = Board_DAQ(3, datatopass3)
    port4 = Board_DAQ(4, datatopass4)

    t1 = threading.Thread(target=port0.read_serial_and_log_high_freq)
    t2 = threading.Thread(target=read_serial_and_log_high_freq_flowmeter)
    t3 = threading.Thread(target=port2.read_serial_and_log_high_freq)
    t4 = threading.Thread(target=port3.read_serial_and_log_high_freq)
    t5 = threading.Thread(target=port4.read_serial_and_log_high_freq)


    t6 = threading.Thread(target=port0.publish_data)
    t7 = threading.Thread(target=publish_data_flowmeter)
    t8 = threading.Thread(target=port2.publish_data)
    t9 = threading.Thread(target=port3.publish_data)
    t10 = threading.Thread(target=port4.publish_data)


    if (ports[0]):
        print("Port 0 functional")
        t1.start()
        t6.start()

    if (ports[1]):
        print("Port 1 functional")
        t2.start()
        t7.start()

    if (ports[2]):
        print("Port 2 functional")
        t3.start()
        t8.start()
    
    if (ports[3]):
        print("Port 3 functional")
        t4.start()
        t9.start()
    
    if (ports[4]):
        print("Port 4 functional")
        t5.start()
        t10.start()
    

if __name__ == '__main__':
    main()
