import serial
import csv
import time
from datetime import datetime

def read_and_log_data(ser, csv_writer,csv_file):
    count = 0;
    try:
        while True:
            try:
                data = ser.readline().decode().strip()
                timestamp = "UTC " + datetime.utcnow().strftime('%Y-%m-%d %H:%M:%S.%f') #time.strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]  # Millisecond precision
                split_data = [value.split(':')[1].strip() for value in data.split('\t') if ':' in value]
                split_PSI = [("PSI:" + str(float(value.split(':')[1].strip())/5)) for value in data.split('\t') if ':' in value]
                # Write data to the CSV file
                csv_writer.writerow([timestamp, data] + split_data + split_PSI)
                if (count>= 50): #approx every 300 ms
                    print(split_PSI)
                    count = 0;
                count = count + 1;
                csv_file.flush()
            except Exception as e:
                print(e)

    except KeyboardInterrupt:
        print("Logging stopped by user.")

    finally:
        # Close the serial connection and CSV file
        ser.close()
        csv_file.close()

# Define the COM port names for your devices
com_port1 = '/dev/cu.wchusbserial56292564361'  #Double ADS for PT
com_port2 = '/dev/cu.wchusbserial55770170291'  #Single ADS for Flow Meter

# Define the baud rate
baud_rate = 921600

# Specify which COM port(s) to read from
read_com_port1 = True
read_com_port2 = False

print("Logging Starting...")
print("Port1:",com_port1,"\t Read:", read_com_port1)
print("Port2:",com_port2,"\t Read:", read_com_port2)

# Create serial objects and CSV files for logging
if read_com_port1:
    ser1 = serial.Serial(com_port1, baud_rate)
    csv_file1 = open('data_from_com1.csv', 'a', newline='')
    csv_writer1 = csv.writer(csv_file1, delimiter=',')

if read_com_port2:
    ser2 = serial.Serial(com_port2, baud_rate)
    csv_file2 = open('data_from_com2.csv', 'a', newline='')
    csv_writer2 = csv.writer(csv_file2, delimiter=',')

try:
    if read_com_port1:
        read_and_log_data(ser1, csv_writer1,csv_file1)

    if read_com_port2:
        read_and_log_data(ser2, csv_writer2,csv_file2)

except KeyboardInterrupt:
    print("Logging stopped by user.")

finally:
    if read_com_port1:
        ser1.close()
        csv_file1.close()
    if read_com_port2:
        ser2.close()
        csv_file2.close()
