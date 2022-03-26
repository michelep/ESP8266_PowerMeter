// PowerMeter 
// with Lolin ESP8266 and INA219, with 7-segment LED display
//
//
// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty

// MQTT PubSub client
// https://github.com/knolleary/pubsubclient
#include <PubSubClient.h>

// ************************************
// mqttCallback()
//
//
// ************************************
void mqttInCallback(char* topic, unsigned char* payload, unsigned int length) {
  DEBUG_PRINT("Message arrived: "+String(topic));
}

// ************************************
// mqttDisconnect()
//
//
// ************************************
void mqttDisconnect() {
  if (!client.connected()) {
    return;
  }
  client.disconnect();
}

// ************************************
// mqttConnect()
//
//
// ************************************
bool mqttConnect() {
  uint8_t timeout=10;
  if (client.connected()) {
    return true;
  }
  DEBUG_PRINT("[MQTT] Attempting connection to "+String(config.broker_host)+":"+String(config.broker_port));
  while((!client.connected())&&(timeout > 0)) {
    // Attempt to connect
    if (client.connect(config.client_id)) {
      DEBUG_PRINT("[MQTT] Connected as "+String(config.client_id));
      return true;
    } else {
      timeout--;
      delay(500);
    } 
  }
  return false;
}

// ************************************
// mqttCallback()
//
//
// ************************************
void mqttCallback() {
  char topic[32];
  DEBUG_PRINT("[DEBUG] MQTT send data");
  // First check for connection
  if(WiFi.status() != WL_CONNECTED) {
    connectToWifi();
  }
  // MQTT not connected? Connect or return false;
  if (!client.connected()) {
    if(!mqttConnect()) {
      // MQTT connection failed or disabled
      return;
    }
  }
  // Update environment value  
  env["uptime"] = millis() / 1000;

  JsonObject root = env.as<JsonObject>();
 
  for (JsonPair keyValue : root) {
    // Don't publish keys that starts with _
    String value = keyValue.value().as<String>();
    if(keyValue.key().c_str()[0] != '_') {
      sprintf(topic,"%s/%s",config.client_id,keyValue.key().c_str());
      if(client.publish(topic,value.c_str())) {
        DEBUG_PRINT("[MQTT] Published "+String(topic)+":"+value);
      } else {
        DEBUG_PRINT("[MQTT] Publish failed!");
      }
    }
  }
}
