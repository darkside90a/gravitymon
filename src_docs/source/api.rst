.. _rest-api:

REST API
########

All the API's use a key called ``ID`` which is the unique device id (chip id). This is used as an API key when sending requests to the device. 

GET: /api/config
================

Retrieve the current configuration of the device via an HTTP GET command. Payload is in JSON format.

* ``temp-format`` can be either ``C`` or ``F``
* ``gravity-format`` is always ``G`` (plato is not yet supported)
* ``ble`` is used to enable ble data transmission (only on esp32) simulating a tilt. Valid color names are; red, green, black, purple, orange, blue, yellow, pink

Other parameters are the same as in the configuration guide.

.. code-block:: json

   {
      "mdns": "gravmon",
      "id": "ee1bfc",
      "ota-url": "http://192.168.1.50:80/firmware/gravmon/",
      "temp-format": "C",
      "ble": "color",
      "token": "token",
      "token2": "token2",
      "http-push": "http://192.168.1.50:9090/api/v1/Qwerty/telemetry",
      "http-push-h1": "header: value",                           
      "http-push-h2": "header: value",                           
      "http-push2": "http://192.168.1.50/ispindel",
      "http-push2-h1": "header: value",                           
      "http-push2-h2": "header: value",                           
      "http-push3": "http://192.168.1.50/ispindel",
      "influxdb2-push": "http://192.168.1.50:8086",
      "influxdb2-org": "org",
      "influxdb2-bucket": "bucket_id",
      "influxdb2-auth": "token",
      "mqtt-push": "192.168.1.50",
      "mqtt-port": 1883,
      "mqtt-user": "user",
      "mqtt-pass": "pass",
      "sleep-interval": 30,
      "voltage-factor": 1.59,
      "gravity-formula": "0.0*tilt^3+0.0*tilt^2+0.0017978*tilt+0.9436",
      "gravity-format": "G",
      "temp-adjustment-value": 0,
      "gravity-temp-adjustment": false,
      "gyro-temp": true,
      "gyro-calibration-data": {
         "ax": -330,
         "ay": -2249,
         "az": 1170,
         "gx": 99,
         "gy": -6,
         "gz": 4
      },
      "formula-calculation-data": {
         "a1":25,
         "a10":0,
         "g1":1,
         "g10":1
      },
      "angle": 90.93,
      "gravity": 1.105,
      "battery": 0.04,
      "app-ver": "0.1.0",
      "app-build": "build",
      "platform": "esp8266",
      "runtime-average": 3.12
   }


GET: /api/device
================

This API has been removed from 0.9 and merged with /api/status


GET: /api/status
================

Retrieve the current device status via an HTTP GET command. Payload is in JSON format.

* ``temp-format`` can be either ``C`` or ``F``
* ``platform`` can be either ``esp8266``, ``esp32c3``, ``esp32s2``, ``esp32`` or ``esp32lite`` (floaty hardware)
* ``temp-c`` will be set to -273 C if there is no temp sensor
* ``angle`` will be set to 0 if no valid angle is found and -1 if there is no gyro

Other parameters are the same as in the configuration guide.

.. code-block:: json

   {
      "id": "ee1bfc",
      "angle": 89.86,
      "gravity": 1.1052,
      "temp-c": 0,
      "temp-f": 32,
      "battery": 0,
      "wifi-ssid": "connected SSID",
      "temp-format": "C",
      "sleep-mode": false,
      "token": "token",
      "token2": "token2",
      "rssi": -56,
      "app-ver": "0.0.0",
      "app-build": "gitrev",
      "mdns": "gravmon",
      "sleep-interval": 30,
      "platform": "esp8266",
      "runtime-average": 3.12
   }


GET: /api/config/formula
========================

Retrieve the data used for formula calculation data via an HTTP GET command. Payload is in JSON format.

* ``a1``-``a10`` are the angles/tilt readings (up to 10 are currently supported)
* ``g1``-``g10`` are the corresponding gravity readings in SG or Plato depending on the device-format.

.. code-block:: json

   { 
      "id": "ee1bfc",   
      "a1": 22.4,       
      "a2": 54.4, 
      "a3": 58, 
      "a4": 0, 
      "a5": 0, 
      "a6": 0, 
      "a7": 0, 
      "a8": 0, 
      "a9": 0, 
      "a10": 0, 
      "g1": 1.000,      
      "g2": 1.053, 
      "g3": 1.062, 
      "g4": 1, 
      "g5": 1,
      "g6": 1,
      "g7": 1,
      "g8": 1,
      "g9": 1,
      "g10": 1,
      "error": "Potential error message",
      "gravity-format": "G", 
      "gravity-formula": "0.0*tilt^3+0.0*tilt^2+0.0017978*tilt+0.9436"
   }


GET: /api/config/advanced
=========================

Used for adjusting some internal constants and other advanced settings. Should be used with caution.

.. code-block:: json

   {
      "gyro-read-count": 50,
      "gyro-moving-threashold": 500,
      "formula-max-deviation": 3.0,
      "wifi-portal-timeout": 120,
      "wifi-connect-timeout": 20,
      "push-timeout": 10,
      "formula-calibration-temp": 20,
      "int-http1": 0,
      "int-http2": 0,
      "int-http3": 0,
      "int-influx": 0,
      "int-mqtt": 0,
      "tempsensor-resolution": 9,
      "ignore-low-angles": false
   }

POST: /api/config/advanced
==========================

Same parameters as above.

Payload should be in standard format used for posting a form. 

.. note::
  ``ignore-low-angles`` is defined as "on" or "off" when posting since this is the output values 
  from a checkbox, when reading data it's sent as boolean (true,false).

GET: /api/clearwifi
===================

Will reset the wifi settings both in the configuration file and eeprom, leaving the rest of the configuration.

For this to work you will need to supply the device id as a parameter in the request:

:: 

   http://mygravity.local/api/clearwifi?id=<mydeviceid>


GET: /api/factory
=================

Will do a reset to factory defaults and delete all data except wifi settings.

For this to work you will need to supply the device id as a parameter in the request:

:: 

   http://mygravity.local/api/factory?id=<mydeviceid>


GET: /api/test/push
===================

Trigger a push on one of the targets, used to validate the configuration from the UI. 

Requires to parameters to function /api/test/push?id=<deviceid>&format=<format>

* ``format`` defines which endpoint to test, valid values are; http-1, http-2, http-3, influxdb, mqtt

The response is an json message with the following values.

* ``code`` is the return code from the push function, typically http responsecode or error code from mqtt library.

.. code-block:: json

   {
      "success": false,
      "enabled": true,
      "code": -3
   }

POST: /api/config/device
========================

Used to update device settings via an HTTP POST command. 

Payload should be in standard format used for posting a form. Such as as: `id=value&mdns=value` etc. Key value pairs are shown below.

* ``temp-format`` can be either ``C`` (Celsius) or ``F`` (Fahrenheit)

.. code-block:: 

   id=ee1bfc
   mdns=gravmon
   temp-format=C
   sleep-interval=30


POST: /api/config/push
======================

Used to update push settings via an HTTP POST command. Payload is in JSON format.

Payload should be in standard format used for posting a form. Such as as: `id=value&mdns=value` etc. Key value pairs are shown below.

.. code-block::

   id=ee1bfc
   token=
   token2=
   http-push=http://192.168.1.50/ispindel
   http-push2=
   http-push3=
   http-push-h1=
   http-push-h2=
   http-push2-h1=
   http-push2-h2=
   influxdb2-push=http://192.168.1.50:8086
   influxdb2-org=
   influxdb2-bucket=
   influxdb2-auth=
   mqtt-push=192.168.1.50
   mqtt-port=1883
   mqtt-user=
   mqtt-pass=


POST: /api/config/gravity
=========================

Used to update gravity settings via an HTTP POST command. Payload is in JSON format.

* ``gravity-formula`` keywords ``temp`` and ``tilt`` are supported.
* ``gravity-format`` can be either ``G`` (SG) or ``P`` (PLATO)

.. note::
  ``gravity-temp-adjustment`` is defined as "on" or "off" when posting since this is the output values 
  from a checkbox, when reading data it's sent as boolean (true,false).

Payload should be in standard format used for posting a form. Such as as: `id=value&mdns=value` etc. Key value pairs are shown below.

.. code-block:: 

   id=ee1bfc                                              
   gravity-formula=0.0*tilt^3+0.0*tilt^2+0.0017978*tilt+0.9436,
   gravity-format=P
   gravity-temp-adjustment=off                                  


POST: /api/config/hardware
==========================

Used to update hardware settings via an HTTP POST command. Payload is in JSON format.

.. note::
  ``gyro-temp`` is defined as "on" or "off" when posting since this is the output values from a checkbox, when
  reading data it's sent as boolean (true,false).

Payload should be in standard format used for posting a form. Such as as: `id=value&mdns=value` etc. Key value pairs are shown below.

.. code-block:: 

   id=ee1bfc
   voltage-factor=1.59
   temp-adjustment=0 
   ble=red
   gyro-temp=off
   ota-url=http://192.168.1.50/firmware/gravmon/


POST: /api/config/formula
=========================

Used to update formula calculation data via an HTTP POST command. Payload is in JSON format.

* ``a1``-``a10`` are the angles/tilt readings (up to 10 are currently supported)
* ``g1``-``g10`` are the corresponding gravity readings (in SG)

Payload should be in standard format used for posting a form. Such as as: `id=value&mdns=value` etc. Key value pairs are shown below.

.. code-block::

   id=ee1bfc
   a1=22.4
   a2=54.4
   a3=58
   a4=0
   a5=0
   a6=0
   a7=0
   a8=0
   a9=0
   a19=0
   g1=1.000      
   g2=1.053 
   g3=1.062
   g4=1
   g5=1 
   g6=1 
   g7=1 
   g8=1 
   g9=1 
   g10=1 


Calling the API's from Python
=============================

Here is some example code for how to access the API's from a python script. Keys should always be 
present or the API call will fail. You only need to include the parameters you want to change. 

The requests package converts the json to standard form post format. 

.. code-block:: python

   import requests
   import json

   host = "192.168.1.1"           # IP address (or name) of the device to send these settings to
   id = "ee1bfc"                  # Device ID (shown in serial console during startup or in UI)

   def set_config( url, json ):
      headers = { "ContentType": "application/json" }
      print( url )
      resp = requests.post( url, headers=headers, data=json )
      if resp.status_code != 200 :
         print ( "Failed "  )
      else :
         print ( "Success "  )

   url = "http://" + host + "/api/config/device"
   json = { "id": id, 
            "mdns": "gravmon",             # Name of the device
            "temp-format": "C",            # Temperature format C or F
            "sleep-interval": 30           # Sleep interval in seconds
         }
   set_config( url, json )

   url = "http://" + host + "/api/config/push"
   json = { "id": id, 
            "token": "",
            "token2": "",
            "http-push": "http://192.168.1.1/ispindel",  
            "http-push2": "",
            "http-push3": "",
            "http-push-h1": "",
            "http-push-h2": "",
            "http-push2-h1": ""
            "http-push2-h2": "",
            "influxdb2-push": "",
            "influxdb2-org": "",
            "influxdb2-bucket": "",
            "influxdb2-auth": "",
            "mqtt-push": "192.168.1.50",
            "mqtt-port": 1883,
            "mqtt-user": "Qwerty",
            "mqtt-pass": "Qwerty"
            }  
   set_config( url, json )

   url = "http://" + host + "/api/config/gravity"
   json = { "id": id, 
            "gravity-formula": "",                  
            "gravity-format": "P",
            "gravity-temp-adjustment": "off"        # Adjust gravity (on/off)
            }
   set_config( url, json )

   url = "http://" + host + "/api/config/hardware"
   json = { "id": id, 
            "voltage-factor": 1.59,                 # Default value for voltage calculation
            "temp-adjustment": 0,                   # If temp sensor needs to be corrected
            "gyro-temp": "on",                      # Use the temp sensor in the gyro instead (on/off)
            "ble": "red",                           # Enable ble on esp32
            "ota-url": ""                           # if the device should search for a new update when active
         }
   set_config( url, json )

   url = "http://" + host + "/api/formula"
   json = { "id": id, 
            "a1": 22.4, 
            "a2": 54.4, 
            "a3": 58, 
            "a4": 0, 
            "a5": 0, 
            "a6": 0, 
            "a7": 0, 
            "a8": 0, 
            "a9": 0, 
            "a10": 0, 
            "g1": 1.000, 
            "g2": 1.053, 
            "g3": 1.062, 
            "g4": 1, 
            "g5": 1, 
            "g6": 1, 
            "g7": 1, 
            "g8": 1, 
            "g9": 1, 
            "g10": 1 
            }
   set_config( url, json )
