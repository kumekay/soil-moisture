#include <FS.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

// Wifi Manager
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

// HTTP requests
#include <ESP8266HTTPClient.h>

// OTA updates
#include <ESP8266httpUpdate.h>
// Blynk
#include <BlynkSimpleEsp8266.h>


// JSON
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson


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
char blynk_token[33] {"Blynk token"};
const char blynk_domain[] {"ezagro.kumekay.com"};
const uint16_t blynk_port {8442};

// Device Id
char device_id[17] = "Aigro SM1";
char fw_ver[17] = "v0.0.1";

// Handy timer
SimpleTimer timer;

// Setup Wifi connection
WiFiManager wifiManager;

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying the need to save config
void saveConfigCallback() {
        DEBUG_SERIAL.println("Should save config");
        shouldSaveConfig = true;
}

void factoryReset() {
        wifiManager.resetSettings();
        SPIFFS.format();
        ESP.reset();
}


char *c_str(String s) {
  char *cstr = new char[s.length() + 1];
  return strcpy(cstr, s.c_str());
}

void printString(String str,  uint8_t r=0, uint8_t c=0) {
  char *line { c_str(String(str))};
  DEBUG_SERIAL.println(line);
}

void sendMeasurements() {
        printString(String(device_id) + " " + String(fw_ver));
        // H/T
        while (sensor.isBusy()) delay(50);

        auto address = String(sensor.getAddress(),HEX);
        auto sensor_fw_ver = String(sensor.getVersion(),HEX);
        printString("Sensore address: " + address);
        printString("Sensore firmware version: " + sensor_fw_ver);

        auto capacitance = String(sensor.getCapacitance());
        auto temperature = String(sensor.getTemperature()/(float)10);
        auto light = String(sensor.getLight(true));
        printString("Soil Moisture Capacitance: " + capacitance);
        printString("Temperature: " + temperature);
        printString("Light: " + light);

        Blynk.virtualWrite(V1, capacitance);
        Blynk.virtualWrite(V2, temperature);
        Blynk.virtualWrite(V3, light);
}

bool loadConfig() {
        File configFile = SPIFFS.open("/config.json", "r");
        if (!configFile) {
                DEBUG_SERIAL.println("Failed to open config file");
                return false;
        }

        size_t size = configFile.size();
        if (size > 1024) {
                DEBUG_SERIAL.println("Config file size is too large");
                return false;
        }

        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        // We don't use String here because ArduinoJson library requires the input
        // buffer to be mutable. If you don't use ArduinoJson, you may as well
        // use configFile.readString instead.
        configFile.readBytes(buf.get(), size);

        StaticJsonBuffer<200> jsonBuffer;
        JsonObject &json = jsonBuffer.parseObject(buf.get());

        if (!json.success()) {
                DEBUG_SERIAL.println("Failed to parse config file");
                return false;
        }

        // Save parameters
        strcpy(device_id, json["device_id"]);
        strcpy(blynk_token, json["blynk_token"]);
}

void setupWiFi() {
        //set config save notify callback
        wifiManager.setSaveConfigCallback(saveConfigCallback);

        // Custom parameters
        WiFiManagerParameter custom_device_id("device_id", "Device ID", device_id, 16);
        WiFiManagerParameter custom_blynk_token("blynk", "Blynk token", blynk_token, 34);
        wifiManager.addParameter(&custom_blynk_token);
        wifiManager.addParameter(&custom_device_id);

        // wifiManager.setTimeout(180);

        String ssid { "aigro" +  String(ESP.getChipId())};
        String pass {"aigro" + String(ESP.getFlashChipId()) };

        printString("Connect to WiFi:",1);
        printString("net: " + ssid,2);
        printString("pw: "+ pass,3);
        printString("Open browser:", 4);
        printString("//192.168.4.1", 5);
        printString("to setup device", 6);

        wifiManager.setTimeout(180);

        // if (!wifiManager.autoConnect(ssid.c_str(), pass.c_str())) {
          if (!wifiManager.autoConnect(ssid.c_str(), pass.c_str())) {
                DEBUG_SERIAL.println("failed to connect and hit timeout");
        }

        //save the custom parameters to FS
        if (shouldSaveConfig) {
                DEBUG_SERIAL.println("saving config");
                DynamicJsonBuffer jsonBuffer;
                JsonObject &json = jsonBuffer.createObject();
                json["device_id"] = custom_device_id.getValue();
                json["blynk_token"] = custom_blynk_token.getValue();

                File configFile = SPIFFS.open("/config.json", "w");
                if (!configFile) {
                        DEBUG_SERIAL.println("failed to open config file for writing");
                }

                json.printTo(DEBUG_SERIAL);
                json.printTo(configFile);
                configFile.close();
                //end save
        }

        //if you get here you have connected to the WiFi
        DEBUG_SERIAL.println("WiFi connected");

        DEBUG_SERIAL.print("IP address: ");
        DEBUG_SERIAL.println(WiFi.localIP());
}

void wifiModeCallback(WiFiManager *myWiFiManager) {
        DEBUG_SERIAL.println("Entered config mode");
        DEBUG_SERIAL.println(WiFi.softAPIP());
}

// Virtual pin update FW
BLYNK_WRITE(V22) {
        if (param.asInt() == 1) {
                DEBUG_SERIAL.println("Got a FW update request");

                char full_version[34] {""};
                strcat(full_version, device_id);
                strcat(full_version, "::");
                strcat(full_version, fw_ver);

                t_httpUpdate_return ret = ESPhttpUpdate.update("http://romfrom.space/get", full_version);
                switch (ret) {
                case HTTP_UPDATE_FAILED:
                        DEBUG_SERIAL.println("[update] Update failed.");
                        break;
                case HTTP_UPDATE_NO_UPDATES:
                        DEBUG_SERIAL.println("[update] Update no Update.");
                        break;
                case HTTP_UPDATE_OK:
                        DEBUG_SERIAL.println("[update] Update ok.");
                        break;
                }

        }
}

// Virtual pin reset settings
BLYNK_WRITE(V23) {
        factoryReset();
}

void setup() {
        // Init serial ports
        DEBUG_SERIAL.begin(115200);

        // Init I2C interface
        Wire.begin(I2C_SDA, I2C_SCL);

        // Init filesystem
        if (!SPIFFS.begin()) {
                DEBUG_SERIAL.println("Failed to mount file system");
                ESP.reset();
        }

        // Setup WiFi
        setupWiFi();

        // Load config
        if (!loadConfig()) {
                DEBUG_SERIAL.println("Failed to load config");
                factoryReset();
        } else {
                DEBUG_SERIAL.println("Config loaded");
        }

        // Start blynk
        Blynk.config(blynk_token, blynk_domain, blynk_port);

        // Setup a function to be called every 10 second
        timer.setInterval(10000L, sendMeasurements);

        sendMeasurements();
}

void loop() {
        Blynk.run();
        timer.run();
}
