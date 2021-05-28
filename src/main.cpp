#include <Arduino.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <FirebaseArduino.h>
#include <Wire.h>
#include <SSD1306Wire.h>

#include "creds.h"

// #define DONT_HAVE_SENSORS

#define DHTPIN 		D5
#define DHTTYPE		DHT11

#define BAUDRATE	115200

struct readings {
  float hum;  				// Humidity in Percent	  ( %)
  float temp;  				// Temperature in Celsius (°C)
} readings;

// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);

// Initialize the OLED display using Wire library
SSD1306Wire  display(0x3c, D2, D1);  //D2=SDK  D1=SCK  As per labeling on NodeMCU

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store

// will store last time data was sent to Firebase
unsigned long previousMillis = 0;
const long interval = 2000;

void connectToWiFi(char const *ssid, char const *password);
void readSensors(struct readings *r);
void displaySensors(char const *ssid, struct readings r);
void sendDataToFirebase(struct readings r);

void setup() {
  delay(1000);
  Serial.begin(BAUDRATE);
  Serial.println("");
  Serial.println("Initializing OLED Display");
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  connectToWiFi(SSID, PASSWORD);
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  dht.begin();
}

void loop() {
  // check to see if it's time to send data to Firebase; that is, if the difference
  // between the current time and last time we sent data is bigger than
  // the interval at which we want to send data.
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time we sent data to Fireabase
    previousMillis = currentMillis;
    readSensors(&readings);
    displaySensors(SSID, readings);
    sendDataToFirebase(readings);
  }
}

void connectToWiFi(char const *ssid, char const *password) {
  delay(10);
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to SSID: ");
  display.drawString(0, 0, "Connecting to SSID: " + String(ssid));
  display.display();
  Serial.print(ssid);
  Serial.print(" ");
  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
  would try to act as both a client and an access-point and could cause
  network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  //start connecting to WiFi
  WiFi.begin(ssid, password);
  //while client is not connected to WiFi keep loading
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to SSID");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  display.clear();
  display.drawString(0, 10, "WiFi connected to: " + String(ssid));
  display.drawString(0, 20, "IP: " + WiFi.localIP().toString());
  display.display();
  delay(2000);
  Serial.println("");
}

void readSensors(struct readings *r) {
  #ifdef DONT_HAVE_SENSORS
    readings.temp = random(0, 80);
    readings.hum = random(0, 80);
  #else
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    r->hum = dht.readHumidity();
    // Read temperature as Celsius (the default)
    r->temp = dht.readTemperature();
    // Check if any reads failed and exit early (to try again).
    if (isnan(r->hum) || isnan(r->temp)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }
  #endif
}

void displaySensors(char const *ssid, struct readings r) {
  Serial.print("[INFO] Humidity: ");
  Serial.print(r.hum);
  Serial.println("%");
  Serial.print("[INFO] Temperature: ");
  Serial.print(r.temp);
  Serial.println("°C ");
  //print values to the OLED screen
  display.clear();
  display.setFont(ArialMT_Plain_10);
  //display.drawString(0, 0, "SSID: " + String(ssid) + " - IP: " + WiFi.localIP().toString());
  display.drawString(0, 0, "Temperature: ");
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 10, String(r.temp) + " °C");
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 36, "Humidity: ");
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 46, String(r.hum) + " %");
  display.display();
}

void sendDataToFirebase(struct readings r) {
  String humValueID = Firebase.pushFloat("dht11/humidity", r.hum);
  if (Firebase.failed()) {
    Serial.print("[ERROR] pushing /dht11/humidity failed:");
    Serial.println(Firebase.error());
    return;
  }

  String tempValueID = Firebase.pushFloat("dht11/temperature", r.temp);
  if (Firebase.failed()) {
    Serial.print("[ERROR] pushing /dht11/temperature failed:");
    Serial.println(Firebase.error());
    return;
  }
}