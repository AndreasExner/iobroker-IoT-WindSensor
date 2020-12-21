/*

  Wind speed and direction sensor for iobroker IoT Framework

  Version: F5_1.1 
  Date: 2020-12-21


  History:
  
  Version: F5_1.1 2020-12-14

  - Major bugfix: sketch does not work with debug=false
  - Major improvements and bugfixes for the UART/RS485 communication: CRC check, remove zero bytes etc
  - changed wind speed data transfer (for de-noising)
  - minor fixes and improvements

  Version: F5_1.0 (release) 2020-12-06
  

  This sketch has several prerquisites discribed in the documentation of the repository:
  https://github.com/AndreasExner/ioBroker-IoT-WindSensor

  This sketch is based on my ioBroker IoT Framework V5.3.2 (or higher)
  https://github.com/AndreasExner/ioBroker-IoT-Framework


  MIT License

  Copyright (c) 2020 Andreas Exner

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

//+++++++++++++++++++++++++++++++++++++++++ enable sensor specific functions +++++++++++++++++++++++++++++++++++++++++++++++++

#define AEX_iobroker_IoT_Framework //generic functions DO NOT CHANGE

// uncomment required sections

//#define BME280_active
//#define BME680_active
//#define SCD30_active
//#define SPS30_active
#define WindSensor_active
//#define ePaper_active

//+++++++++++++++++++++++++++++++++++++++++ generic device section +++++++++++++++++++++++++++++++++++++++++++++++++

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// device settings - change settings to match your requirements

const char* ssid     = "<ssid>"; // Wifi SSID
const char* password = "<password>"; //Wifi password

String SensorID = "07"; //predefinded sensor ID, DEV by default to prevent overwriting productive data

int interval = 10;  // waiting time for the first masurement and fallback on error reading interval from iobroker

/*
   The BSEC library sets the pace for the loop duration -> BSEC_LP ~ 3000 ms per request
   For a transmission interval of 5 minutes configure an interval of 100 (*3 = 300 seconds = 5 minutes)
*/

bool DevMode = true; //enable DEV mode on boot (do not change)
bool debug = true; //debug to serial monitor
bool led = true; //enable external status LED on boot
bool sensor_active = false; // dectivate sensor(s) on boot (do not change)

/*
   build base URL's
   Change IP/FQND and path to match your environment
*/

String baseURL_DEVICE_GET = "http://192.168.1.240:8087/getPlainValue/0_userdata.0.IoT-Devices." + SensorID + ".";
String baseURL_DEVICE_SET = "http://192.168.1.240:8087/set/0_userdata.0.IoT-Devices." + SensorID + ".";

// end of device settings - don not change anything below the line until required

// define generic device URL's

String URL_IP = baseURL_DEVICE_SET + "SensorIP?value=";
String URL_RST = baseURL_DEVICE_SET + "Reset?value=";
String URL_LED = baseURL_DEVICE_GET + "LED";
String URL_MAC = baseURL_DEVICE_SET + "MAC?value=";
String URL_RSSI = baseURL_DEVICE_SET + "RSSI?value=";
String URL_DevMode = baseURL_DEVICE_GET + "DevMode";
String URL_ErrorLog = baseURL_DEVICE_SET + "ErrorLog?value=";
String URL_sensor_active = baseURL_DEVICE_GET + "SensorActive";

String baseURL_DATA_GET, baseURL_DATA_SET; // URL's for data
String URL_SID, URL_INT; // URL's for sensor ID and interval

// other definitions

#define LED D3 // gpio pin for external status LED
void(* HWReset) (void) = 0; // define reset function DO NOT CHANGE
int counter = interval;  // countdown for next interval

//+++++++++++++++++++++++++++++++++++++++++ wind sensor section +++++++++++++++++++++++++++++++++++++++++++++++++

int analog_Pin = A0;
double WindSensor_A0_Step_Voltage = 0.03; //default for 10V signal
int crcErrors, rxTimeOuts;

String URL_A0_Step_Voltage, URL_crcErrors, URL_rxTimeOuts, URL_WindSpeed, URL_WindDirection, URL_WindSpeedArray, URL_WindDirectionArray;

int WindSensor_Direction; //buffer for the case of CRC errors oder timeouts
double WindSpeedArray[250]; //the array sizes MUST be >= interval, otherwise the code will fail during runtime! DO NOT exceed 250, otherwise the URL can be > 2000 chars
int WindDirectionArray[250];  //the array sizes MUST be >= interval, otherwise the code will fail during runtime! DO NOT exceed 250, otherwise the URL can be > 2000 chars

#define SERIAL_BAUD_RATE 9600 // baud rate for wind sensor DO NOT CHANGE
#define DEBUG_BAUD_RATE 115200 // baud rate for debug via serial1

#define RTS D5  // RS485 bus master
int expectedFrameSize = 9;  // expected RX frame size

byte request_buffer[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x0B};  // TX frame to request wind direction
int request_buffer_length = 8; // TX frame length

//######################################### setup ##############################################################

void setup(void) {

  Serial1.begin(DEBUG_BAUD_RATE);
  delay(1000);

  Serial.begin(SERIAL_BAUD_RATE);
  delay(1000);

  pinMode(LED, OUTPUT);

  connect_wifi();
  get_wifi_state();

  // initial communication with iobroker on boot
  
  bool debug_state = debug;  //debug output during setup
  debug = true;

  get_dynamic_config();
  build_urls();
  WindSensor_get_config();
  send_ip();
  send_sid();
  send_rst();

  debug = debug_state;

  // setup sensors

  if (sensor_active) {WindDirection_setup();}

}

//######################################### specific device functions #######################################################

void build_urls() {

  URL_SID = baseURL_DATA_SET + "SensorID?value=" + SensorID;
  URL_INT = baseURL_DATA_GET + "Interval";

  URL_A0_Step_Voltage = baseURL_DATA_GET + "A0_Step_Voltage";
  URL_crcErrors = baseURL_DATA_SET + "crcErrors?value=";
  URL_rxTimeOuts = baseURL_DATA_SET + "rxTimeOuts?value=";
  URL_WindDirectionArray = baseURL_DATA_SET + "WindDirectionArray?value=";
  URL_WindSpeedArray = baseURL_DATA_SET + "WindSpeedArray?value=";
}

void send_data() {

  Serial1.println("send data to iobroker");

    HTTPClient http;

    String sendURL;

    sendURL = URL_crcErrors;
    sendURL += String(crcErrors);
    http.begin(sendURL);
    http.GET();
    
    sendURL = URL_rxTimeOuts;
    sendURL += String(rxTimeOuts);
    http.begin(sendURL);
    http.GET();
    
    sendURL = URL_WindSpeedArray;
    sendURL += String(WindSpeedArray[0]);
    for (int i = 1; i < interval; i++) {

      sendURL += ",";
      sendURL += String(WindSpeedArray[i]);
    }
    http.begin(sendURL);
    http.GET();

    sendURL = URL_WindDirectionArray;
    sendURL += String(WindDirectionArray[0]);
    for (int i = 1; i < interval; i++) {

      sendURL += ",";
      sendURL += String(WindDirectionArray[i]);
    }
    http.begin(sendURL);
    http.GET();

    http.end();
}


//####################################################################
// Loop

void loop(void) {

  int Analog_Input;

  WindSpeed_get_data();
  WindDirection_get_data();
  WindSensor_serial_output();

  if (counter == 0) {

    get_wifi_state();
    send_ip();

    get_dynamic_config();
    build_urls();
    send_data();
    WindSensor_get_config();
    
    if (sensor_active) {WindDirection_setup();}
    
    get_interval();
    counter = interval;
    crcErrors = 0;
    rxTimeOuts = 0;
  }
  else {
    counter--;
    Serial1.println(counter);
  }

  // Blink LED

  if (led && (counter % 2)) {
    digitalWrite(LED, LOW);
  }
  else {
    digitalWrite(LED, HIGH);
  }

  delay(1000);
}
