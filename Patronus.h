#ifndef Patronus_H
#define Patronus_H
#include <Arduino.h>
#include <ArduinoJson.h>
class PatronusClass {
  public:
    uint64_t lastConnect = 0;
    bool suspended = false;
    bool begin();
    bool loop();
    bool end();
    bool connect();
    bool disconnect();
    bool connected();
    void reset();
};
#endif