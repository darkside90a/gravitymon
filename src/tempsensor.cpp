/*
MIT License

Copyright (c) 2021-22 Magnus

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */
#include <DallasTemperature.h>
#include <Wire.h>
#include <OneWire.h>

#include <config.hpp>
#include <gyro.hpp>
#include <helper.hpp>
#include <tempsensor.hpp>

//
// Conversion between C and F
//
float convertCtoF(float t) { return (t * 1.8) + 32.0; }

#if !defined(USE_GYRO_TEMP)
OneWire myOneWire(D6);
DallasTemperature mySensors(&myOneWire);
#define TEMPERATURE_PRECISION 9
#endif

TempSensor myTempSensor;

//
// Setup temp sensors
//
void TempSensor::setup() {
#if defined(SIMULATE_TEMP)
  hasSensors = true;
  return;
#endif

#if defined(USE_GYRO_TEMP)
  Log.notice(F("TSEN: Using temperature from gyro." CR));
#else
#if LOG_LEVEL == 6 && !defined(TSEN_DISABLE_LOGGING)
  Log.verbose(F("TSEN: Looking for temp sensors." CR));
#endif
  mySensors.begin();

  if (mySensors.getDS18Count()) {
#if !defined(TSEN_DISABLE_LOGGING)
    Log.notice(F("TSEN: Found %d temperature sensor(s)." CR),
               mySensors.getDS18Count());
#endif
    mySensors.setResolution(TEMPERATURE_PRECISION);
  }
#endif

  float t = myConfig.getTempSensorAdj();

  // Set the temp sensor adjustment values
  if (myConfig.isTempC()) {
    tempSensorAdjF = t * 1.8;  // Convert the adjustment value to C
    tempSensorAdjC = t;
  } else {
    tempSensorAdjF = t;
    tempSensorAdjC = t * 0.556;  // Convert the adjustent value to F
  }

#if LOG_LEVEL == 6 && !defined(TSEN_DISABLE_LOGGING)
  Log.verbose(F("TSEN: Adjustment values for temp sensor %F C, %F F." CR),
              tempSensorAdjC, tempSensorAdjF);
#endif
}

//
// Retrieving value from sensor, value is in Celcius
//
float TempSensor::getValue() {
#if defined(SIMULATE_TEMP)
  return 21;
#endif

#if defined(USE_GYRO_TEMP)
  // When using the gyro temperature only the first read value will be accurate
  // so we will use this for processing. LOG_PERF_START("temp-get");
  float c = myGyro.getInitialSensorTempC();
  // LOG_PERF_STOP("temp-get");
  hasSensor = true;
  return c;
#if LOG_LEVEL == 6 && !defined(TSEN_DISABLE_LOGGING)
  Log.verbose(F("TSEN: Reciving temp value for gyro sensor %F C." CR), c);
#endif
#else
  // If we dont have sensors just return 0
  if (!mySensors.getDS18Count()) {
    Log.error(F("TSEN: No temperature sensors found. Skipping read." CR));
    return -273;
  }

  // Read the sensors
  // LOG_PERF_START("temp-request");
  mySensors.requestTemperatures();
  // LOG_PERF_STOP("temp-request");

  float c = 0;

  if (mySensors.getDS18Count() >= 1) {
    // LOG_PERF_START("temp-get");
    c = mySensors.getTempCByIndex(0);
    // LOG_PERF_STOP("temp-get");

#if LOG_LEVEL == 6 && !defined(TSEN_DISABLE_LOGGING)
    Log.verbose(F("TSEN: Reciving temp value for sensor %F C." CR), c);
#endif
    hasSensor = true;
  }
  return c;
#endif
}

// EOF
