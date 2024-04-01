from machine import Pin
from time import sleep
from umqtt.simple import MQTTClient
import network
import time
import json
import dht
import config

class RGBLed:
    def __init__(self, pin_red, pin_green, pin_blue):
        self.pin_red = Pin(pin_red, Pin.OUT)
        self.pin_green = Pin(pin_green, Pin.OUT)
        self.pin_blue = Pin(pin_blue, Pin.OUT)

    def set_red(self):
        self.pin_red.value(0)
        self.pin_green.value(1)
        self.pin_blue.value(1)

    def set_green(self):
        self.pin_red.value(1)
        self.pin_green.value(0)
        self.pin_blue.value(1)

def connect_wifi(ssid, password):
    print("Connecting to WiFi", end="")
    sta_if = network.WLAN(network.STA_IF)
    sta_if.active(True)
    sta_if.connect(ssid, password)
    while not sta_if.isconnected():
        print(".", end="")
        time.sleep(0.1)
    print(" Connected!")
    return sta_if

def connect_mqtt(broker, client_id, user, password, port=8883):
    print("Connecting to MQTT server... ", end="")
    client = MQTTClient(client_id, broker, port=port, user=user, password=password, ssl=True, ssl_params={'server_hostname': broker})
    client.connect()
    print("Connected!")
    return client

# Define WiFi credentials
ssid = config.wifi_ssid
password = config.wifi_password

# Define MQTT parameters
mqtt_broker = config.mqtt_broker
mqtt_client_id = config.mqtt_client_id
mqtt_user = config.mqtt_user
mqtt_password = config.mqtt_password
mqtt_topic = "data/parking"

# Initialize the RGB LEDs
led1 = RGBLed(19, 5, 2)  # First LED
led2 = RGBLed(13, 26, 32)  # Second LED

# Connect to WiFi
wifi_interface = connect_wifi(ssid, password)

# Connect to MQTT broker
mqtt_client = connect_mqtt(mqtt_broker, mqtt_client_id, mqtt_user, mqtt_password)

# Callback function for MQTT messages
def mqtt_callback(topic, msg):
    print("Message received on topic: %s, Message: %s" % (topic, msg))
    try:
        data = json.loads(msg)
        sensor_number = data.get("sensor_number")
        
        spot = data.get("spot")
        status = data.get("status")
        
        if sensor_number == "1":
            if spot == "1":
                if status == "taken":
                    led1.set_red()
                elif status == "free":
                    led1.set_green()
            elif spot == "2":
                if status == "taken":
                    led2.set_red()
                elif status == "free":
                    led2.set_green()
        else:
            print("Unknown sensor number:", sensor_number)
            
    except Exception as e:
        print("Error processing MQTT message:", e)

# Subscribe to MQTT topic
mqtt_client.set_callback(mqtt_callback)
mqtt_client.subscribe(mqtt_topic)

# Main loop
while True:
    mqtt_client.wait_msg()
