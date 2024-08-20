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
Adafruit_VEML7700 veml = Adafruit_VEML7700();

PatronusClass* callbackInstance = nullptr;


void staticNewDataCallback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec)
{
  if (callbackInstance) {
    callbackInstance->bmeNewDataCallback(data, outputs, bsec);
  }
}

bool PatronusClass::begin() {
  this->suspended = false;
  Serial.println("Patronus begin ...");
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
  Serial.println("Patronus loop ...");
  if(settings.Patronus.enabled && !rebootDelay.reboot && !this->suspended) {
    esp_task_wdt_reset();
    if(!this->connected()) this->connect();
  }
  if(settings.Patronus.enabled) {
    // Query VEML7700
    this->lastLux = veml.readLux(VEML_LUX_AUTO);

    // Query BME688
    if (!envSensor.run())
    {
      this->bmeCheckBsecStatus(envSensor);
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
  Serial.println("Patronus connect ...");
  esp_task_wdt_reset(); // Make sure we do not reboot here.

  Wire.begin();
  Serial.println("Patronus connect wire began");

  /* Initialize VEML7700 sensor */
  this->vemlConnected = veml.begin();

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
  this->bsec2Connected = envSensor.begin(BME68X_I2C_ADDR_LOW, Wire);
  if (!this->bsec2Connected)
  {
    return this->bmeCheckBsecStatus(envSensor);
  }

  /* Subsribe to the desired BSEC2 outputs */
  if (!envSensor.updateSubscription(sensorList, ARRAY_LEN(sensorList), BSEC_SAMPLE_RATE_LP))
  {
    return this->bmeCheckBsecStatus(envSensor);
  }

  /* Whenever new data is available call the newDataCallback function */
  callbackInstance = this;
  envSensor.attachCallback(staticNewDataCallback);

  //Serial.println("BSEC library version " +
  //               String(envSensor.version.major) + "." + String(envSensor.version.minor) + "." + String(envSensor.version.major_bugfix) + "." + String(envSensor.version.minor_bugfix));
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
    return;
  }
  //Serial.println("BSEC outputs:\n\ttimestamp = " + String((int)(outputs.output[0].time_stamp / INT64_C(1000000))));

  // Verify readingsDelay
  if (millis() - this->readingsTime >= this->readingsDelay)
    this->readingsTime += this->readingsDelay;
  else
  {
    //Serial.println("Reading skipped due to delay filtering");
    return;
  }

  this->lastOutputs = outputs;
}

void PatronusClass::emitData(const char *evt) { this->emitData(255, evt); }
void PatronusClass::emitData(uint8_t num, const char *evt) {
  JsonSockEvent *json = sockEmit.beginEmit(evt);
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