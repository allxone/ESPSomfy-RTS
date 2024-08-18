#include <esp_task_wdt.h>
#include "ConfigSettings.h"
#include "Patronus.h"
#include "Utils.h"

extern ConfigSettings settings;
extern rebootDelay_t rebootDelay;

Bsec2 envSensor;
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
  this->lastConnect = 0;
  this->connect();
}
bool PatronusClass::loop() {
  if(settings.Patronus.enabled && !rebootDelay.reboot && !this->suspended) {
    esp_task_wdt_reset();
    if(!this->connected()) this->connect();
  }
  if(settings.Patronus.enabled) {
    // Query BME688
    if (!envSensor.run())
    {
      this->bmeCheckBsecStatus(envSensor);
    }
  }
  return true;
}
bool PatronusClass::connect() {
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

  /* Initialize the library and interfaces */
  if (!envSensor.begin(BME68X_I2C_ADDR_LOW, Wire))
  {
    this->bmeCheckBsecStatus(envSensor);
  }

  /* Subsribe to the desired BSEC2 outputs */
  if (!envSensor.updateSubscription(sensorList, ARRAY_LEN(sensorList), BSEC_SAMPLE_RATE_LP))
  {
    this->bmeCheckBsecStatus(envSensor);
  }

  /* Whenever new data is available call the newDataCallback function */
  callbackInstance = this;
  envSensor.attachCallback(staticNewDataCallback);

  //Serial.println("BSEC library version " +
  //               String(envSensor.version.major) + "." + String(envSensor.version.minor) + "." + String(envSensor.version.major_bugfix) + "." + String(envSensor.version.minor_bugfix));
  this->initialized = true;
  return true;
}
bool PatronusClass::disconnect() {
  //Serial.println("BSEC library callback detached!");
  this->initialized = false;
  return true;
}
bool PatronusClass::connected() {
  if(settings.Patronus.enabled) return this->initialized;
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

  String sensor;
  for (uint8_t i = 0; i < outputs.nOutputs; i++)
  {
    const bsecData output = outputs.output[i];
    switch (output.sensor_id)
    {
    case BSEC_OUTPUT_STABILIZATION_STATUS:
      sensor = "stabilization_status";
      break;
    case BSEC_OUTPUT_RUN_IN_STATUS:
      sensor = "run_in_status";
      break;
    case BSEC_OUTPUT_STATIC_IAQ:
      sensor = "iaq";
      break;
    case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
      sensor = "vocp";
      break;
    case BSEC_OUTPUT_CO2_EQUIVALENT:
      sensor = "co2";
      break;
    case BSEC_OUTPUT_RAW_TEMPERATURE:
      sensor = "temperature_raw";
      break;
    case BSEC_OUTPUT_RAW_PRESSURE:
      sensor = "pressure_raw";
      break;
    case BSEC_OUTPUT_RAW_HUMIDITY:
      sensor = "humidity_raw";
      break;
    case BSEC_OUTPUT_RAW_GAS:
      sensor = "gas_raw";
      break;
    case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
      sensor = "temperature";
      break;
    case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
      sensor = "humidity";
      break;
    default:
      //Serial.println("Warning!!! Unknown value");
      sensor = "unknown";
      break;
    }
    Serial.print(sensor);
    Serial.print(": ");
    Serial.println(String(output.signal));
    //Serial.println(" (accuracy: " + String((int)output.accuracy) + ")");
    //TODO: mqtt_publish(sensor, output.signal);
  }
}

void PatronusClass::bmeCheckBsecStatus(Bsec2 bsec)
{
  if (bsec.status < BSEC_OK)
  {
    Serial.print(F("BSEC err: "));
    Serial.println(String(bsec.status));
  }
  else if (bsec.status > BSEC_OK)
  {
    Serial.print(F("BSEC warn: "));
    Serial.println(String(bsec.status));
  }

  if (bsec.sensor.status < BME68X_OK)
  {
    Serial.print(F("BME68X err: "));
    Serial.println(String(bsec.sensor.status));
  }
  else if (bsec.sensor.status > BME68X_OK)
  {
    Serial.print(F("BME68X warn: "));
    Serial.println(String(bsec.sensor.status));
  }
}