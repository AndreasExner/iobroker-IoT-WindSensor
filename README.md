# ioBroker-IoT-WindSensor

**Important: This project based on my ioBroker IoT Framework V5**

[AndreasExner/ioBroker-IoT-Framework: ioBroker IoT Framework (based on NodeMCU ESP8266) (github.com)](https://github.com/AndreasExner/ioBroker-IoT-Framework)

**Please refer to this documentation for the the basic setup and configuration.**

Required Version: 5.3.2 (or higher)



## Description

#### Introduction

This sketch reads the wind speed and direction from devices similar to these (**no** affiliate links!):

[Broco Garten Signalausgang Aluminiumlegierung Windrichtung Sensor Windfahne: AmazonSmile: Küche & Haushalt](https://smile.amazon.de/gp/product/B081NNH9RX/ref=ppx_yo_dt_b_asin_title_o04_s00?ie=UTF8&psc=1)

[Rosilesi Windgeschwindigkeitssensor - Impulssignalausgang Aluminiumlegierter Windgeschwindigkeitssensor Anemometer: AmazonSmile: Garten](https://smile.amazon.de/gp/product/B08J4DLPBV/ref=ppx_yo_dt_b_asin_title_o05_s00?ie=UTF8&psc=1)

Both are sold from different sellers and under different names as well. Since the wind speed sensor offers an analog output the wind direction sensor uses the RS486 interface.

#### De noising

The raw data will be send to the ioborker, because de nosing can be a challenging part of the this project. 

For the wind speed and direction, the value is measured with round about 1 Hz and written into an array. The array is submitted to iobroker on every interval. The maximum size of the array is limited by the maximum length of an URL (2000 characters) to 250. The recommended interval for a "real time" display should be between 30 and 60. On the iobroker side, the array must be processed do extract the required information. 



#### Wiring

Both sensors require 12-24V DC power. The RS485 shield is used to convert RS485 into TTL. 

<img src="https://github.com/AndreasExner/ioBroker-IoT-WindSensor/blob/main/WindSensor_Steckplatine.png?raw=true" style="zoom: 50%;" />

Voltage divider resistors: 330 ohm and 150 ohm, 1% or better, 1/4 watts



## History

**Version: F5_1.1 2020-12-14**

- Major bugfix: sketch does not work with debug=false
- added CRC check for UART/RS485
- changed wind speed data transfer (for de-noising)
- minor fixes and improvements



**Version: F5_1.0 (release) 2020-12-06**



#### Tested environment

- Software
  - Arduino IDE 1.8.13 (Windows)
  - EspSoftwareSerial 6.10.0 [plerup/espsoftwareserial: Implementation of the Arduino software serial for ESP8266 (github.com)](https://github.com/plerup/espsoftwareserial)
- Hardware
  - NodeMCU Lolin V3 (ESP8266MOD 12-F)
  - NodeMCU D1 Mini (ESP8266MOD 12-F)
  - 5 V MAX485 / RS485 Modul TTL to RS-485 MCU



## Prerequisites

* You need a running Arduino IDE and at least basic knowledge how it works. 
* You also need a running ioBroker instance and a little more than basic knowledge on that stuff.
* You need a REST API in your ioBroker setup
* You need to **secure** the REST API (for example, expose the required data only and hide any confidential and secret stuff)
* You need a userdata section in your ioBroker objects
* You should know how to work with IoT devices and electronics
* You need to read this first: [AndreasExner/ioBroker-IoT-Framework: ioBroker IoT Framework (based on NodeMCU ESP8266) (github.com)](https://github.com/AndreasExner/ioBroker-IoT-Framework)



## Setup

- Create a folder in your Arduino library folder
- Copy the primary sketch (e.g. WindSensor.ino) and the extension file (AEX_iobroker_IoT_Framework.ino) into the folder
- Open the primary sketch (e.g. WindSensor.ino) 
- **Install required libraries into your Arduino IDE**
- Create (import) the datapoints in iobroker
  - 0_userdata.0.IoT-Devices.07.json (generic device configuration and monitoring)
  - 0_userdata.0.IoT.WindSensor.json (specific device configuration and data, production)
  - 0_userdata.0.IoT-Dev.WindSensor.json (specific device configuration and data, development, optional)
- Set values for datapoints (see iobroker datapoints)



## Configuration

#### Generic device section

```c++
// device settings - change settings to match your requirements

const char* ssid     = "<ssid>"; // Wifi SSID
const char* password = "<password>"; //Wifi password

String SensorID = "DEV"; //predefinded sensor ID, DEV by default to prevent overwriting productive data

int interval = 10;  // waiting time for the first masurement and fallback on error reading interval from iobroker

bool DevMode = true; //enable DEV mode on boot (do not change)
bool debug = true; //debug to serial monitor
bool led = true; //enable external status LED on boot
bool sensor_active = false; // dectivate sensor(s) on boot (do not change)
```

- Enter proper Wifi information
- The **`SensorID`** is part of the URL's and important for the the iobroker communications
- **`Interval`** defines the delay between two data transmissions / measurements. This value is used initially after boot. The interval dynamically updates from iobroker
- The **`DevMode`** switch prevents the device from sending data into your productive datapoints. It is enabled by default and can be overwritten dynamically from iobroker
- **`debug`** enables a more detailed serial output
- **`led`** enables the onboard led (status)
- The **`sensor_active`** switch disables the loading of hardware specific code on startup. This is very useful to test a sketch on the bread board without the connected hardware. It is disabled by default and gets dynamically enabled from the iobrocker during boot, as long as nothing else is configured.



#### Base URL's

```c++
/*
 * build base URL's
 * Change IP/FQND and path to match your environment
 */

String baseURL_DEVICE_GET = "http://192.168.1.240:8087/getPlainValue/0_userdata.0.IoT-Devices." + SensorID + ".";
String baseURL_DEVICE_SET = "http://192.168.1.240:8087/set/0_userdata.0.IoT-Devices." + SensorID + ".";
```

The base url's, one for read and one to write data, are pointing to your iobroker datapoints in the devices section. The SensorID will be added to complete the path. 



## iobroker datapoints

#### Devices section

The default path for the devices root folder is: **`0_userdata.0.IoT-Devices`**. When the path is changed, it has to be changed in the sketch as well.

**It is mandatory to setup the following datapoints prior the first device boot:**

- **`DevMode`** If true, the baseURL's for the IoT-Dev section are used to prevent overwriting your production data. Also see generic device section.

- **`LED`** Controls the status LED. Also see generic device section.

- **`SensorActive`** Controls the hardware sensors. Also see generic device section.

- **`SensorName`** Easy to understand name for the sensor. Not used in the code.

- **`baseURL_GET_DEV`** points to the IoT-Dev datapoints in iobroker (e.g. 0_userdata.0.IoT-Dev.DEV)

  - ```
    http://192.168.1.240:8087/getPlainValue/0_userdata.0.IoT-Dev.WindSensor.
    ```

- **`baseURL_SET_DEV`** points to the IoT-Dev datapoints in iobroker (e.g. 0_userdata.0.IoT-Dev.DEV)

  - ```
    http://192.168.1.240:8087/set/0_userdata.0.IoT-Dev.WindSensor.
    ```

- **`baseURL_GET_PROD`** points to the IoT datapoints in iobroker (e.g. 0_userdata.0.IoT.Weather)

  - ```
    http://192.168.1.240:8087/getPlainValue/0_userdata.0.IoT.WindSensor.
    ```

- **`baseURL_SET_PROD`** points to the IoT datapoints in iobroker (e.g. 0_userdata.0.IoT.Weather)

  - ```
    http://192.168.1.240:8087/set/0_userdata.0.IoT.WindSensor.
    ```



#### Specific device configuration and data

Depending on the **`DevMode`**, the device reads config and writes data into different datapoint sections:

- Development root folder: **`0_userdata.0.IoT-Dev`**
- Production root folder: **`0_userdata.0.IoT`**

It is recommended to keep the datapoints in both sections identical to avoid errors when changing the **`DevMode`**. The values can be different. Setup these datapoints before boot up the device:

- **`Interval`** [10] Defines the delay (in seconds) between two data transmissions 
- **`A0_Step_Voltage`** The voltage per step of the A/D converter for the  windspeed sensor. Due to the line resistance it might be necessary to adjust these values individually. A good point to start is 0.03 V.

These datapoints are for output only:

- **`WindDirectionArray`** Array of wind direction values
- **`WindSpeedArray`** Array of wind speed values
- **`SensorID`** Sensor device ID



## How it works

#### Boot phase / setup

- Connect Wifi
- Get Wifi State
- Get the dynamic configuration from iobroker (generic devices section)
- Build specific device URL's (based on the dynamic configuration)
- Send devices IP address
- Send device ID
- Send information about the last restart / reset
- Run's setup for active sensors



#### Loop

Every n-th tick, defined by the **`Interval`**,  the following sequence will proceed:

- Loop (1 Hz)
  - Get data of all sensors
  - Serial output all sensor data
  - Interval reached
    - Get Wifi State (try reconnect first, then reset if not connected)
    - Send devices IP address
    - Get the dynamic configuration from iobroker (generic devices section)
    - Build specific device URL's (based on the dynamic configuration)
    - Send data to iobroker
    - Get configurationdata (A0_Step_Voltage) for wind sensor from iobroker
    - Run setup for inactive sensors (if activated now)
    - Get the new interval (specific device section)
  - LED Blink



## Appendix
