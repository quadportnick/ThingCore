#include <FS.h>
#include <DNSServer.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include "WiFiManager.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define TC_LIBVER   "2.0.20170226"
#define TC_PINLED   LED_BUILTIN
#define TC_PINRESET D5
#define TC_LEDON    LOW
#define TC_LEDOFF   HIGH
#define TC_CALLBACK_GETAPPSTATUS    void (*cb_getappstatus)(JsonObject&)
#define TC_CALLBACK_PUBSUBMESSAGE   void (*cb_pubsubmessage)(String, const char*)
#define TC_CALLBACK_PUBSUBCONNECT   void (*cb_pubsubconnect)()
#define TC_CALLBACK_WEBSOCKMESSAGE  void (*cb_websockmessage)(uint8_t, const char*)
#define TC_CALLBACK_WEBSOCKCONNECT  void (*cb_websockconnect)(uint8_t)

#define TC_PORTHTTP   8023
#define TC_PORTWS     9023

#define DEFAULT_CONFIG "{\"web_user\":\"webuser\",\"web_pass\":\"i love passwords.\" }"

using namespace std;

class ThingCore
{
private:
  WiFiClient _espClient;
  ESP8266WebServer _httpServer;
  ESP8266HTTPUpdateServer _httpUpdater;
  PubSubClient _mqttClient;
  WebSocketsServer _webSockets;

  String _appName;
  String _appVersion;
  String _pubSubListenTopic;
  String _config;
  String _defaultDeviceName;
  bool _pubSubConfigured;
  int _ledBlinks = 0;
  int _ledLastState = TC_LEDON;
  unsigned long _psLastConnect = 0;
  unsigned long _ledLastNow = 0;

  TC_CALLBACK_GETAPPSTATUS;
  TC_CALLBACK_PUBSUBMESSAGE;
  TC_CALLBACK_PUBSUBCONNECT;
  TC_CALLBACK_WEBSOCKMESSAGE;
  TC_CALLBACK_WEBSOCKCONNECT;

  void _loadSettings();
  String _getSetting(String key);
  const char* _getSettingChar(String key);
  int _getSettingInt(String key);
  bool _haveSetting(String key);
  void _setSetting(String key, String value);
  void _setSetting(String key, int value);
  bool _saveSettings();

  String _getAppStatus();
  void _loopStatusLed();

  void _setupWifi();

  void _setupPubSub();
  void _loopPubSub();
  void _doSubscribe();

  void _setupHttp();
  void _loopHttp();

  void _setupWebSockets();
  void _loopWebSockets();

public:
  ThingCore(String app_name, String app_verson);
  void start();
  void loop();

  String getDeviceName();
  void blinkStatusLed(int count);

  void setAppStatusHandler(TC_CALLBACK_GETAPPSTATUS);

  void setPubSubMessageHandler(String topic, TC_CALLBACK_PUBSUBMESSAGE);
  void setPubSubConnectHandler(TC_CALLBACK_PUBSUBCONNECT);
  bool pubSubPublish(String topic, const char* payload, bool retained);

  void setWebSocketsMessageHandler(TC_CALLBACK_WEBSOCKMESSAGE);
  void setWebSocketsConnectHandler(TC_CALLBACK_WEBSOCKCONNECT);
  void webSocketsSend(const char* payload);
  void webSocketsSend(uint8_t num, const char* payload);
};
