// PowerMeter 
// with Lolin ESP8266 and INA219, with 7-segment LED display
//
//
// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty

String templateProcessor(const String& var)
{
  if(var == "perc") {
    return String("%");
  } 
  //
  // System values
  //
  if(var == "hostname") {
    return String(config.hostname);
  } 
  if(var == "fw_name") {
    return String(FW_NAME);
  }
  if(var=="fw_version") {
    return String(FW_VERSION);
  }
  if(var=="uptime") {
    return String(millis()/1000);
  }
  if(var=="timedate") {
    return String(timeClient.getFormattedTime());
  }
  //
  // Config values
  //
  if(var=="wifi_essid") {
    return String(config.wifi_essid);
  }
  if(var=="wifi_password") {
    return String(config.wifi_password);
  }
  if(var=="broker_host") {
    return String(config.broker_host);
  }
  if(var=="broker_port") {
    return String(config.broker_port);
  }  
  if(var=="client_id") {
    return String(config.client_id);
  }  
  if(var=="broker_timeout") {
    return String(config.broker_timeout);
  }
  if(var=="auth_username") {
    return String(config.auth_username);
  }  
  if(var=="auth_password") {
    return String(config.auth_password);
  }  
  if(var=="wifi_rssi") {
    return String(WiFi.RSSI());
  }
  if(var=="free_heap") {
    return String(ESP.getFreeHeap());
  }
  return String();
}

// ************************************
// initWebServer
//
// initialize web server
// ************************************
void initWebServer() {
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html").setTemplateProcessor(templateProcessor);
  
  // If present, set authentication
  if(strlen(config.auth_username) > 0) {
    // server.setAuthentication(config.auth_username, config.auth_password);
  }

  server.on("/restart", HTTP_POST, [](AsyncWebServerRequest *request){
    request->redirect("/?restart=ok");
    ESP.restart();
  });

  server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request){
    String message;
    if(request->hasParam("hostname", true)) {
        strcpy(config.hostname,request->getParam("hostname", true)->value().c_str());
    }
    if(request->hasParam("wifi_essid", true)) {
        strcpy(config.wifi_essid,request->getParam("wifi_essid", true)->value().c_str());
    }
    if(request->hasParam("wifi_password", true)) {
        strcpy(config.wifi_password,request->getParam("wifi_password", true)->value().c_str());
    }
    if(request->hasParam("broker_host", true)) {
        strcpy(config.broker_host,request->getParam("broker_host", true)->value().c_str());
    }
    if(request->hasParam("broker_port", true)) {
        config.broker_port = atoi(request->getParam("broker_port", true)->value().c_str());
    }
    if(request->hasParam("client_id", true)) {
        strcpy(config.client_id, request->getParam("client_id", true)->value().c_str());
    }    
    if(request->hasParam("mqtt_timer", true)) {
        config.broker_timeout = atoi(request->getParam("broker_timeout", true)->value().c_str());
    }
    if(request->hasParam("auth_username", true)) {
        strcpy(config.auth_username, request->getParam("auth_username", true)->value().c_str());
    }    
    if(request->hasParam("auth_password", true)) {
        strcpy(config.auth_password, request->getParam("auth_password", true)->value().c_str());
    }    
    saveConfigFile();
    request->redirect("/?result=ok");
  });

  server.on("/ajax", HTTP_POST, [] (AsyncWebServerRequest *request) {
    String action,value,response="";
    char outputJson[256];

    if (request->hasParam("action", true)) {
      action = request->getParam("action", true)->value();
      if(action.equals("env")) {
        serializeJson(env,outputJson);
        response = String(outputJson);
      }
    }
    request->send(200, "text/plain", response);
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.printf("NOT_FOUND: ");
    request->send(404);
  });

  server.begin();
}
