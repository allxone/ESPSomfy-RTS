#ifndef Patronus_H
#define Patronus_H
#include <bme68xLibrary.h>
#include <bsec2.h>
#include <"Adafruit_VEML7700.h>"
#include <Arduino.h>
#include <ArduinoJson.h>

class PatronusClass {
  public:
    uint64_t lastConnect = 0;
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
    /**
     * @brief : This function checks the BSEC status, prints the respective error code. Halts in case of error
     * @param[in] bsec  : Bsec2 class object
     */
    void bmeCheckBsecStatus(Bsec2 bsec);

    /**
     * @brief : This function is called by the BSEC library when a new output is available
     * @param[in] input     : BME68X sensor data before processing
     * @param[in] outputs   : Processed BSEC BSEC output data
     * @param[in] bsec      : Instance of BSEC2 calling the callback
     */
    void bmeNewDataCallback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec);

};
#endif