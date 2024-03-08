#include <SPI.h>
#include <WiFiNINA.h>
#include <WiFiUdp.h>
#include <Arduino_LSM6DS3.h>

// WiFi settings
#include "arduino_secrets.h"
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

// Network settings
unsigned int localPort = 2390;      
unsigned int sendPort = 7403;       
char outIP[] = "TD.COMPUTER.IP"; 

WiFiUDP Udp;

unsigned long previousMillis = 0;
const long interval = 200;  // Interval at which to send data (milliseconds)

void setup() {
  Serial.begin(9600);
  //while (!Serial) {
   // ; // Wait for serial port to connect. Needed for native USB port only
  //}

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

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (true);
  }
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float x, y, z;
    float accX, accY;

    // Read accelerometer (only X and Y)
    if (IMU.accelerationAvailable()) {
      IMU.readAcceleration(accX, accY, z);
    }

    // Read gyroscope and calculate magnitude
    if (IMU.gyroscopeAvailable()) {
      IMU.readGyroscope(x, y, z);
    }
    float gyroMagnitude = sqrt(x * x + y * y + z * z);

    // Print data to Serial
    Serial.print("Accel - X: ");
    Serial.print(accX);
    Serial.print(", Y: ");
    Serial.print(accY);
    Serial.print(" / Gyro Mag: ");
    Serial.println(gyroMagnitude);

    // Send data via UDP
    Udp.beginPacket(outIP, sendPort);
    Udp.print(accX);
    Udp.print(",");
    Udp.print(accY);
    Udp.print("/");
    Udp.println(gyroMagnitude);
    Udp.endPacket();
  }
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
