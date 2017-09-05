#include <FS.h>
#include <Arduino.h>
#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino

// Configuration
#include <credentials.h>

// Blynk
#include <BlynkSimpleEsp8266.h>

// GPIO Defines
#define I2C_SDA 5 // D1 Orange
#define I2C_SCL 4 // D2 Yellow

#include <Wire.h>

// Moisture sensor
#include <I2CSoilMoistureSensor.h>

I2CSoilMoistureSensor sensor;

// Handy timers
#include <SimpleTimer.h>

#include <SoftwareSerial.h>

#define DEBUG_SERIAL Serial

// Blynk token
const uint16_t blynk_port{8442};
const char device_id[17] = "Aigro SM1";
const char fw_ver[17] = "v0.0.1";

// Handy timer
SimpleTimer timer;

char *c_str(String s)
{
        char *cstr = new char[s.length() + 1];
        return strcpy(cstr, s.c_str());
}

void printString(String str, uint8_t r = 0, uint8_t c = 0)
{
        char *line{c_str(String(str))};
        DEBUG_SERIAL.println(line);
}

void sendMeasurements()
{
        printString(String(device_id) + " " + String(fw_ver));
        // H/T
        while (sensor.isBusy())
                delay(50);

        auto address = String(sensor.getAddress(), HEX);
        auto sensor_fw_ver = String(sensor.getVersion(), HEX);
        printString("Sensore address: " + address);
        printString("Sensore firmware version: " + sensor_fw_ver);

        auto capacitance = String(sensor.getCapacitance());
        auto temperature = String(sensor.getTemperature() / (float)10);
        auto light = String(sensor.getLight(true));
        printString("Soil Moisture Capacitance: " + capacitance);
        printString("Temperature: " + temperature);
        printString("Light: " + light);

        Blynk.virtualWrite(V1, capacitance);
        Blynk.virtualWrite(V2, temperature);
        Blynk.virtualWrite(V3, light);
}

void setup()
{
        // Init serial ports
        DEBUG_SERIAL.begin(115200);

        // Init I2C interface
        Wire.begin(I2C_SDA, I2C_SCL);

        // Connect WiFi
        Blynk.connectWiFi(WIFI_SSID, WIFI_PASS);

        // Start blynk
        Blynk.config(BLYNK_AUTH, BLYNK_SERVER, blynk_port);

        // Setup a function to be called every 10 second
        timer.setInterval(10000L, sendMeasurements);

        sendMeasurements();
}

void loop()
{
        Blynk.run();
        timer.run();
}
