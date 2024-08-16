#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include "ConfigSettings.h"
#include "Patronus.h"
#include "Somfy.h"
#include "Utils.h"

extern ConfigSettings settings;
extern SomfyShadeController somfy;
extern rebootDelay_t rebootDelay;

bool PatronusClass::begin() {
  this->suspended = false;
  return true;
}
bool PatronusClass::end() {
  this->suspended = true;
  this->disconnect();
  return true;
}
void PatronusClass::reset() {
  this->disconnect();
  this->lastConnect = 0;
  this->connect();
}
bool PatronusClass::loop() {
  esp_task_wdt_reset();
  return true;
}
bool PatronusClass::connect() {
  esp_task_wdt_reset(); // Make sure we do not reboot here.
  return true;
}
bool PatronusClass::disconnect() {
  return true;
}
bool PatronusClass::connected() {
  return false;
}
