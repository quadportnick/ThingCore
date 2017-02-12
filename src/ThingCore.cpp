#include "ThingCore.h"

ThingCore::ThingCore(String app_name, String app_version) :
                            _webSockets(TC_PORTWS), _httpServer(TC_PORTHTTP) {

  _mqttClient = PubSubClient(_espClient);

  _appName = app_name;
  _appVersion = app_version;
  _pubSubListenTopic = "";

  _defaultDeviceName = _appName + "-" + String(ESP.getChipId());
  _defaultDeviceName.toLowerCase();
}

void ThingCore::start() {
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

  _loadSettings();
  _setupWifi();
  _setupHttp();
  _setupPubSub();
  _setupWebSockets();
}

void ThingCore::loop() {
  _loopStatusLed();
  _loopHttp();
  _loopPubSub();
  _loopWebSockets();
}

String ThingCore::getDeviceName() {
  return _getSetting("device_name");
}

void ThingCore::blinkStatusLed(int count) {
  _ledBlinks = count;
}

void ThingCore::setAppStatusHandler(TC_CALLBACK_GETAPPSTATUS) {
  this->cb_getappstatus = cb_getappstatus;
}

/****************************************************************************/

String ThingCore::_getAppStatus() {
  DynamicJsonBuffer buff;
  JsonObject& status = buff.createObject();
  status["core_version"] = TC_LIBVER;
  status["app_name"] = _appName;
  status["app_version"] = _appVersion;
  status["device_name"] = _getSetting("device_name");
  status["wifi_ssid"] = WiFi.SSID();
  status["wifi_rssi"] = WiFi.RSSI();
  status["wifi_mac"] = WiFi.macAddress();
  status["websockets_enabled"] = (true ? "true" : "false");
  status["websockets_port"] = TC_PORTWS;
  status["pubsub_configured"] = (_pubSubConfigured ? "true" : "false");
  status["pubsub_connected"] = (_mqttClient.connected() ? "true" : "false");
  status["pubsub_subscribed"] = _pubSubListenTopic;
  if (this->cb_getappstatus)
    this->cb_getappstatus(status);
  String output;
  status.printTo(output);
  return output;
}

void ThingCore::_loopStatusLed() {
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
      digitalWrite(TC_PINLED, TC_LEDOFF);
    }
  }
}
