#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include "cert.h" // Include the certificate file

// Pin Definitions
#define PIN_ECHO1 32
#define PIN_TRIG1 33
#define PIR1 35
#define PIN_ECHO2 25
#define PIN_TRIG2 26
#define PIR2 14

// Initialize LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Array to store parking states
int parking_states[2] = {0,0};

// Array to store motion sensor pins
int motion_sensors[2] = {PIR1, PIR2};

// WiFi credentials
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// MQTT Broker settings
const char* mqtt_server = "af131d05d32a4261a4b276030065628f.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const int sensor_number = 1;
const char* mqtt_client_id = "ESP8266Client-13412";

WiFiClientSecure espClient;
PubSubClient client(espClient);

const char* sensor_topic = "data/parking";
const char* hello_topic = "data/hello";

// Variable to track previous parking states
int prev_parking_states[2] = {0,0};

// Variable to track if LCD needs to be updated
bool lcd_needs_update = true;

// Function to set up WiFi connection
void setup_WiFi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password, 6);
  
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("WiFi is connected!");
  Serial.print("Local IP address is:");
  Serial.println(WiFi.localIP());
}

// Function to measure distance
int distance(int sensor_delay, int trig, int echo) {  
  digitalWrite(trig, HIGH);
  delayMicroseconds(sensor_delay);
  digitalWrite(trig, LOW);
  int distance = pulseIn(echo, HIGH);
  distance = distance / 58;
  return distance;
}

// Function to detect motion
bool motion_detection(int pir) {
  bool motion;
  motion = digitalRead(pir) == HIGH ? true : false;
  return motion;
}

// Function to print parking states on LCD
void print_parking_state() {
  if (lcd_needs_update) {
    lcd.clear();
    lcd.setCursor(0, 0);
    parking_states[0] == 0 ? lcd.print("Spot 1: free") : lcd.print("Spot 1: taken");
    lcd.setCursor(0, 1);
    parking_states[1] == 0 ? lcd.print("Spot 2: free") : lcd.print("Spot 2: taken");
    lcd_needs_update = false;
  }
}

// Callback function for MQTT messages
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  
  Serial.println();
}

// Function to handle MQTT reconnection
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Connected");
      client.subscribe(sensor_topic);
      client.publish(hello_topic, "Hello from ESP8266");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}


void setup() {
  pinMode(2, OUTPUT);
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();

  WiFi.mode(WIFI_STA);
  setup_WiFi();

  espClient.setCACert(root_ca);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  // Initialize sensor pin modes
  pinMode(PIN_ECHO1, INPUT); pinMode(PIN_ECHO2, INPUT);
  pinMode(PIN_TRIG1, OUTPUT); pinMode(PIN_TRIG2, OUTPUT);
  pinMode(PIR1, INPUT); pinMode(PIR2, INPUT);

  // Initialize previous parking states
  prev_parking_states[0] = parking_states[0];
  prev_parking_states[1] = parking_states[1];
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  
  client.loop();
  
  // Check for state changes and publish MQTT messages accordingly
  for (int i = 0; i < 2; i++) {
    if (motion_detection(motion_sensors[i])) {
      if (distance(100, i == 0 ? PIN_TRIG1 : PIN_TRIG2, i == 0 ? PIN_ECHO1 : PIN_ECHO2) <= 100) {
        parking_states[i] = 1; // Update parking state
        // Publish MQTT message with JSON-like format
        if (prev_parking_states[i] == 0) {

          String message = "{\"clientid\": \"" + String(mqtt_client_id) + "\", \"spot\": \"" + String(i + 1) + "\", \"status\": \"taken\", \"sensor_number\": \"" + String(sensor_number) + "\"}";
          client.publish(sensor_topic, message.c_str());
          lcd_needs_update = true; // LCD needs update
        }
      } else {
        parking_states[i] = 0; // Update parking state
        // Publish MQTT message with JSON-like format
        if (prev_parking_states[i] == 1) {
          String message = "{\"clientid\": \"" + String(mqtt_client_id) + "\", \"spot\": \"" + String(i + 1) + "\", \"status\": \"free\", \"sensor_number\": \"" + String(sensor_number) + "\"}";
          client.publish(sensor_topic, message.c_str());
          lcd_needs_update = true; // LCD needs update
        }
      }
    }
    // Update previous parking states
    prev_parking_states[i] = parking_states[i];
  }
  
  // Update LCD if needed
  print_parking_state();
  delay(1000);
}