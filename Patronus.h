#ifndef Patronus_H
#define Patronus_H
#include <bme68xLibrary.h>
#include <bsec2.h>
//#include <Adafruit_VEML7700.h>
#include <Arduino.h>
#include <ArduinoJson.h>

class PatronusClass {
  public:
    bool suspended = false;
    bool vemlConnected = false;
    bool bsec2Connected = false;
    float lastLux;
    bsecOutputs lastOutputs;
    unsigned long readingsDelay = 15 * 1000L; // 15m
    unsigned long readingsTime = millis() - readingsDelay;
    bool begin();
    bool loop();
    bool end();
    bool connect();
    bool disconnect();
    bool connected();
    void reset();
    void publish();
    bool bmeCheckBsecStatus(Bsec2 bsec);
    void bmeNewDataCallback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec);
    void emitData(const char *evt = "patronusData");
    void emitData(uint8_t num, const char *evt = "patronusData");
};
#endif