#include <ThingCore.h>

void ThingCore::setWebSocketsMessageHandler(TC_CALLBACK_WEBSOCKMESSAGE) {
  this->cb_websockmessage = cb_websockmessage;
}

void ThingCore::setWebSocketsConnectHandler(TC_CALLBACK_WEBSOCKCONNECT) {
  this->cb_websockconnect = cb_websockconnect;
}

void ThingCore::webSocketsSend(const char* payload) {
  Serial.printf("WebSockets: [B] Sending: %s\n", payload);
  _webSockets.broadcastTXT(payload);
}

void ThingCore::webSocketsSend(uint8_t num, const char* payload) {
  Serial.printf("WebSockets: [%u] Sending: %s\n", num, payload);
  _webSockets.sendTXT(num, payload);
}

/****************************************************************************/

void ThingCore::_setupWebSockets() {
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

void ThingCore::_loopWebSockets() {
  _webSockets.loop();
}
