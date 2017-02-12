#include "ThingCore.h"

void ThingCore::_loadSettings() {
  Serial.printf("Config: Loading Config... ");
  _config = DEFAULT_CONFIG;
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
        _config = "";
        json.printTo(_config);
        Serial.printf("Done!\n");
      } else {
        Serial.printf("App Name changed. Leaving defaults\n");
      }
    }
  } else
    Serial.printf("No config file yet\n");

  if (!_haveSetting("device_name"))
    _setSetting("device_name", _defaultDeviceName);

  Serial.printf("Config is: %s\n", _config.c_str());
}

String ThingCore::_getSetting(String key) {
  DynamicJsonBuffer buffer;
  JsonObject& json = buffer.parseObject(_config);
  return json[key].as<String>();
}

const char* ThingCore::_getSettingChar(String key) {
  String val = _getSetting(key);
  char* result = new char[val.length()+1];
  strcpy(result,val.c_str());
  return result;
}

int ThingCore::_getSettingInt(String key) {
  String val = _getSetting(key);
  return val.toInt();
}

bool ThingCore::_haveSetting(String key) {
  DynamicJsonBuffer buffer;
  JsonObject& json = buffer.parseObject(_config);
  return json.containsKey(key);
}

void ThingCore::_setSetting(String key, String value) {
  DynamicJsonBuffer buffer;
  JsonObject& json = buffer.parseObject(_config);
  json[key] = value;
  _config = "";
  json.printTo(_config);
}

void ThingCore::_setSetting(String key, int value) {
  _setSetting(key, String(value));
}

bool ThingCore::_saveSettings() {
  Serial.printf("Config: Saving Config... ");

  DynamicJsonBuffer buffer;
  JsonObject& json = buffer.parseObject(_config);
  json["app_name"] = _appName;
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
