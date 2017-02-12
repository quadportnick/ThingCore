#include <ThingCore.h>

void ThingCore::setPubSubMessageHandler(String topic, TC_CALLBACK_PUBSUBMESSAGE) {
  _pubSubListenTopic = topic;
  this->cb_pubsubmessage = cb_pubsubmessage;
  _doSubscribe();
}

void ThingCore::setPubSubConnectHandler(TC_CALLBACK_PUBSUBCONNECT) {
  this->cb_pubsubconnect = cb_pubsubconnect;
  if (_mqttClient.connected()) { this->cb_pubsubconnect(); }
}

bool ThingCore::pubSubPublish(String topic, const char* payload, bool retained) {
  if (_pubSubConfigured)
  {
    Serial.printf("PubSub: Publishing Topic: %s Payload: %s\n", topic.c_str(), payload);
    return _mqttClient.publish(topic.c_str(), payload, retained);
  } else {
    Serial.printf("PubSub: Discarding publish request because mqtt is not enabled\n");
    return false;
  }
}

/****************************************************************************/

void ThingCore::_setupPubSub() {
  _pubSubConfigured = (_getSetting("mqtt_server").length() > 0) &&
                (_getSettingInt("mqtt_port") > 0) &&
                (_getSetting("mqtt_user").length() > 0);

  if (_pubSubConfigured)
  {
    _mqttClient.setServer(_getSettingChar("mqtt_server"),
          _getSettingInt("mqtt_port"));

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

void ThingCore::_loopPubSub() {
  if (!_pubSubConfigured) return;
  if (!_mqttClient.connected()) {
    if (_psLastConnect == 0) {
      digitalWrite(TC_PINLED, TC_LEDOFF);
      Serial.printf("PubSub: Attempting MQTT connection... ");
      if (_mqttClient.connect(_getSettingChar("device_name"),
          _getSettingChar("mqtt_user"),
          _getSettingChar("mqtt_pass"))) {
        digitalWrite(TC_PINLED, TC_LEDON);
        Serial.printf("connected!\n");
        if (this->cb_pubsubconnect)
          this->cb_pubsubconnect();
        _doSubscribe();
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

void ThingCore::_doSubscribe() {
  if ((_pubSubListenTopic.length() > 0) && _mqttClient.connected()) {
    _mqttClient.subscribe(_pubSubListenTopic.c_str());
    Serial.printf("PubSub: Subscribed to topic: %s\n", _pubSubListenTopic.c_str());
  }
}
