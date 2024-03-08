#include <SPI.h>
#include <WiFiNINA.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <Arduino_LSM6DS3.h>
#include <SparkFun_ADS1015_Arduino_Library.h>
#include <Wire.h>

// WiFi settings
#include "arduino_secrets.h"
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

// Network settings
unsigned int localPort = 7400;
unsigned int sendPort = 7400;


char outIP[] = "MAX.COMPUTER.IP";

// Timing
unsigned long previousMillis = 0;
unsigned long lastUdpMillis = 0;
const long interval = 300;
const long udpInterval = 300;

// WiFi and UDP objects
WiFiUDP Udp;
ADS1015 fingerSensor;

// Flex sensor readings
const int numFlexReadings = 10;
int flexReadings0[numFlexReadings];
int flexReadings1[numFlexReadings];
int flexReadIndex = 0;

void setup() {
  // Initialize Serial and wait for port to open:
  Serial.begin(9600);

  // Initialize IMU
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (true);
  }

  // Initialize WiFi
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    delay(10000);
  }

  Serial.println("Connected to WiFi");
  printWifiStatus();

  Udp.begin(localPort);

  // Initialize flex sensor
  Wire.begin();
  if (fingerSensor.begin() == false) {
    Serial.println("Device not found. Check wiring.");
    while (1);
  }
  fingerSensor.setGain(ADS1015_CONFIG_PGA_TWOTHIRDS); // Adjust gain for flex sensors

  // Initialize sensor readings arrays
  for (int i = 0; i < numFlexReadings; i++) {
    flexReadings0[i] = 0;
    flexReadings1[i] = 0;
  }
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Read accelerometer and gyroscope
    float accX, accY, accZ, gyroX, gyroY, gyroZ;
    if (IMU.accelerationAvailable()) {
      IMU.readAcceleration(accX, accY, accZ);
    }
    if (IMU.gyroscopeAvailable()) {
      IMU.readGyroscope(gyroX, gyroY, gyroZ);
    }

    // Calculate magnitudes
    float accMagnitude = sqrt(accX * accX + accY * accY + accZ * accZ);
    float gyroMagnitude = sqrt(gyroX * gyroX + gyroY * gyroY + gyroZ * gyroZ);

    // Update flex sensor readings
    flexReadings0[flexReadIndex] = fingerSensor.getAnalogData(0);
    flexReadings1[flexReadIndex] = fingerSensor.getAnalogData(1);

    // Calculate average for flex sensors
    float averageFlex0 = calculateAverage(flexReadings0, numFlexReadings);
    float averageFlex1 = calculateAverage(flexReadings1, numFlexReadings);

    // Correctly map the average values to 0-127 range
    uint16_t mappedFlex0 = map(averageFlex0, 1091, 780, 0, 127);
    uint16_t mappedFlex1 = map(averageFlex1, 1100, 830, 0, 127);

    Serial.print("Finger 1: ");
    Serial.print(mappedFlex0);
    Serial.print("\tFinger 2: ");
    Serial.println(mappedFlex1);
    Serial.print("acc: ");
    Serial.print(accMagnitude);
    Serial.print("\tgyro: ");
    Serial.println(gyroMagnitude);

    // Send data via UDP
    if (currentMillis - lastUdpMillis >= udpInterval) {
      OSCMessage msg("/sensorData");
      msg.add(mappedFlex0); // Flex Sensor 0
      msg.add(mappedFlex1); // Flex Sensor 1
      msg.add(accMagnitude);
      msg.add(gyroMagnitude);

      Udp.beginPacket(outIP, sendPort);
      msg.send(Udp);
      Udp.endPacket();
      msg.empty();

      lastUdpMillis = currentMillis;
    }

    flexReadIndex = (flexReadIndex + 1) % numFlexReadings;
  }
}

float calculateAverage(int *arr, int numElements) {
  long sum = 0;
  for (int i = 0; i < numElements; i++) {
    sum += arr[i];
  }
  return (float)sum / numElements;
}

void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
