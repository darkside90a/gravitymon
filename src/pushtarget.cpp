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
#if defined(ESP8266)
#include <ESP8266mDNS.h>
#else  // defined (ESP32)
#endif
#include <MQTT.h>

#include <config.hpp>
#include <helper.hpp>
#include <pushtarget.hpp>
#include <wifi.hpp>

#define PUSHINT_FILENAME "/push.dat"

//
// Decrease counters
//
void PushIntervalTracker::update(const int index, const int defaultValue) {
  if (_counters[index] <= 0)
    _counters[index] = defaultValue;
  else 
    _counters[index]--;
}

//
// Load data from file
//
void PushIntervalTracker::load() {
  File intFile = LittleFS.open(PUSHINT_FILENAME, "r");
  int i = 0;

  if (intFile) {
    String line = intFile.readStringUntil('\n');
    Log.notice(F("PUSH: Read interval tracker %s." CR), line.c_str());

    char temp[80];
    char *s, *p = &temp[0];
    int i = 0;

    snprintf(&temp[0], sizeof(temp), "%s", line.c_str());
    while ((s = strtok_r(p, ":", &p)) != NULL) {
      _counters[i++] = atoi(s);
    }

    intFile.close();
  }
 
#if !defined(PUSH_DISABLE_LOGGING)
  Log.verbose(F("PUSH: Parsed trackers: %d:%d:%d:%d:%d." CR), _counters[0], _counters[1], _counters[2], _counters[3], _counters[4] );
#endif
}

//
// Update and save counters
//
void PushIntervalTracker::save() {
  update(0, myAdvancedConfig.getPushIntervalHttp1());
  update(1, myAdvancedConfig.getPushIntervalHttp2());
  update(2, myAdvancedConfig.getPushIntervalHttp3());
  update(3, myAdvancedConfig.getPushIntervalInflux());
  update(4, myAdvancedConfig.getPushIntervalMqtt());

  // If this feature is disabled we skip saving the file
  if (!myAdvancedConfig.isPushIntervalActive()) {
#if !defined(PUSH_DISABLE_LOGGING)
    Log.notice(F("PUSH: Variabled push interval disabled." CR));
#endif
    LittleFS.remove(PUSHINT_FILENAME);
  } else {
    Log.notice(F("PUSH: Variabled push interval enabled, updating counters." CR));
    File intFile = LittleFS.open(PUSHINT_FILENAME, "w");

    if (intFile) {
      // Format=http1:http2:http3:influx:mqtt
      intFile.printf("%d:%d:%d:%d:%d\n", _counters[0], _counters[1], _counters[2], _counters[3], _counters[4] );
      intFile.close();
    }
  }
}

//
// Send the data to targets
//
void PushTarget::sendAll(float angle, float gravitySG, float corrGravitySG,
                         float tempC, float runTime) {
  printHeap("PUSH");
  _http.setReuse(false);
  _httpSecure.setReuse(false);

  TemplatingEngine engine;
  engine.initialize(angle, gravitySG, corrGravitySG, tempC, runTime);

  PushIntervalTracker intDelay;
  intDelay.load();

  if (myConfig.isHttpActive() && intDelay.useHttp1()) {
    LOG_PERF_START("push-http");
    sendHttpPost(engine, myConfig.isHttpSSL(), 0);
    LOG_PERF_STOP("push-http");
  }

  if (myConfig.isHttp2Active() && intDelay.useHttp2()) {
    LOG_PERF_START("push-http2");
    sendHttpPost(engine, myConfig.isHttp2SSL(), 1);
    LOG_PERF_STOP("push-http2");
  }

  if (myConfig.isHttp3Active() && intDelay.useHttp3()) {
    LOG_PERF_START("push-http3");
    sendHttpGet(engine, myConfig.isHttp3SSL());
    LOG_PERF_STOP("push-http3");
  }

  if (myConfig.isInfluxDb2Active() && intDelay.useInflux()) {
    LOG_PERF_START("push-influxdb2");
    sendInfluxDb2(engine, myConfig.isInfluxSSL());
    LOG_PERF_STOP("push-influxdb2");
  }

  if (myConfig.isMqttActive() && intDelay.useMqtt()) {
    LOG_PERF_START("push-mqtt");
    sendMqtt(engine, myConfig.isMqttSSL());
    LOG_PERF_STOP("push-mqtt");
  }

  intDelay.save();
}

//
// Send to influx db v2
//
void PushTarget::sendInfluxDb2(TemplatingEngine& engine, bool isSecure) {
#if !defined(PUSH_DISABLE_LOGGING)
  Log.notice(F("PUSH: Sending values to influxdb2." CR));
#endif
  _lastCode = 0;
  _lastSuccess = false;

  String serverPath =
      String(myConfig.getInfluxDb2PushUrl()) +
      "/api/v2/write?org=" + String(myConfig.getInfluxDb2PushOrg()) +
      "&bucket=" + String(myConfig.getInfluxDb2PushBucket());
  String doc = engine.create(TemplatingEngine::TEMPLATE_INFLUX);

  if (isSecure) {
    Log.notice(F("PUSH: InfluxDB, SSL enabled without validation." CR));
    _wifiSecure.setInsecure();

#if defined(ESP8266)
    String host =
        serverPath.substring(8);  // remove the prefix or the probe will fail,
                                  // it needs a pure host name.
    int idx = host.indexOf("/");
    if (idx != -1) host = host.substring(0, idx);

    if (_wifiSecure.probeMaxFragmentLength(host, 443, 512)) {
      Log.notice(F("PUSH: InfluxDB server supports smaller SSL buffer." CR));
      _wifiSecure.setBufferSizes(512, 512);
    }
#endif

    _httpSecure.begin(_wifiSecure, serverPath);
    _httpSecure.setTimeout(myAdvancedConfig.getPushTimeout() * 1000);
    _lastCode = _httpSecure.POST(doc);
  } else  {
    _http.begin(_wifi, serverPath);
    _http.setTimeout(myAdvancedConfig.getPushTimeout() * 1000);
    _lastCode = _http.POST(doc);
  }

#if LOG_LEVEL == 6 && !defined(PUSH_DISABLE_LOGGING)
  Log.verbose(F("PUSH: url %s." CR), serverPath.c_str());
  Log.verbose(F("PUSH: data %s." CR), doc.c_str());
#endif

  String auth = "Token " + String(myConfig.getInfluxDb2PushToken());
  _http.addHeader(F("Authorization"), auth.c_str());
  _lastCode = _http.POST(doc);

  if (_lastCode == 204) {
    _lastSuccess = true;
    Log.notice(F("PUSH: InfluxDB2 push successful, response=%d" CR), _lastCode);
  } else {
    ErrorFileLog errLog;
    errLog.addEntry("PUSH: Influxdb push failed response=" + String(_lastCode));
  }

  if (isSecure) {
    _httpSecure.end();
    _wifiSecure.stop();
  } else {
    _http.end();
    _wifi.stop();
  }
  tcp_cleanup();
}

//
// Add HTTP header to request
//
void PushTarget::addHttpHeader(HTTPClient& http, String header) {
  if (!header.length()) return;

  int i = header.indexOf(":");
  if (i) {
    String name = header.substring(0, i);
    String value = header.substring(i + 1);
    value.trim();
    Log.notice(F("PUSH: Adding header '%s': '%s'" CR), name.c_str(),
               value.c_str());
    http.addHeader(name, value);
  } else {
    ErrorFileLog errLog;
    errLog.addEntry("PUSH: Unable to set header, invalid value " + header);
  }
}

//
// Send data to http target using POST
//
void PushTarget::sendHttpPost(TemplatingEngine& engine, bool isSecure,
                              int index) {
#if !defined(PUSH_DISABLE_LOGGING)
  Log.notice(F("PUSH: Sending values to http (%s)" CR),
             index ? "http2" : "http");
#endif
  _lastCode = 0;
  _lastSuccess = false;

  String serverPath, doc;

  if (index == 0) {
    serverPath = myConfig.getHttpUrl();
    doc = engine.create(TemplatingEngine::TEMPLATE_HTTP1);
  } else {
    serverPath = myConfig.getHttp2Url();
    doc = engine.create(TemplatingEngine::TEMPLATE_HTTP2);
  }

#if LOG_LEVEL == 6 && !defined(PUSH_DISABLE_LOGGING)
  Log.verbose(F("PUSH: url %s." CR), serverPath.c_str());
  Log.verbose(F("PUSH: json %s." CR), doc.c_str());
#endif

  if (isSecure) {
    Log.notice(F("PUSH: HTTP, SSL enabled without validation." CR));
    _wifiSecure.setInsecure();

#if defined(ESP8266)
    String host =
        serverPath.substring(8);  // remove the prefix or the probe will fail,
                                  // it needs a pure host name.
    int idx = host.indexOf("/");
    if (idx != -1) host = host.substring(0, idx);

    if (_wifiSecure.probeMaxFragmentLength(host, 443, 512)) {
      Log.notice(F("PUSH: HTTP server supports smaller SSL buffer." CR));
      _wifiSecure.setBufferSizes(512, 512);
    }
#endif

    _httpSecure.begin(_wifiSecure, serverPath);
    _httpSecure.setTimeout(myAdvancedConfig.getPushTimeout() * 1000);

    if (index == 0) {
      addHttpHeader(_httpSecure, myConfig.getHttpHeader(0));
      addHttpHeader(_httpSecure, myConfig.getHttpHeader(1));
    } else {
      addHttpHeader(_httpSecure, myConfig.getHttp2Header(0));
      addHttpHeader(_httpSecure, myConfig.getHttp2Header(1));
    }

    _lastCode = _httpSecure.POST(doc);
  } else {
    _http.begin(_wifi, serverPath);
    _http.setTimeout(myAdvancedConfig.getPushTimeout() * 1000);

    if (index == 0) {
      addHttpHeader(_http, myConfig.getHttpHeader(0));
      addHttpHeader(_http, myConfig.getHttpHeader(1));
    } else {
      addHttpHeader(_http, myConfig.getHttp2Header(0));
      addHttpHeader(_http, myConfig.getHttp2Header(1));
    }

    _lastCode = _http.POST(doc);
  }

  if (_lastCode == 200) {
    _lastSuccess = true;
    Log.notice(F("PUSH: HTTP post successful, response=%d" CR), _lastCode);
  } else {
    ErrorFileLog errLog;
    errLog.addEntry("PUSH: HTTP post failed response=" + String(_lastCode) +
                    String(index == 0 ? " (http)" : " (http2)"));
  }

  if (isSecure) {
    _httpSecure.end();
    _wifiSecure.stop();
  } else {
    _http.end();
    _wifi.stop();
  }
  tcp_cleanup();
}

//
// Send data to http target using GET
//
void PushTarget::sendHttpGet(TemplatingEngine& engine, bool isSecure) {
#if !defined(PUSH_DISABLE_LOGGING)
  Log.notice(F("PUSH: Sending values to http3" CR));
#endif
  _lastCode = 0;
  _lastSuccess = false;

  String serverPath;

  serverPath = myConfig.getHttp3Url();
  serverPath += engine.create(TemplatingEngine::TEMPLATE_HTTP3);

#if LOG_LEVEL == 6 && !defined(PUSH_DISABLE_LOGGING)
  Log.verbose(F("PUSH: url %s." CR), serverPath.c_str());
#endif

  if (isSecure) {
    Log.notice(F("PUSH: HTTP, SSL enabled without validation." CR));
    _wifiSecure.setInsecure();

#if defined(ESP8266)
    String host =
        serverPath.substring(8);  // remove the prefix or the probe will fail,
                                  // it needs a pure host name.
    int idx = host.indexOf("/");
    if (idx != -1) host = host.substring(0, idx);

    if (_wifiSecure.probeMaxFragmentLength(host, 443, 512)) {
      Log.notice(F("PUSH: HTTP server supports smaller SSL buffer." CR));
      _wifiSecure.setBufferSizes(512, 512);
    }
#endif

    _httpSecure.begin(_wifiSecure, serverPath);
    _httpSecure.setTimeout(myAdvancedConfig.getPushTimeout() * 1000);
    _lastCode = _httpSecure.GET();
  } else {
    _http.begin(_wifi, serverPath);
    _http.setTimeout(myAdvancedConfig.getPushTimeout() * 1000);
    _lastCode = _http.GET();
  }

  if (_lastCode == 200) {
    _lastSuccess = true;
    Log.notice(F("PUSH: HTTP get successful, response=%d" CR), _lastCode);
  } else {
    ErrorFileLog errLog;
    errLog.addEntry("PUSH: HTTP get failed response=" + String(_lastCode));
  }

  if (isSecure) {
    _httpSecure.end();
    _wifiSecure.stop();
  } else {
    _http.end();
    _wifi.stop();
  }
  tcp_cleanup();
}

//
// Send data to mqtt target
//
void PushTarget::sendMqtt(TemplatingEngine& engine, bool isSecure) {
#if !defined(PUSH_DISABLE_LOGGING)
  Log.notice(F("PUSH: Sending values to mqtt." CR));
#endif
  _lastCode = 0;
  _lastSuccess = false;

  MQTTClient mqtt(512);
  String host = myConfig.getMqttUrl();
  String doc = engine.create(TemplatingEngine::TEMPLATE_MQTT);
  int port = myConfig.getMqttPort();

  if (myConfig.isMqttSSL()) {
    Log.notice(F("PUSH: MQTT, SSL enabled without validation." CR));
    _wifiSecure.setInsecure();

#if defined(ESP8266)
    if (_wifiSecure.probeMaxFragmentLength(host, port, 512)) {
      Log.notice(F("PUSH: MQTT server supports smaller SSL buffer." CR));
      _wifiSecure.setBufferSizes(512, 512);
    }
#endif

    mqtt.begin(host.c_str(), port, _wifiSecure);
  } else {
    mqtt.begin(host.c_str(), port, _wifi);
  }

  mqtt.connect(myConfig.getMDNS(), myConfig.getMqttUser(),
               myConfig.getMqttPass());

#if LOG_LEVEL == 6 && !defined(PUSH_DISABLE_LOGGING)
  Log.verbose(F("PUSH: url %s." CR), myConfig.getMqttUrl());
  Log.verbose(F("PUSH: data %s." CR), doc.c_str());
#endif

  // Send MQQT message(s)
  mqtt.setTimeout(myAdvancedConfig.getPushTimeout());  // 10 seconds timeout

  int lines = 1;
  // Find out how many lines are in the document. Each line is one
  // topic/message. | is used as new line.
  for (unsigned int i = 0; i < doc.length() - 1; i++) {
    if (doc.charAt(i) == '|') lines++;
  }

  int index = 0;
  while (lines) {
    int next = doc.indexOf('|', index);
    String line = doc.substring(index, next);

    // Each line equals one topic post, format is <topic>:<value>
    String topic = line.substring(0, line.indexOf(":"));
    String value = line.substring(line.indexOf(":") + 1);
#if LOG_LEVEL == 6 && !defined(PUSH_DISABLE_LOGGING)
    Log.verbose(F("PUSH: topic '%s', value '%s'." CR), topic.c_str(),
                value.c_str());
#endif
    if (mqtt.publish(topic, value)) {
      _lastSuccess = true;
      Log.notice(F("PUSH: MQTT publish successful on %s" CR), topic.c_str());
      _lastCode = 0;
    } else {
      _lastCode = mqtt.lastError();
      ErrorFileLog errLog;
      errLog.addEntry("PUSH: MQTT push on " + topic +
                      " failed error=" + String(mqtt.lastError()));
    }

    index = next + 1;
    lines--;
  }

  mqtt.disconnect();
  if (isSecure) {
    _wifiSecure.stop();
  } else {
    _wifi.stop();
  }
  tcp_cleanup();
}

// EOF
