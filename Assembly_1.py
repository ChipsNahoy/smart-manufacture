import serial
import paho.mqtt.client as mqtt
from PIL import Image, ImageTk
import tkinter as tk

MQTT_BROKER = "192.168.132.184"  # Change this to your MQTT broker address
MQTT_PORT = 1885  # Change this to your MQTT broker port
MQTT_TOPIC = "assembly_in_time"
# Serial port configuration
ser = serial.Serial('/dev/ttyUSB0', 9600)  # Change '/dev/ttyUSB0' to match your NodeMCU's serial port
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
        #print("UID (Hex): ", uid)
        print("From Serial: ", prod)
        client.publish(MQTT_TOPIC, prod)
        
       # image_path1 = ""
       # image_path2 = ""
        
        if prod[-1] == "Y":
            image_path1 = "Gy1.png"
            image_path2 = "Ay1.png"
        elif prod[-1] == "X":
            image_path1 = "Gx1.png"
            image_path2 = "Ax1.png"
        elif prod[-1] == "Z":
            image_path1 = "Gz1.png"
            image_path2 = "Az1.png"
        else:
            print(f"Card is not Registered!!!\n")
            return
            
        root = tk.Tk()
        root.title("RFID Card Image")
        
        img1 = Image.open(image_path1)
        img1 = img1.resize((625,625), Image.ANTIALIAS)
        photo1 = ImageTk.PhotoImage(img1)
        label1 = tk.Label(root, image=photo1)
        label1.pack(side=tk.LEFT, padx=10, pady=10)
        
        img2 = Image.open(image_path2)
        img2 = img2.resize((625,625), Image.ANTIALIAS)
        photo2 = ImageTk.PhotoImage(img2)
        label2 = tk.Label(root, image=photo2)
        label2.pack(side=tk.LEFT, padx=10, pady=10)
        
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
