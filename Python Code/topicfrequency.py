import json
import serial
import time
from collections import defaultdict

# Configure your serial port (replace with your port and baudrate)
ser = serial.Serial('/dev/cu.usbserial-0001', 921600)

def extract_board_id(board_id_hex):
    try:
        # Convert the hex string to an integer, then perform boardID // 16 (equivalent to /10 in hex)
        return int(board_id_hex, 16) // 16
    except ValueError as e:
        #print(f"ValueError: Invalid hex value '{board_id_hex}'. Skipping this line.")
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
    return sensor_type, sensors

# Dictionary to store counts for each board's sensor types
boards_data = defaultdict(lambda: defaultdict(int))

# Store the last time we calculated frequencies
last_time = time.time()
interval = 1.0  # Time interval in seconds to calculate frequency

while True:
    if ser.in_waiting > 0:
        serial_data = ser.readline().decode('utf-8').strip()
        try:
            # Parse the serial data as JSON
            data = json.loads(serial_data)
            
            # Extract the board ID, skip if invalid hex or key is missing
            try:
                extractdata = data.get("BoardID", "")
            except AttributeError as e:
                continue

            board_id = extract_board_id(extractdata)
            if board_id is None:
                continue

            # Process sensor data, skip if there's an error
            sensor_type, sensors = process_data(data)
            if sensor_type is None or sensors is None:
                continue

            # Update count of sensor types for this specific board ID
            boards_data[board_id][sensor_type] += 1
            
            # Check if interval has passed
            current_time = time.time()
            if current_time - last_time >= interval:
                print(f"\nData summary for the past {interval} second(s):")

                # Print data for each board ID
                for board, sensor_data in boards_data.items():
                    print(f"\nBoard ID: {board}")
                    print("Sensor Type Frequencies (per second):")
                    for stype, count in sensor_data.items():
                        print(f"  Sensor Type {stype}: {count / interval} Hz")
                
                # Reset counts and timer
                boards_data.clear()
                last_time = current_time

        except json.JSONDecodeError:
            ser.flush()
            #print("Error decoding JSON from serial input. Skipping this line.")
