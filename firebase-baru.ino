#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <SoftwareSerial.h>
#include <Adafruit_VL53L0X.h>
#include <TinyGPS++.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID "kontrasepsi"
#define WIFI_PASSWORD "heriyanto"
#define API_KEY "AIzaSyDO-Z56bvNKh_HzSvdzOXK2NWpUMBqPLRA"
#define DATABASE_URL "https://gotrash-ec9f3-default-rtdb.firebaseio.com/"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;

int trig1 = 15;
int echo1 = 2;
int trig2 = 18;
int echo2 = 4;

long echoTime1;
long echoTime2;
Adafruit_VL53L0X lox = Adafruit_VL53L0X();
TinyGPSPlus gps; 
SoftwareSerial mygps(17, 16);

float binHeight = 15.00;
float baseArea = 400.00;

void setup() {
  Serial.begin(9600);
  mygps.begin(9600);
  connectToWiFi();
  initializeFirebase();
  initializeSensors();
}

void loop() {
  updateGPS();
  float dist1 = readDistance(trig1, echo1);
  float dist2 = readDistance(trig2, echo2);
  float distVL53L0X = readVL53L0X();
  float distAvg = calculateAverageDistance(dist1, dist2, distVL53L0X);
  float volume = calculateVolume(distAvg);
  float latitude = gps.location.lat();
  float longitude = gps.location.lng();

  printResults(dist1, dist2, distVL53L0X, distAvg, volume, binHeight, latitude, longitude);

  if (Firebase.ready() && millis() - sendDataPrevMillis > 15000) {
    sendDataPrevMillis = millis();
    sendDataToFirebase(dist1, dist2, distVL53L0X, distAvg, volume, latitude, longitude);
  }
}

void connectToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP : ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void initializeFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase sign up successful");
  } else {
    Serial.printf("Firebase sign up failed: %s\n", config.signer.signupError.message.c_str());
  }
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void initializeSensors() {
  pinMode(trig1, OUTPUT);
  pinMode(echo1, INPUT);
  pinMode(trig2, OUTPUT);
  pinMode(echo2, INPUT);
  
  digitalWrite(trig1, LOW);
  digitalWrite(trig2, LOW);
  
  Wire.begin();
  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while (1);
  }
  Serial.println(F("VL53L0X API Simple Ranging example\n\n"));
}

void updateGPS() {
  while (mygps.available() > 0) {
    gps.encode(mygps.read());
    if (gps.location.isUpdated()) {
      Serial.print("Latitude= ");
      Serial.print(gps.location.lat(), 6);
      Serial.print(" Longitude= ");
      Serial.println(gps.location.lng(), 6);
    }
  }
}

float readDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long echoTime = pulseIn(echoPin, HIGH, 30000);
  if (echoTime > 0) {
    float distance = (0.034 * (float)echoTime) / 2;
    return distance > 15 ? 15 : distance;
  } else {
    return 15;
  }
}

float readVL53L0X() {
  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false);
  return measure.RangeStatus != 4 ? measure.RangeMilliMeter / 10.0 : 15;
}

float calculateAverageDistance(float dist1, float dist2, float distVL53L0X) {
  if (dist1 >= 0 && dist2 >= 0 && distVL53L0X >= 0) {
    return (dist1 + dist2 + distVL53L0X) / 3;
  } else if (dist1 >= 0 && dist2 >= 0) {
    return (dist1 + dist2) / 2;
  } else if (dist1 >= 0 && distVL53L0X >= 0) {
    return (dist1 + distVL53L0X) / 2;
  } else if (dist2 >= 0 && distVL53L0X >= 0) {
    return (dist2 + distVL53L0X) / 2;
  } else if (dist1 >= 0) {
    return dist1;
  } else if (dist2 >= 0) {
    return dist2;
  } else if (distVL53L0X >= 0) {
    return distVL53L0X;
  } else {
    return -1; // Invalid measurement
  }
}

float calculateVolume(float distAvg) {
  return distAvg >= 0 ? baseArea * (binHeight - distAvg) : -1;
}

void printResults(float dist1, float dist2, float distVL53L0X, float distAvg, float volume, float binHeight, float latitude, float longitude) {
  Serial.print("Distance Sensor 1 = ");
  Serial.print(dist1);
  Serial.println(" cm");

  Serial.print("Distance Sensor 2 = ");
  Serial.print(dist2);
  Serial.println(" cm");

  Serial.print("Distance Sensor VL53L0X = ");
  Serial.print(distVL53L0X);
  Serial.println(" cm");

  Serial.print("Average Surface Distance = ");
  Serial.print(distAvg);
  Serial.println(" cm");

  Serial.print("Trash Height = ");
  Serial.print(binHeight - distAvg);
  Serial.println(" cm");

  Serial.print("Volume = ");
  Serial.print(volume);
  Serial.println(" cm³");

  Serial.print("Latitude = ");
  Serial.print(latitude, 6);
  Serial.println();

  Serial.print("Longitude = ");
  Serial.print(longitude, 6);
  Serial.println();

  Serial.println("==========================================");
}

void sendDataToFirebase(float dist1, float dist2, float distVL53L0X, float distAvg, float volume, float latitude, float longitude) {
  if (Firebase.RTDB.setInt(&fbdo, "tps-data/dist1", dist1)) {
    Serial.println("Data sent: dist1");
  } else {
    Serial.println("Failed to send dist1: " + fbdo.errorReason());
  }
  
  if (Firebase.RTDB.setInt(&fbdo, "tps-data/dist2", dist2)) {
    Serial.println("Data sent: dist2");
  } else {
    Serial.println("Failed to send dist2: " + fbdo.errorReason());
  }
  
  if (Firebase.RTDB.setInt(&fbdo, "tps-data/distvl53l0x", distVL53L0X)) {
    Serial.println("Data sent: distvl53l0x");
  } else {
    Serial.println("Failed to send distvl53l0x: " + fbdo.errorReason());
  }
  
  if (Firebase.RTDB.setInt(&fbdo, "tps-data/distavg", distAvg)) {
    Serial.println("Data sent: distavg");
  } else {
    Serial.println("Failed to send distavg: " + fbdo.errorReason());
  }
  
  if (Firebase.RTDB.setInt(&fbdo, "tps-data/volume", volume)) {
    Serial.println("Data sent: volume");
  } else {
    Serial.println("Failed to send volume: " + fbdo.errorReason());
  }
  
  if (Firebase.RTDB.setFloat(&fbdo, "tps-data/latitude", latitude)) {
    Serial.println("Data sent: latitude");
  } else {
    Serial.println("Failed to send latitude: " + fbdo.errorReason());
  }

  if (Firebase.RTDB.setFloat(&fbdo, "tps-data/longitude", longitude)) {
    Serial.println("Data sent: longitude");
  } else {
    Serial.println("Failed to send longitude: " + fbdo.errorReason());
  }
}