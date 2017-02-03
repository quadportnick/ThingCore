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

#define TC_LIBVER   "1.2.20170202"
#define TC_PINLED   LED_BUILTIN
#define TC_PINRESET D5
#define TC_LEDON    LOW
#define TC_LEDOFF   HIGH
#define TC_CALLBACK_PUBSUBMESSAGE   void (*cb_pubsubmessage)(String, const char*)
#define TC_CALLBACK_PUBSUBCONNECT   void (*cb_pubsubconnect)()
#define TC_CALLBACK_WEBSOCKMESSAGE  void (*cb_websockmessage)(uint8_t, const char*)
#define TC_CALLBACK_WEBSOCKCONNECT  void (*cb_websockconnect)(uint8_t)
#define TC_CALLBACK_WEBSTATUSRENDER String (*cb_webstatusrender)()

#define TC_PORTHTTP   8023
#define TC_PORTWS     9023

#define TC_DEFAULT_UPDATEURI   "/update"
#define TC_DEFAULT_WEBUSER     "webuser"
#define TC_DEFAULT_WEBPASS     "i love passwords."
#define TC_DEFAULT_MQTTSERVER  ""
#define TC_DEFAULT_MQTTPORT    1883
#define TC_DEFAULT_MQTTUSER    ""
#define TC_DEFAULT_MQTTPASS    ""

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
  int _ledBlinks = 0;
  int _ledLastState = TC_LEDON;
  unsigned long _psLastConnect = 0;
  unsigned long _ledLastNow = 0;

  String conf_devicename;
  String conf_webupdatepath;
  String conf_webuser;
  String conf_webpass;
  String conf_mqttserver;
  int    conf_mqttport;
  String conf_mqttuser;
  String conf_mqttpass;

  TC_CALLBACK_PUBSUBMESSAGE;
  TC_CALLBACK_PUBSUBCONNECT;
  TC_CALLBACK_WEBSOCKMESSAGE;
  TC_CALLBACK_WEBSOCKCONNECT;
  TC_CALLBACK_WEBSTATUSRENDER;

  void _LoadConfig();
  bool _SaveConfig();

  void _SetupWifi();
  void _SetupDebug();
  void _SetupPubSub();
  void _SetupHttp();
  void _SetupWebSockets();

  void _LoopPubSub();
  void _LoopStatusLed();

  bool _PubSubConfigured();
  void _DoSubscribe();

public:
  ThingCore(String app_name, String app_verson);
  void Start();
  void Loop();

  String getDeviceName();
  void BlinkStatusLed(int count);

  void RegisterPubSubMessage(String topic, TC_CALLBACK_PUBSUBMESSAGE);
  void RegisterPubSubConnect(TC_CALLBACK_PUBSUBCONNECT);
  bool PubSubPublish(String topic, const char* payload, bool retained);

  void RegisterWebSocketsMessage(TC_CALLBACK_WEBSOCKMESSAGE);
  void RegisterWebSocketsConnect(TC_CALLBACK_WEBSOCKCONNECT);
  void WebSocketsSend(const char* payload);
  void WebSocketsSend(uint8_t num, const char* payload);

  void RegisterWebStatusRender(TC_CALLBACK_WEBSTATUSRENDER);
};
