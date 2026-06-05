import serial
import csv
from datetime import datetime

ser = serial.Serial('COM3', 115200)  # change COM3 to your port

with open(r'C:\Users\Samkafs\Downloads\Training data\sensor_data.csv', 'w', newline='') as f:
    writer = csv.writer(f)
    writer.writerow(['timestamp', 'temperature', 'pressure', 'humidity', 'gas_resistance'])
    
    while True:
        line = ser.readline().decode('utf-8').strip()
        values = line.split(' ')
        if len(values) == 4:
            writer.writerow([datetime.now().strftime('%Y-%m-%d %H:%M:%S')] + values)
            f.flush()  # write immediately, don't buffer
            print(line)