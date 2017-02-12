#include <ThingCore.h>

void ThingCore::_setupWifi() {
  WiFi.hostname(_getSetting("device_name"));
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setMinimumSignalQuality(25);

  if (digitalRead(TC_PINRESET) == LOW) {
    Serial.printf("WiFi: Reset pin is set; Running WiFi configuration\n");
    wifiManager.startConfigPortal(
      _getSettingChar("device_name"), NULL);
  }

  Serial.printf("WiFi: Starting WiFi...\n");
  if (!wifiManager.autoConnect(
    _getSettingChar("device_name"))) { ESP.reset(); }
  Serial.printf("WiFi: Connected! Local IP: %s\n", WiFi.localIP().toString().c_str());
}
