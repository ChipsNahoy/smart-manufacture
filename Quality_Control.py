import serial
import paho.mqtt.client as mqtt
from PIL import Image, ImageTk
import tkinter as tk

# Serial port configuration
# ser = serial.Serial('/dev/ttyUSB0', zz 9600)  # Change '/dev/ttyUSB0' to match your NodeMCU's serial port

# MQTT broker configuration
MQTT_BROKER = "192.168.132.184"  # Change this to your MQTT broker address
MQTT_PORT = 1885  # Change this to your MQTT broker port
MQTT_TOPIC = "qc"

def close_window(event=None):
    root.destroy()

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker!")
    else:
        print("Failed to connect, return code %d\n", rc)

def on_publish(client, userdata, mid):
    print("Message Published!")

def close_window(event=None):
    root.destroy()

def read_card_display_image():
    global root
    try:
        # Serial port configuration
        ser = serial.Serial('/dev/ttyUSB0', 9600)
        
        print("Hold your card near the reader...")
        prod = ser.readline().strip().decode('utf-8')
        print(f"From Serial: \n", prod)
        
        # MQTT publish
        client.publish(MQTT_TOPIC, prod)
        
        # Define image paths based on serial input
        if prod[-1] == "Y":
            image_path = "iy.jpeg"
        elif prod[-1] == "X":
            image_path = "ix.jpeg"
        elif prod[-1] == "Z":
            image_path = "iz.jpeg"
        else:
            print(f"Card is not Registered!!!\n")
            return
            
        root = tk.Tk()
        root.title("RFID Card Image")
        
        img = Image.open(image_path)
        img = img.resize((800,450), Image.ANTIALIAS)
        photo = ImageTk.PhotoImage(img)
        label = tk.Label(root, image=photo)
        label.pack(side=tk.LEFT, padx=10, pady=10)
        
        root.bind("<Key>", close_window)
        
        root.mainloop()
    finally:
        ser.close()

if __name__ == "__main__":
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_publish = on_publish
    
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    while True:
        read_card_display_image()
