#include <ThingCore.h>

void ThingCore::_setupHttp() {
  _httpUpdater.setup(&_httpServer,"/update",
      _getSettingChar("web_user"),
      _getSettingChar("web_pass"));
  _httpServer.on("/settings", HTTPMethod::HTTP_GET, [&]() {
    if(!_httpServer.authenticate(_getSettingChar("web_user"), _getSettingChar("web_pass")))
      return _httpServer.requestAuthentication();
    Serial.printf("HttpServer: Request from %s uri: %s\n",
        _httpServer.client().remoteIP().toString().c_str(),
        _httpServer.uri().c_str());
    _httpServer.send(200, "application/json", _config);
  });
  _httpServer.on("/settings", HTTPMethod::HTTP_POST, [&]() {
    if(!_httpServer.authenticate(_getSettingChar("web_user"), _getSettingChar("web_pass")))
      return _httpServer.requestAuthentication();
    Serial.printf("HttpServer: Request from %s uri: %s\n",
        _httpServer.client().remoteIP().toString().c_str(),
        _httpServer.uri().c_str());
    DynamicJsonBuffer buff;
    JsonObject& obj = buff.parseObject(_httpServer.arg("plain"));
    if (obj.success()) {
      for (JsonObject::iterator it=obj.begin(); it!=obj.end(); ++it)
        _setSetting(it->key, it->value.as<String>());
      _saveSettings();
      _httpServer.send(200, "application/json", "{success: true}");
      ESP.reset();
    } else {
      _httpServer.send(500, "application/json", "{success: false}");
    }
  });
  _httpServer.on("/status", HTTPMethod::HTTP_GET, [&]() {
    Serial.printf("HttpServer: Request from %s uri: %s\n",
        _httpServer.client().remoteIP().toString().c_str(),
        _httpServer.uri().c_str());
    _httpServer.send(200, "application/json", _getAppStatus());
  });
  _httpServer.begin();
  Serial.printf("HttpServer: Listening on port %d\n", TC_PORTHTTP);
}

void ThingCore::_loopHttp() {
  _httpServer.handleClient();
}
