// PowerMeter 
// with Lolin ESP8266 and INA219, with 7-segment LED display
//
//
// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty

#include <ArduinoJson.h>

// ************************************
// Config, save and load functions
//
// save and load configuration from config file in SPIFFS. JSON format (need ArduinoJson library)
// ************************************
bool loadConfigFile() {
  DynamicJsonDocument root(512);
  
  DEBUG_PRINT("[DEBUG] loadConfigFile()");

  configFile = SPIFFS.open(CONFIG_FILE, "r");
  if (!configFile) {
    DEBUG_PRINT("[CONFIG] Config file not available");
    return false;
  } else {
    // Get the root object in the document
    DeserializationError err = deserializeJson(root, configFile);
    if (err) {
      DEBUG_PRINT("[CONFIG] Failed to read config file:"+String(err.c_str()));
      return false;
    } else {
      strlcpy(config.wifi_essid, root["wifi_essid"], sizeof(config.wifi_essid));
      strlcpy(config.wifi_password, root["wifi_password"], sizeof(config.wifi_password));
      strlcpy(config.hostname, root["hostname"] | "aiq-sensor", sizeof(config.hostname));
      
      strlcpy(config.broker_host, root["broker_host"], sizeof(config.broker_host));
      config.broker_port = root["broker_port"] | 1883;
      strlcpy(config.client_id, root["client_id"], sizeof(config.client_id));
      config.broker_timeout = root["broker_timeout"] | 3600;
      
      strlcpy(config.auth_username, root["auth_username"] | "admin", sizeof(config.auth_username));
      strlcpy(config.auth_password, root["auth_password"] | "admin", sizeof(config.auth_password));
      
      DEBUG_PRINT("[INIT] Configuration loaded");
    }
  }
  configFile.close();
  return true;
}

bool saveConfigFile() {
  DynamicJsonDocument root(512);
  DEBUG_PRINT("[DEBUG] saveConfigFile()");

  root["wifi_essid"] = config.wifi_essid;
  root["wifi_password"] = config.wifi_password;
  root["hostname"] = config.hostname;
  root["broker_host"] = config.broker_host;
  root["broker_port"] = config.broker_port;
  root["client_id"] = config.client_id;
  root["broker_timeout"] = config.broker_timeout;
  root["auth_username"] = config.auth_username;
  root["auth_password"] = config.auth_password;

  configFile = SPIFFS.open(CONFIG_FILE, "w");
  if(!configFile) {
    DEBUG_PRINT("[CONFIG] Failed to create config file !");
    return false;
  }
  serializeJson(root,configFile);
  configFile.close();
    
  return true;
}
