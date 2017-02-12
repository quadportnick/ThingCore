# ThingCore
This is my common middleware library for my IoT projects around the house since 80%+ of each device's codebase is the same fundamental stuff. It's currently based on ESP8266. This is a work in progress as I learn and improve.

## Features

### WiFi Configuration
When the ESP8266's wifi is not configured, it will broadcast as an access point. Connecting to that AP will provide a captive portal to configure WiFi settings. If the WiFi network fails in the future it will go into this mode with a timeout, as well as if the RESET_PIN is set. Leveraging WiFiManager for this functionality.

### HTTP Server
This area needs some work, currently redoing it. Provides a few services on port 8023 thanks to the ESP8266WebServer library:
* /status - returns a JSON of the platform's current status
* /settings (authenticated) - returns a JSON of the platform's current settings, as well as setting new settings via POST
* /update (authenticated) - OTA firmware update via ESP8266HTTPUpdateServer
Apps using Core can extend the status JSON for frontend UI to display appropriate customized content. Default logon is webuser / i love passwords.

## WebSockets Server
Listens on port 9023 for WebSockets connections thanks to the WebSocketsServer library. Apps using Core can leverage communications over WebSockets. There are no default handlers.

## PubSub (MQTT) Client
If configured, the platform will connect to an MQTT server using the PubSubClient library. Apps using Core can subscribe and publish to topics. There are no default handlers. I use this for state/control with Home Assistant.

## Settings
Leveraging ArduinoJson to save/restore settings to the device's filesystem. I plan to extend this to the apps I build on this core, however most of them don't need it.

## Blinky Light
Apps can blink the on-board LED. Oooooh.
