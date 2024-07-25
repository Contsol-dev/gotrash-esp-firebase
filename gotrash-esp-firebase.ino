#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <SoftwareSerial.h>
#include <Adafruit_VL53L0X.h>
#include <TinyGPS++.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Konfigurasi koneksi WiFi
#define WIFI_SSID "kontrasepsi"
#define WIFI_PASSWORD "heriyanto"

// Konfigurasi Firebase
#define API_KEY "AIzaSyDO-Z56bvNKh_HzSvdzOXK2NWpUMBqPLRA"
#define DATABASE_URL "https://gotrash-ec9f3-default-rtdb.firebaseio.com/"

// Objek Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variabel untuk pengiriman data ke Firebase
unsigned long sendDataPrevMillis = 0;

// Pin untuk sensor jarak ultrasonik
int trig1 = 15;
int echo1 = 2;
int trig2 = 18;
int echo2 = 4;

// Variabel untuk menyimpan hasil pengukuran jarak
long echoTime1;
long echoTime2;

// Objek sensor jarak dan GPS
Adafruit_VL53L0X lox = Adafruit_VL53L0X();
TinyGPSPlus gps;
SoftwareSerial mygps(17, 16);

// Konstanta untuk perhitungan volume
float binHeight = 15.00;
float baseArea = 400.00;

void setup() {
  Serial.begin(9600);           // Inisialisasi komunikasi serial
  mygps.begin(9600);            // Inisialisasi komunikasi GPS
  connectToWiFi();              // Menghubungkan ke jaringan WiFi
  initializeFirebase();         // Menginisialisasi koneksi Firebase
  initializeSensors();         // Menginisialisasi sensor
}

void loop() {
  updateGPS();                  // Memperbarui data GPS
  float dist1 = readDistance(trig1, echo1);          // Membaca jarak dari sensor ultrasonik 1
  float dist2 = readDistance(trig2, echo2);          // Membaca jarak dari sensor ultrasonik 2
  float distVL53L0X = readVL53L0X();                  // Membaca jarak dari sensor VL53L0X
  float distAvg = calculateAverageDistance(dist1, dist2, distVL53L0X);  // Menghitung jarak rata-rata
  float volume = calculateVolume(distAvg);            // Menghitung volume sampah
  float latitude = gps.location.lat();                 // Mendapatkan latitude dari GPS
  float longitude = gps.location.lng();                // Mendapatkan longitude dari GPS

  printResults(dist1, dist2, distVL53L0X, distAvg, volume, binHeight, latitude, longitude);  // Mencetak hasil pengukuran

  // Mengirim data ke Firebase setiap 15 detik
  if (Firebase.ready() && millis() - sendDataPrevMillis > 15000) {
    sendDataPrevMillis = millis();
    sendDataToFirebase(dist1, dist2, distVL53L0X, distAvg, volume, latitude, longitude);
  }
}

// Fungsi untuk menghubungkan ke WiFi
void connectToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);  // Memulai koneksi WiFi
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");  // Menampilkan status koneksi
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP : ");
  Serial.println(WiFi.localIP());  // Menampilkan IP yang diperoleh
  Serial.println();
}

// Fungsi untuk menginisialisasi Firebase
void initializeFirebase() {
  config.api_key = API_KEY;  // Menetapkan API key
  config.database_url = DATABASE_URL;  // Menetapkan URL database
  if (Firebase.signUp(&config, &auth, "", "")) {  // Mendaftar dengan Firebase
    Serial.println("Firebase sign up successful");
  } else {
    Serial.printf("Firebase sign up failed: %s\n", config.signer.signupError.message.c_str());
  }
  config.token_status_callback = tokenStatusCallback;  // Callback status token
  Firebase.begin(&config, &auth);  // Memulai koneksi Firebase
  Firebase.reconnectWiFi(true);  // Mengatur koneksi WiFi otomatis
}

// Fungsi untuk menginisialisasi sensor
void initializeSensors() {
  pinMode(trig1, OUTPUT);  // Mengatur pin trig1 sebagai output
  pinMode(echo1, INPUT);   // Mengatur pin echo1 sebagai input
  pinMode(trig2, OUTPUT);  // Mengatur pin trig2 sebagai output
  pinMode(echo2, INPUT);   // Mengatur pin echo2 sebagai input
  digitalWrite(trig1, LOW);  // Menetapkan nilai awal trig1
  digitalWrite(trig2, LOW);  // Menetapkan nilai awal trig2
  Wire.begin();             // Memulai komunikasi I2C
  if (!lox.begin()) {       // Memeriksa keberhasilan inisialisasi sensor VL53L0X
    Serial.println(F("Failed to boot VL53L0X"));
    while (1);  // Menghentikan eksekusi jika gagal
  }
  Serial.println(F("VL53L0X API Simple Ranging example\n\n"));
}

// Fungsi untuk memperbarui data GPS
void updateGPS() {
  while (mygps.available() > 0) {
    gps.encode(mygps.read());  // Membaca data GPS
    if (gps.location.isUpdated()) {  // Memeriksa apakah data lokasi diperbarui
      Serial.print("Latitude= ");
      Serial.print(gps.location.lat(), 6);  // Menampilkan latitude
      Serial.print(" Longitude= ");
      Serial.println(gps.location.lng(), 6);  // Menampilkan longitude
    }
  }
}

// Fungsi untuk membaca jarak dari sensor ultrasonik
float readDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, HIGH);  // Mengaktifkan sinyal trigger
  delayMicroseconds(10);        // Menunggu
  digitalWrite(trigPin, LOW);   // Menonaktifkan sinyal trigger
  long echoTime = pulseIn(echoPin, HIGH, 30000);  // Mengukur waktu pulsa
  if (echoTime > 0) {
    float distance = (0.034 * (float)echoTime) / 2;  // Menghitung jarak
    return distance > 15 ? 15 : distance;  // Membatasi jarak maksimum
  } else {
    return 15;  // Mengembalikan jarak default jika gagal
  }
}

// Fungsi untuk membaca jarak dari sensor VL53L0X
float readVL53L0X() {
  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false);  // Mengambil data pengukuran
  return measure.RangeStatus != 4 ? measure.RangeMilliMeter / 10.0 : 15;  // Mengembalikan jarak atau nilai default
}

// Fungsi untuk menghitung jarak rata-rata dari beberapa sensor
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
    return -1;  // Jarak tidak valid
  }
}

// Fungsi untuk menghitung volume sampah berdasarkan jarak rata-rata
float calculateVolume(float distAvg) {
  return distAvg >= 0 ? baseArea * (binHeight - distAvg) : -1;  // Menghitung volume
}

// Fungsi untuk mencetak hasil pengukuran ke Serial Monitor
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

// Fungsi untuk mengirim data ke Firebase
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
