# Trash Bin Monitoring System

## Overview

This project involves a trash bin monitoring system that uses various sensors to measure the fill level of a trash bin and send the data to Firebase for remote monitoring. The system integrates an ultrasonic sensor, a VL53L0X distance sensor, and a GPS module to provide accurate data on the trash bin's fill level and location.

## Components

- **Arduino Board (ESP32)**: The main controller for the system.
- **Ultrasonic Sensors (HC-SR04)**: Measures the distance to the trash.
- **VL53L0X**: A time-of-flight distance sensor that provides additional measurements.
- **GPS Module (Neo-6M)**: Provides the geographical location of the trash bin.
- **Firebase**: Cloud-based database for storing and retrieving sensor data.
- **WiFi Module**: For connecting to the internet and sending data to Firebase.

## Installation

1. **Hardware Setup**

   - Connect the ultrasonic sensors to the Arduino board:

     - **Sensor 1**:
       - Trig pin: 15
       - Echo pin: 2
     - **Sensor 2**:
       - Trig pin: 18
       - Echo pin: 4

   - Connect the VL53L0X distance sensor to the I2C pins on the Arduino.

   - Connect the GPS module:
     - TX pin to Arduino pin 17
     - RX pin to Arduino pin 16

2. **Software Setup**

   - Install the required libraries via the Arduino Library Manager:

     - `WiFi`
     - `Firebase ESP Client`
     - `Adafruit VL53L0X`
     - `TinyGPS++`

   - Create a new Firebase project and obtain your API key and database URL.

   - Replace `WIFI_SSID`, `WIFI_PASSWORD`, `API_KEY`, and `DATABASE_URL` in the code with your Firebase project details and WiFi credentials.

   - Upload the provided Arduino sketch to your Arduino board.

## Code Description

- **`setup()`**: Initializes the serial communication, connects to WiFi, initializes Firebase, and sets up the sensors.
- **`loop()`**: Continuously reads sensor data, updates GPS information, calculates the average distance, volume, and sends data to Firebase every 15 seconds.
- **`connectToWiFi()`**: Connects to the WiFi network.
- **`initializeFirebase()`**: Sets up the Firebase connection.
- **`initializeSensors()`**: Configures the sensor pins and initializes the sensors.
- **`updateGPS()`**: Reads and updates GPS data.
- **`readDistance()`**: Reads the distance from the ultrasonic sensors.
- **`readVL53L0X()`**: Reads the distance from the VL53L0X sensor.
- **`calculateAverageDistance()`**: Calculates the average distance from multiple sensors.
- **`calculateVolume()`**: Calculates the volume of trash based on the average distance.
- **`printResults()`**: Prints sensor data to the Serial Monitor.
- **`sendDataToFirebase()`**: Sends sensor data to Firebase.

## Usage

- Ensure the hardware is properly connected as described.
- Configure the `README.md` with your Firebase credentials and WiFi details.
- Upload the code to your Arduino board using the Arduino IDE.
- Open the Serial Monitor to view real-time sensor data and status updates.
- Data will be sent to your Firebase Realtime Database every 15 seconds.

## Troubleshooting

- **Connection Issues**: Ensure that your WiFi credentials and Firebase configuration are correct.
- **Sensor Readings**: Verify that all sensors are properly connected and functioning. Check the wiring and sensor placement.
- **Firebase Errors**: Check the Firebase console for any configuration issues or quota limits.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [Adafruit VL53L0X Library](https://github.com/adafruit/Adafruit_VL53L0X)
- [TinyGPS++ Library](https://github.com/mikalhart/TinyGPSPlus)
- [Firebase ESP Client](https://github.com/mobizt/Firebase-ESP-Client)
