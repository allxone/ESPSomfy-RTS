#include "Patronus.h"
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include "ConfigSettings.h"
#include "MQTT.h"
#include "Somfy.h"
#include "Sockets.h"
#include "Network.h"
#include "Utils.h"

extern ConfigSettings settings;
extern rebootDelay_t rebootDelay;
extern MQTTClass mqtt;
extern SocketEmitter sockEmit;

Bsec2 envSensor;
//Adafruit_VEML7700 veml = Adafruit_VEML7700();

PatronusClass* callbackInstance = nullptr;


void staticNewDataCallback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec)
{
  if (callbackInstance) {
    callbackInstance->bmeNewDataCallback(data, outputs, bsec);
  }
}

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
  this->connect();
}
bool PatronusClass::loop() {
  if(settings.Patronus.enabled && !rebootDelay.reboot && !this->suspended) {
    esp_task_wdt_reset();
    if(!this->connected()) this->connect();
  }

  if(settings.Patronus.enabled && !this->suspended) {

    // Query VEML7700
    // this->lastLux = veml.readLux(VEML_LUX_AUTO);

    // Query BME688
    if (!envSensor.run())
    {
      return this->bmeCheckBsecStatus(envSensor);
    }

    if(millis() - this->lastEmit > 15000) {
      // Post our connection status if needed.
      this->lastEmit = millis();
      if(this->connected()) {
        this->emitData();
        this->lastEmit = millis();
      }
      esp_task_wdt_reset(); // Make sure we do not reboot here.
    }

  }
  return true;
}
void PatronusClass::publish() {
  if(mqtt.connected()) {
    //TODO: publish bsec2 data
    mqtt.publish("patronus/data/lux", String(this->lastLux), true);
  }
}
bool PatronusClass::connect() {
  this->bsec2Connected = false;

  esp_task_wdt_reset(); // Make sure we do not reboot here.

  Wire.begin();

  /* Desired subscription list of BSEC2 outputs */
  bsecSensor sensorList[] = {
      BSEC_OUTPUT_STABILIZATION_STATUS,
      BSEC_OUTPUT_RUN_IN_STATUS,
      BSEC_OUTPUT_STATIC_IAQ,
      BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
      BSEC_OUTPUT_CO2_EQUIVALENT,
      BSEC_OUTPUT_RAW_TEMPERATURE,
      BSEC_OUTPUT_RAW_PRESSURE,
      BSEC_OUTPUT_RAW_HUMIDITY,
      BSEC_OUTPUT_RAW_GAS,
      BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
      BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
  };

  /* Initialize the bsec2 library and interfaces */
  if (!envSensor.begin(BME68X_I2C_ADDR_LOW, Wire));
  {
    Serial.println("BME688 can't begin!");
    this->bmeCheckBsecStatus(envSensor);
    return false;
  }

  /* Subsribe to the desired BSEC2 outputs */
  if (!envSensor.updateSubscription(sensorList, ARRAY_LEN(sensorList), BSEC_SAMPLE_RATE_ULP))
  {
    Serial.println("BME688 can't subscribe!");
    this->bmeCheckBsecStatus(envSensor);
    return false;
  }

  /* Whenever new data is available call the newDataCallback function */
  callbackInstance = this;
  envSensor.attachCallback(staticNewDataCallback);
  this->bsec2Connected = true;

  /* Initialize VEML7700 sensor */
  // this->vemlConnected = veml.begin(&Wire);
  this->vemlConnected = false;

  return true;
}
bool PatronusClass::disconnect() {
  //Serial.println("BSEC library callback detached!");
  this->vemlConnected = false;
  this->bsec2Connected = false;
  return true;
}
bool PatronusClass::connected() {
  if(settings.Patronus.enabled) return this->vemlConnected || this->bsec2Connected;
  return false;
}
void PatronusClass::bmeNewDataCallback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec)
{
  if (!outputs.nOutputs)
  {
    Serial.println("BME688 no output!");
    return;
  }

  // Verify readingsDelay
  if (millis() - this->readingsTime < settings.Patronus.warmup * 1000)
  {
    Serial.println("BME688 warmup!");
    return;
  }
  else
  {
    this->lastOutputs = outputs;
    Serial.println("BME688 data downloaded!");
  }
}

void PatronusClass::emitData() { this->emitData(255); }
void PatronusClass::emitData(uint8_t num) {
  JsonSockEvent *json = sockEmit.beginEmit("patronusData");
  json->beginObject();
  json->addElem("lux", this->lastLux);
  for (uint8_t i = 0; i < this->lastOutputs.nOutputs; i++)
  {
    const bsecData output = this->lastOutputs.output[i];
    switch (output.sensor_id)
    {
    case BSEC_OUTPUT_STABILIZATION_STATUS:
      json->addElem("stabilization_status", String(output.signal));
      break;
    case BSEC_OUTPUT_RUN_IN_STATUS:
      json->addElem("run_in_status", String(output.signal));
      break;
    case BSEC_OUTPUT_STATIC_IAQ:
      json->addElem("iaq", String(output.signal));
      break;
    case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
      json->addElem("vocp", String(output.signal));
      break;
    case BSEC_OUTPUT_CO2_EQUIVALENT:
      json->addElem("co2", String(output.signal));
      break;
    case BSEC_OUTPUT_RAW_TEMPERATURE:
      json->addElem("temperature_raw", String(output.signal));
      break;
    case BSEC_OUTPUT_RAW_PRESSURE:
      json->addElem("pressure_raw", String(output.signal));
      break;
    case BSEC_OUTPUT_RAW_HUMIDITY:
      json->addElem("humidity_raw", String(output.signal));
      break;
    case BSEC_OUTPUT_RAW_GAS:
      json->addElem("gas_raw", String(output.signal));
      break;
    case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
      json->addElem("temperature", String(output.signal));
      break;
    case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
      json->addElem("humidity", String(output.signal));
      break;
    default:
      break;
    }
  }
  json->endObject();
  sockEmit.endEmit(num);

  // Publish to MQTT
  this->publish();
}

bool PatronusClass::bmeCheckBsecStatus(Bsec2 bsec)
{
  if (bsec.status < BSEC_OK)
  {
    Serial.print(F("BSEC err: "));
    Serial.println(String(bsec.status));
    return false;
  }
  else if (bsec.status > BSEC_OK)
  {
    Serial.print(F("BSEC warn: "));
    Serial.println(String(bsec.status));
    return true;
  }

  if (bsec.sensor.status < BME68X_OK)
  {
    Serial.print(F("BME68X err: "));
    Serial.println(String(bsec.sensor.status));
    return false;
  }
  else if (bsec.sensor.status > BME68X_OK)
  {
    Serial.print(F("BME68X warn: "));
    Serial.println(String(bsec.sensor.status));
    return true;
  }
  return false;
}