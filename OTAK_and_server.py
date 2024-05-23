import random
from datetime import datetime
import os
import sqlite3
import serial
import threading
import paho.mqtt.client as mqtt
from datetime import datetime

# Function to parse input string
def parse_input(input_string):
    info_list = input_string.split(',')
    Order_ID, product_type, qty = None, None, None
    Order_ID = info_list[0].split(':')[1].strip()
    product_type = info_list[1].split(':')[1].strip()
    qty = info_list[2].split(':')[1].strip()

    return Order_ID, product_type, qty

# Function to update specified column in database
def update_to_column(Order_ID, column_name, column_data):
    conn = sqlite3.connect("data.db")
    c = conn.cursor()
    c.execute(f"UPDATE data SET {column_name} = ? WHERE Order_ID = ?", (column_data, Order_ID))
    conn.commit()
    conn.close()
    print('Update success')

# Function to save parsed information into database columns


def save_to_columns(Order_ID, date_rec, qty, product_type):
    if not os.path.exists("data.db"):
        conn = sqlite3.connect("data.db")
        c = conn.cursor()
        c.execute('''CREATE TABLE data (
                        Order_ID TEXT,
                        Date_order_received TEXT,
                        Product_type TEXT,
                        Quantity INTEGER,
                        material_handling_in_time TEXT,
                        warehouse_in_time TEXT,
                        assembly_in_time TEXT,
                        assembly_in_time_2 TEXT,
                        qc TEXT,
                        storage_in_time TEXT,
                        storage_out_time TEXT,
                        unit_progress TEXT
                    )''')
        conn.commit()
        conn.close()

    conn = sqlite3.connect("data.db")
    c = conn.cursor()
    c.execute("SELECT * FROM data WHERE Order_ID=?", (Order_ID,))
    existing_data = c.fetchone()
    if not existing_data:
        c.execute("INSERT INTO data (Order_ID, Date_order_received, material_handling_in_time, Product_type, unit_progress, Quantity) VALUES (?, ?, ?, ?, ?, ?)", (Order_ID, date_rec, date_rec, product_type, 'material handling', qty))
        conn.commit()
        conn.close()
        print('Save to columns success')
    else:
        # Check if any column is empty for the existing Order_ID
        existing_order_id = existing_data[0]
        existing_columns = existing_data[1:]
        if any(not column for column in existing_columns):
            #print('Some columns for the existing Order_ID are empty, data not inserted')
            pass
        else:
            print('All columns for the existing Order_ID have data, inserting new row...')
            c.execute("INSERT INTO data (Order_ID, Date_order_received, material_handling_in_time, Product_type, unit_progress, Quantity) VALUES (?, ?, ?, ?, ?, ?)", (Order_ID, date_rec, date_rec, product_type, 'material handling', qty))
            conn.commit()
            conn.close()
            print('Save to columns success')
# Function to send data to NodeMCU over serial
def send_to_nodemcu(data):
    ser = serial.Serial('/dev/ttyUSB0', 9600)
    ser.write(data.encode())
    ser.close()
    print('Sukses mengirim data ke NodeMCU')


#new database and save
    '''
// 1: Bolt
// 2: Nut silver
// 3: Nut black
// 4: Washer silver
// 5: Washer black

'''
def create_warehouse_database():
    if not os.path.exists("data_warehouse.db"):
        conn = sqlite3.connect("data_warehouse.db")
        c = conn.cursor()
        c.execute('''CREATE TABLE warehouse (
                        Order_ID TEXT,
                        Bolt INTEGER,
                        Nut_silver INTEGER,
                        Nut_black INTEGER,
                        Washer_silver INTEGER,
                        Washer_black INTEGER
                    )''')
        conn.commit()
        conn.close()

def save_to_warehouse(Order_ID, bolt, nut_silver, nut_black, washer_silver, washer_black):
    create_warehouse_database()  # Ensure the database exists

    conn = sqlite3.connect("data_warehouse.db")
    c = conn.cursor()
    c.execute("INSERT INTO warehouse (Order_ID, Bolt, Nut_silver, Nut_black, Washer_silver, Washer_black) VALUES (?, ?, ?, ?, ?, ?)", (Order_ID, bolt, nut_silver, nut_black, washer_silver, washer_black))
    conn.commit()
    conn.close()
    print('Data saved to warehouse')

# MQTT on_connect callback
def on_connect(client, userdata, flags, rc):
    print("Connected to MQTT Broker with result code "+str(rc))
    # Subscribe to MQTT topics
    client.subscribe([
        ("raw_storage", 0),
        ("assembly_in_time", 0),
        ("storage", 0),
        ('inventory', 0),
        
        ('assembly_in_time_2', 0),
        ('qc', 0)
    ])

# MQTT on_message callback
def on_message(client, userdata, msg):
    # Extract topic and payload
    topic = msg.topic
    payload = msg.payload.decode()
    print('Payload: ', payload)
    print('Topic: ', topic)
    if topic == 'inventory':
        parts = payload.split(',')
        parts = [part.strip() for part in parts]

    # Extract Order_ID and quantities
        Order_ID = parts[0].split(':')[1].strip()  # Extracting the Order_ID from the first part
        quantities = [int(part) for part in parts[1:]]
        save_to_warehouse(Order_ID, *quantities)
    else:
            
        p_split = payload.split(',')
        station_id = p_split[0].split(':')[1].strip()
        id = p_split[1].split(':')[1].strip()
        date = datetime.now().strftime("%Y-%m-%d-%H:%M:%S")
        product_type = p_split[3].split(':')[1].strip()
        
        if (topic == 'raw_storage'):
            column_name = 'warehouse_in_time'
            update_to_column(id, column_name, date)
            update_to_column(id, 'unit_progress', 'warehouse loading')
        elif (topic == 'assembly_in_time'):
            column_name = 'assembly_in_time'
            update_to_column(id, column_name, date)
            update_to_column(id, 'unit_progress', 'assembly 1  in')
        elif topic == 'assembly_in_time_2':
            column_name = 'assembly_in_time_2'
            update_to_column(id, column_name, date)
            update_to_column(id, 'unit_progress', 'assembly 2  in')
        elif topic == 'qc':
            column_name = 'qc'
            update_to_column(id, column_name, date)
            update_to_column(id, 'unit_progress', 'quality control start')
        
            
        if (len(p_split) == 5):
            in_out = p_split[4].strip()
            if in_out == 'in':
                column_name = 'storage_in_time'
                update_to_column(id, column_name, date)
                update_to_column(id, 'unit_progress', 'inside storage')
                
            elif in_out == 'out':
                column_name = 'storage_out_time'
                update_to_column(id, column_name, date)
                update_to_column(id, 'unit_progress', 'product sent')
    
def handle_serial_data():
    ser = serial.Serial('/dev/ttyUSB0', 9600)
    while True:
        serial_data = ser.readline().decode().strip()
        print("Serial Data Received:", serial_data)
        # Process serial data here
        
# Start serial data monitoring in a separate thread
serial_thread = threading.Thread(target=handle_serial_data)
serial_thread.start()

   


# Function to start MQTT client

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

# Connect to MQTT Broker
client.connect("0.0.0.0", 1885)

# Start the loop
client.loop_start()

# Start MQTT client in a separate thread
#mqtt_thread = threading.Thread(target=start_mqtt_client)
#mqtt_thread.start()

# Main loop for user input
while True:
    try:
        input_string = input("Masukkan teks content dari qr scan (tekan Ctrl+C untuk keluar): ")
        Order_ID, product_type, qty = parse_input(input_string)
        date_order_received = datetime.now().strftime("%Y-%m-%d")
        date_detail = datetime.now().strftime("%Y-%m-%d-%H:%M:%S")
        fix = f"ID unit: {Order_ID}, Date: {date_order_received}, Product type: {product_type}"
        
        save_to_columns(Order_ID, date_detail, qty, product_type)
        send_to_nodemcu(fix)
        print(fix, 'fix')
        #ser = serial.Serial('/dev/ttyUSB0', 9600)
        #serial_data = ser.readline().decode().strip()
        #print("Serial Data Received:", serial_data)

    except KeyboardInterrupt:
        print("\nExiting...")
        client.loop_stop()
        break
