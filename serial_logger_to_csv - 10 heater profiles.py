import serial
import csv
from datetime import datetime

# Open serial port
ser = serial.Serial('COM3', 115200, timeout=1)

# CSV output file
file_path = r'C:\Users\Samkafs\Downloads\Training data\sensor_data.csv'

with open(file_path, 'w', newline='') as f:
    writer = csv.writer(f)

    # CSV Header
    writer.writerow([
        'timestamp',
        'R1', 'R2', 'R3', 'R4', 'R5',
        'R6', 'R7', 'R8', 'R9', 'R10',
        'humidity',
        'temperature',
        'pressure'
    ])

    print("Logging started...\n")

    while True:
        try:
            # Read serial line
            line = ser.readline().decode('utf-8').strip()

            # Skip empty lines
            if not line:
                continue

            print(line)

            # Split CSV values
            values = line.split(',')

            # Expect exactly 13 sensor values
            if len(values) == 13:

                # Add timestamp
                row = [datetime.now().strftime('%Y-%m-%d %H:%M:%S')] + values

                # Write row
                writer.writerow(row)

                # Save immediately
                f.flush()

            else:
                print(f"Skipped malformed line ({len(values)} values)")

        except KeyboardInterrupt:
            print("\nLogging stopped.")
            break

        except Exception as e:
            print(f"Error: {e}")