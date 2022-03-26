// PowerMeter 
// with Lolin ESP8266 and INA219, with 7-segment LED display
//
//
// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty
// 
// Program using: WeMos D1 mini Module - 4MB flash size (FS 2MB, OTA 1MB)
//
// v0.0.1 - 26.03.2022
// - first release 

// Enable diagnostic messages to serial
#define __DEBUG__

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ESP8266httpUpdate.h>
#include <EEPROM.h>

WiFiUDP udpClient;

// Task Scheduler
#include <TaskScheduler.h>
Scheduler runner;

#define MINUTE 60000
#define SECOND 1000

#define MQTT_INTERVAL MINUTE*15 // Every 15 minutes send MQTT frames with environmental values
void mqttCallback();
Task mqttTask(MQTT_INTERVAL, TASK_FOREVER, &mqttCallback);

#define INA_INTERVAL SECOND*5 // Every 5 seconds
void inaCallback();
Task inaTask(INA_INTERVAL, TASK_FOREVER, &inaCallback);

#define DISPLAY_INTERVAL SECOND*5 // Every 5 seconds
void displayCallback();
Task displayTask(DISPLAY_INTERVAL, TASK_FOREVER, &displayCallback);

// Arduino Json
#include <ArduinoJson.h>

// NTP ClientLib 
// https://github.com/taranais/NTPClient
#include <NTPClient.h>
#include <WiFiUdp.h>

#define NTPSERVER "time.ien.it"
#define NTPTZ 3600

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP,NTPSERVER,NTPTZ);

// MQTT PubSub client
// https://github.com/knolleary/pubsubclient
#include <PubSubClient.h>

WiFiClient wifiClient;
PubSubClient client(wifiClient);

#include <Wire.h>

// File System
#include <FS.h>   
// #include <SPIFFS.h> // SPIFFS

#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define SDA D1 // GPIO5 D1
#define SCL D2 // GPIO4 D2

// TM1637 7-segment LED display
// https://www.arduino.cc/reference/en/libraries/tm1637/
#include <TM1637Display.h>

#define CLK D5 // GPIO14 D5
#define DIO D6 // GPIO12 D6

TM1637Display display(CLK, DIO);

//INA219
// 
#include <Adafruit_INA219.h>
Adafruit_INA219 ina219;


// Config
struct Config {
  // WiFi config
  char wifi_essid[16];
  char wifi_password[16];
  // MQTT config
  char broker_host[32];
  unsigned int broker_port;
  char client_id[18];
  unsigned int broker_timeout;
  // Host config
  char hostname[16];
  char auth_username[16];
  char auth_password[16];
};

#define CONFIG_FILE "/config.json"

File configFile;
Config config; // Global config object

// Format SPIFFS if mount failed
#define FORMAT_SPIFFS_IF_FAILED 1

// Firmware data
const char BUILD[] = __DATE__ " " __TIME__;
#define FW_NAME         "power-meter"
#define FW_VERSION      "0.0.1"

// Web server
AsyncWebServer server(80);

DynamicJsonDocument env(256);

unsigned long last;

// ************************************
// DEBUG_PRINT(message)
//
// ************************************
void DEBUG_PRINT(String message) {
#ifdef __DEBUG__
  Serial.println(message);    
#endif
}

// ************************************
// connectToWifi()
//
// ************************************
bool connectToWifi() {
  uint8_t timeout=0;

  if(strlen(config.wifi_essid) > 0) {
    DEBUG_PRINT("[DEBUG] Connecting to "+String(config.wifi_essid));

    WiFi.mode(WIFI_STA);
    WiFi.begin(config.wifi_essid, config.wifi_password);
  
    while((WiFi.status() != WL_CONNECTED)&&(timeout < 10)) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(250);
      DEBUG_PRINT(".");
      digitalWrite(LED_BUILTIN, HIGH);
      delay(250);
      timeout++;
    }
    if(WiFi.status() == WL_CONNECTED) {
      DEBUG_PRINT("OK. IP:"+WiFi.localIP().toString());
      if (MDNS.begin(config.hostname)) {
        DEBUG_PRINT("[DEBUG] MDNS responder started");
        // Add service to MDNS-SD
        MDNS.addService("http", "tcp", 80);
      }
      
      // NTP    
      timeClient.begin();
            
      return true;  
    } else {
      DEBUG_PRINT("[ERROR] Failed to connect to WiFi");
      return false;
    }
  } else {
    DEBUG_PRINT("[ERROR] Please configure Wifi");
    return false; 
  }
}

// *******************************
// Setup
//
// Boot-up routine: setup hardware, display, sensors and connect to wifi...
// *******************************
void setup() {
  Serial.begin(115200);
  delay(10);

  // print firmware and build data
  Serial.println();
  Serial.println();
  Serial.print(FW_NAME);  
  Serial.print(" ");
  Serial.print(FW_VERSION);
  Serial.print(" ");  
  Serial.println(BUILD);

  // Initialize SPIFFS
  if(!SPIFFS.begin()){
    DEBUG_PRINT("[ERROR] SPIFFS Mount Failed. Try formatting...");
    if(SPIFFS.format()) {
      DEBUG_PRINT("[INIT] SPIFFS initialized successfully");
    } else {
      DEBUG_PRINT("[FATAL] SPIFFS error");
      ESP.restart();
    }
  } else {
    DEBUG_PRINT("[INIT] SPIFFS done");
  }

  uint8_t data[] = { 0xff, 0xff, 0xff, 0xff };
  display.setBrightness(0x0f);
  // Run through all the dots
  for(uint8_t k=0; k <= 4; k++) {
    display.showNumberDecEx(0, (0x80 >> k), true);
    delay(500);
  }

  // Load configuration
  loadConfigFile();

  // Connect to WiFI network and sync time
  connectToWifi();
      
  // Setup I2C...
  DEBUG_PRINT("[INIT] Initializing I2C bus...");
  Wire.begin(SDA, SCL);
  Wire.setClock(100000);
  
  i2c_status();
  i2c_scanner();

  if(!ina219.begin()) {
    DEBUG_PRINT("[ERROR] Failed to find INA219 chip");
  }

  // Change INA219 calibration settings
  //ina219.setCalibration_32V_1A();
  //ina219.setCalibration_16V_400mA();
  
  // Setup MQTT connection
  client.setServer(config.broker_host, config.broker_port);
  client.setCallback(mqttInCallback); 

  // Setup OTA
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    DEBUG_PRINT("[OTA] Progress: "+String(progress/(total/100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    if(error == OTA_AUTH_ERROR) {
       DEBUG_PRINT("[OTA] Auth Failed");
    }
    else if(error == OTA_BEGIN_ERROR) DEBUG_PRINT("[OTA] Begin Failed");
    else if(error == OTA_CONNECT_ERROR) DEBUG_PRINT("[OTA] Connect Failed");
    else if(error == OTA_RECEIVE_ERROR) DEBUG_PRINT("[OTA] Receive Failed");
    else if(error == OTA_END_ERROR) DEBUG_PRINT("[OTA] End Failed");
  });
  ArduinoOTA.setHostname(config.hostname);
  ArduinoOTA.begin();

  // Setup runner tasks
  runner.addTask(mqttTask);
  mqttTask.setInterval(config.broker_timeout*MINUTE);
  mqttTask.enable();

  // Add INA task
  runner.addTask(inaTask);
  inaTask.enable();

  // Add display task
  runner.addTask(displayTask);
  displayTask.enable();

  env["status"] = "System running";

  // Initialize web server on port 80
  initWebServer();

  DEBUG_PRINT("Run!");
}

// *******************************
// INA data fetched
//
void inaCallback() {
  // Read voltage and current from INA219.
  float shuntvoltage = ina219.getShuntVoltage_mV();
  float busvoltage = ina219.getBusVoltage_V();
  float current_mA = ina219.getCurrent_mA();

  env["shuntvoltage"] = shuntvoltage;
  env["busvoltage"] = busvoltage;
  env["current_mA"] = current_mA;

  // Compute load voltage, power, and milliamp-hours.
  float loadvoltage = busvoltage + (shuntvoltage / 1000);
  float power_mW = loadvoltage * current_mA;

  env["loadvoltage"] = loadvoltage;
  env["power_mW"] = power_mW;

  float total_mA = env["total_mA"];
  total_mA += current_mA;
  
  long total_sec = env["total_sec"];
  total_sec += 5;
  env["total_sec"] = total_sec;
  
  float total_mAH = total_mA / 3600.0;
  env["total_mAH"] = total_mAH;
  env["total_mA"] = total_mA;
}


// *******************************
// Display data
//

uint8_t display_step=0;

void displayCallback() {
  display.clear();

  display_step++;

  switch(display_step) {
    case 1:
      // Display voltage
      display.showNumberDec(env["busvoltage"], false);
      break;
    case 2:
      // Display current
      display.showNumberDec(env["current_mA"], false);
      break;
    case 3:
      // Display power
      display.showNumberDec(env["power_mW"], false);
      break;
    case 4:
      // Display total mAH
      display.showNumberDec(env["total_mAH"], false);
      break;
    default:
      display.showNumberDec(env["total_sec"], false);
      display_step=0;
      break;
  }
}

// ************************************
// Loop()
//
// ************************************
void loop() {
  // Handle OTA
  ArduinoOTA.handle();

  // Scheduler
  runner.execute();

  if((millis() - last) > 5100) {              
    if(WiFi.status() != WL_CONNECTED) {
      connectToWifi();
    }
    
    // NTP Sync
    timeClient.update();
    
    // Work done!
    env["uptime"] = millis() / 1000;
    last = millis();
  }
}
