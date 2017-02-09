#include "ThingCore.h"

ThingCore::ThingCore(String app_name, String app_version) :
                            _webSockets(TC_PORTWS), _httpServer(TC_PORTHTTP) {

  _mqttClient = PubSubClient(_espClient);

  _appName = app_name;
  _appVersion = app_version;
  _pubSubListenTopic = "";

  conf_devicename = _appName + "-" + String(ESP.getChipId());
  conf_devicename.toLowerCase();

  conf_webupdatepath = TC_DEFAULT_UPDATEURI;
  conf_webuser = TC_DEFAULT_WEBUSER;
  conf_webpass = TC_DEFAULT_WEBPASS;
  conf_mqttserver = TC_DEFAULT_MQTTSERVER;
  conf_mqttport = TC_DEFAULT_MQTTPORT;
  conf_mqttuser = TC_DEFAULT_MQTTUSER;
  conf_mqttpass = TC_DEFAULT_MQTTPASS;
}

void ThingCore::Start() {
  wifi_status_led_uninstall();
  pinMode(TC_PINLED, OUTPUT);
  pinMode(TC_PINRESET, INPUT_PULLUP);
  digitalWrite(TC_PINLED, TC_LEDOFF);

  Serial.printf("\n\n*** Initializing ThingCore version %s ***\n",
            TC_LIBVER);

  if (!SPIFFS.begin())
    Serial.printf("SPIFFS: SPIFFS won't initialize!\n");
  else
    Serial.printf("SPIFFS: Initialized Successfully\n");

  _LoadConfig();
  _SetupWifi();
  _SetupHttp();
  _SetupPubSub();
  _SetupWebSockets();
}

void ThingCore::Loop() {
  _httpServer.handleClient();
  _LoopPubSub();
  _webSockets.loop();
  _LoopStatusLed();
}

String ThingCore::getDeviceName() {
  return conf_devicename;
}

void ThingCore::_LoadConfig() {
  Serial.printf("Config: Loading Config... ");
  if (SPIFFS.exists("/config.json")) {
    Serial.printf("Reading Saved File... ");
    File configFile = SPIFFS.open("/config.json", "r");
    if (configFile) {
      size_t size = configFile.size();
      std::unique_ptr<char[]> buf(new char[size]);
      configFile.readBytes(buf.get(), size);
      DynamicJsonBuffer buffer;
      JsonObject& json = buffer.parseObject(buf.get());

      if (_appName.equalsIgnoreCase(json["app_name"])) {
        if (json.containsKey("device_name")) conf_devicename = json["device_name"].as<String>();
        if (json.containsKey("web_updatepath")) conf_webupdatepath = json["web_updatepath"].as<String>();
        if (json.containsKey("web_user")) conf_webuser = json["web_user"].as<String>();
        if (json.containsKey("web_pass")) conf_webpass = json["web_pass"].as<String>();
        if (json.containsKey("mqtt_server")) conf_mqttserver = json["mqtt_server"].as<String>();
        if (json.containsKey("mqtt_port")) conf_mqttport = json["mqtt_port"].as<int>();
        if (json.containsKey("mqtt_user")) conf_mqttuser = json["mqtt_user"].as<String>();
        if (json.containsKey("mqtt_pass")) conf_mqttpass = json["mqtt_pass"].as<String>();
        Serial.printf("Done!\n");
      } else {
        Serial.printf("App Name changed. Leaving defaults\n");
      }
    }
  } else
    Serial.printf("No config file yet\n");
}

bool ThingCore::_SaveConfig() {
  Serial.printf("Config: Saving Config... ");

  DynamicJsonBuffer buffer;
  JsonObject& json = buffer.createObject();

  json["app_name"] = _appName;
  json["device_name"] = conf_devicename;
  json["web_updatepath"] = conf_webupdatepath;
  json["web_user"] = conf_webuser;
  json["web_pass"] = conf_webpass;
  json["mqtt_server"] = conf_mqttserver;
  json["mqtt_port"] = conf_mqttport;
  json["mqtt_user"] = conf_mqttuser;
  json["mqtt_pass"] = conf_mqttpass;

  json.printTo(Serial);

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.printf("Failed to open config file for writing\n");
    return false;
  } else {
    json.printTo(configFile);
    configFile.close();
    Serial.printf("Saved!\n");
    return true;
  }
}

void ThingCore::_SetupWifi() {
  WiFi.hostname(conf_devicename);
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setMinimumSignalQuality(25);

  if (digitalRead(TC_PINRESET) == LOW) {
    Serial.printf("WiFi: Reset pin is set; Running WiFi configuration\n");
    wifiManager.startConfigPortal(conf_devicename.c_str(), NULL);
  }

  Serial.printf("WiFi: Starting WiFi...\n");
  if (!wifiManager.autoConnect(conf_devicename.c_str())) { ESP.reset(); }
  Serial.printf("WiFi: Connected! Local IP: %s\n", WiFi.localIP().toString().c_str());
}

bool ThingCore::_PubSubConfigured() {
  return (conf_mqttserver.length() > 0) && (conf_mqttport > 0);
}

void ThingCore::_SetupPubSub() {
  if (_PubSubConfigured())
  {
    _mqttClient.setServer(conf_mqttserver.c_str(), conf_mqttport);

    auto f = [&](char* topic, byte* payload, unsigned int length) {
      char message_buff[length + 1];
      strncpy(message_buff, (char*)payload, length);
      message_buff[length] = '\0';
      Serial.printf("PubSub: Received Topic: %s Payload: %s\n", topic, message_buff);
      if (this->cb_pubsubmessage)
        this->cb_pubsubmessage(String(topic), message_buff);
    };
    _mqttClient.setCallback(f);
  }
}

void ThingCore::_SetupHttp() {
  _httpUpdater.setup(&_httpServer, conf_webupdatepath.c_str(), conf_webuser.c_str(), conf_webpass.c_str());
  _httpServer.on("/settings", HTTPMethod::HTTP_GET, [&]() {
    if(!_httpServer.authenticate(conf_webuser.c_str(), conf_webpass.c_str()))
      return _httpServer.requestAuthentication();
    Serial.printf("HttpServer: Request from %s uri: %s\n",
        _httpServer.client().remoteIP().toString().c_str(),
        _httpServer.uri().c_str());
    String content;
    content = "<!DOCTYPE HTML>\r\n<html><h1>Settings</h1>";
    content += "<form method=\"post\" action=\"/settings\">";
    content += "<label>Device Name: </label><input name=\"device_name\" length=32 value=\"";
    content += conf_devicename;
    content += "\" /><br />";
    content += "<label>Web Username: </label><input name=\"web_user\" length=32 value=\"";
    content += conf_webuser;
    content += "\" /><br />";
    content += "<label>Web Password: </label><input name=\"web_pass\" length=64 value=\"";
    content += conf_webpass;
    content += "\" /><br />";
    content += "<label>MQTT Server: </label><input name=\"mqtt_server\" length=64 value=\"";
    content += conf_mqttserver;
    content += "\" /><br />";
    content += "<label>MQTT Port: </label><input name=\"mqtt_port\" length=6 value=\"";
    content += conf_mqttport;
    content += "\" /><br />";
    content += "<label>MQTT Username: </label><input name=\"mqtt_user\" length=32 value=\"";
    content += conf_mqttuser;
    content += "\" /><br />";
    content += "<label>MQTT Password: </label><input name=\"mqtt_pass\" length=64 value=\"";
    content += conf_mqttpass;
    content += "\" /><br />";
    content += "<input type=\"submit\" value=\"Save\" /></form></p></html>";
    _httpServer.send(200, "text/html", content);
  });
  _httpServer.on("/settings", HTTPMethod::HTTP_POST, [&]() {
    if(!_httpServer.authenticate(conf_webuser.c_str(), conf_webpass.c_str()))
      return _httpServer.requestAuthentication();
    Serial.printf("HttpServer: Request from %s uri: %s\n",
        _httpServer.client().remoteIP().toString().c_str(),
        _httpServer.uri().c_str());
    conf_devicename = _httpServer.arg("device_name");
    conf_webuser = _httpServer.arg("web_user");
    conf_webpass = _httpServer.arg("web_pass");
    conf_mqttserver = _httpServer.arg("mqtt_server");
    conf_mqttport = _httpServer.arg("mqtt_port").toInt();
    conf_mqttuser = _httpServer.arg("mqtt_user");
    conf_mqttpass = _httpServer.arg("mqtt_pass");
    if (_SaveConfig()) {
      _httpServer.sendHeader("Location", String("/settings"), true);
      _httpServer.send(302, "text/plain", "");
      ESP.reset();
    } else {
      _httpServer.send(200, "application/json", "{\"Failure\":\"Settings did not save\"}");
    }
  });
  _httpServer.on("/", HTTPMethod::HTTP_GET, [&]() {
    Serial.printf("HttpServer: Request from %s uri: %s\n",
        _httpServer.client().remoteIP().toString().c_str(),
        _httpServer.uri().c_str());
    String content;
    content = "<!DOCTYPE HTML><html><head><title>" + conf_devicename + "</title><style>body { font-family: monospace; }</style></head><body>";
    content += "<h1>Hello from " + conf_devicename + "!</h1>";
    content += "<h2>ThingCore Platform</h2>";
    content += "<ul>";
    content += "<li>App Name&nbsp;&nbsp;&nbsp;&nbsp;: " + _appName + "</li>";
    content += "<li>App Version&nbsp;: " + _appVersion + "</li>";
    content += "<li>Core Version: ";
      content += TC_LIBVER;
      content += "</li>";
    content += "</ul>";
    content += "<ul>";
    content += "<li>WiFi Network: " + WiFi.SSID() + "</li>";
    content += "<li>WiFi RSSI&nbsp;&nbsp;&nbsp;: ";
      content += WiFi.RSSI();
      content += "</li>";
    content += "<li>Device MAC&nbsp;&nbsp;: " + WiFi.macAddress() + "</li>";
    content += "</ul>";
    content += "<ul>";
    content += "<li>WebSockets&nbsp;&nbsp;: ";
      content += (true ? "TRUE" : "FALSE");
      content += "</li>";
    content += "</ul>";
    content += "<ul>";
    content += "<li>PubSub&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;: ";
      content += (_PubSubConfigured() ? "TRUE" : "FALSE");
      content += "</li>";
    if (_PubSubConfigured()) {
      content += "<li>Connected&nbsp;&nbsp;&nbsp;: ";
        content += (_mqttClient.connected() ? "TRUE" : "FALSE");
        content += "</li>";
      if (_pubSubListenTopic.length() > 0)
        content += "<li>Subscribed&nbsp;&nbsp;: " + _pubSubListenTopic + "</li>";
    }
    content += "</ul>";
    if (this->cb_webstatusrender) {
      content += "<h2>" + _appName + " App</h2>";
      content += this->cb_webstatusrender();
    }
    content += "</body></html>";
    _httpServer.send(200, "text/html", content);
  });
  _httpServer.begin();
  Serial.printf("HttpServer: Listening on port %d\n", TC_PORTHTTP);
}

void ThingCore::_SetupWebSockets() {
  auto f = [&](uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
      case WStype_DISCONNECTED:
        Serial.printf("WebSockets: [%u] Disconnected!\n", num);
        break;
      case WStype_CONNECTED: {
        IPAddress ip = _webSockets.remoteIP(num);
        Serial.printf("WebSockets: [%u] Connected from %s url: %s\n", num, ip.toString().c_str(), payload);
        if (this->cb_websockconnect)
          this->cb_websockconnect(num);
        }
        break;
      case WStype_TEXT: {
        Serial.printf("WebSockets: [%u] Received: %s\n", num, payload);
        char message_buff[length + 1];
        strncpy(message_buff, (char*)payload, length);
        message_buff[length] = '\0';
        if (this->cb_websockmessage)
          this->cb_websockmessage(num, message_buff);
        }
        break;
    }
  };

  _webSockets.begin();
  _webSockets.onEvent(f);
  Serial.printf("WebSockets: Listening on port %d\n", TC_PORTWS);
}

void ThingCore::_LoopPubSub() {
  if (!_PubSubConfigured()) return;
  if (!_mqttClient.connected()) {
    if (_psLastConnect == 0) {
      digitalWrite(TC_PINLED, TC_LEDOFF);
      Serial.printf("PubSub: Attempting MQTT connection... ");
      if (_mqttClient.connect(conf_devicename.c_str(), conf_mqttuser.c_str(),
          conf_mqttpass.c_str())) {
        digitalWrite(TC_PINLED, TC_LEDON);
        Serial.printf("connected!\n");
        if (this->cb_pubsubconnect)
          this->cb_pubsubconnect();
        _DoSubscribe();
      } else {
        Serial.printf("failed, rc=%d\n", _mqttClient.state());
        _psLastConnect = millis();
      }
    }
    else
    {
      if (millis() - _psLastConnect >= 5000)
        _psLastConnect = 0;
    }
  }
  else
    _mqttClient.loop();
}

void ThingCore::_LoopStatusLed() {
  unsigned long led_now = millis();
  if (led_now - _ledLastNow > 500) {
    if (_ledBlinks > 0) {
      _ledLastNow = led_now;
      _ledLastState = (_ledLastState == TC_LEDON) ? TC_LEDOFF : TC_LEDON;
      digitalWrite(TC_PINLED, _ledLastState);
      if (_ledLastState == TC_LEDON) _ledBlinks--;
      if (_ledBlinks == 0) {
        _ledLastNow = 0;
        _ledLastState = TC_LEDON;
      }
    } else {
      digitalWrite(TC_PINLED, _mqttClient.connected() ? TC_LEDON : TC_LEDOFF);
    }
  }
}

void ThingCore::_DoSubscribe() {
  if ((_pubSubListenTopic.length() > 0) && _mqttClient.connected()) {
    _mqttClient.subscribe(_pubSubListenTopic.c_str());
    Serial.printf("PubSub: Subscribed to topic: %s\n", _pubSubListenTopic.c_str());
  }
}

void ThingCore::BlinkStatusLed(int count) {
  _ledBlinks = count;
}

void ThingCore::RegisterPubSubConnect(TC_CALLBACK_PUBSUBCONNECT) {
  this->cb_pubsubconnect = cb_pubsubconnect;
  if (_mqttClient.connected()) { this->cb_pubsubconnect(); }
}

void ThingCore::RegisterWebStatusRender(TC_CALLBACK_WEBSTATUSRENDER) {
  this->cb_webstatusrender = cb_webstatusrender;
}

void ThingCore::RegisterWebSocketsConnect(TC_CALLBACK_WEBSOCKCONNECT) {
  this->cb_websockconnect = cb_websockconnect;
}

void ThingCore::RegisterWebSocketsMessage(TC_CALLBACK_WEBSOCKMESSAGE) {
  this->cb_websockmessage = cb_websockmessage;
}

void ThingCore::RegisterPubSubMessage(String topic, TC_CALLBACK_PUBSUBMESSAGE) {
  _pubSubListenTopic = topic;
  this->cb_pubsubmessage = cb_pubsubmessage;
  _DoSubscribe();
}

bool ThingCore::PubSubPublish(String topic, const char* payload, bool retained) {
  if (_PubSubConfigured())
  {
    Serial.printf("PubSub: Publishing Topic: %s Payload: %s\n", topic.c_str(), payload);
    return _mqttClient.publish(topic.c_str(), payload, retained);
  } else {
    Serial.printf("PubSub: Discarding publish request because mqtt is not enabled\n");
    return false;
  }
}

void ThingCore::WebSocketsSend(const char* payload) {
  Serial.printf("WebSockets: [B] Sending: %s\n", payload);
  _webSockets.broadcastTXT(payload);
}

void ThingCore::WebSocketsSend(uint8_t num, const char* payload) {
  Serial.printf("WebSockets: [%u] Sending: %s\n", num, payload);
  _webSockets.sendTXT(num, payload);
}
