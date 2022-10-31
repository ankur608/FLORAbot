/***************************************Licenses**************************************
Florabot code is released under the [MIT License](http://opensource.org/licenses/MIT).
*************************************************************************************/

#include "WizFi360.h"
#include <ArduinoJson.h>
#include <Wire.h>
#include <SparkFunBME280.h>
#include <SparkFunCCS811.h>
#define CCS811_ADDR 0x5B //Default I2C Address
#define PIN_NOT_WAKE 5
//Global sensor objects
CCS811 myCCS811(CCS811_ADDR);
BME280 myBME280;

// User settings to configure Sensor Calibration data
String state =""; String plant_mood =""; 
float sen_voltage, vol_water_cont; // preallocate to approx. voltage and theta_v
float slope = 2.48; // slope from linear fit from pre-calib. data
float intercept = -0.72; // intercept from linear fit from pre-calib. data
float rounded_vwc, rounded_sv;
/* Wi-Fi info */
char ssid[] = "your-ssid";
char pass[] = "your-network-password";

/* MQTT credential info */
String mqttUserName="florabot";
String mqttPassword="florabot";
String mqttClientID="FloraBot";
String mqttAliveTime="60";
String mqttPublishTopic="sensors";
String mqttSubscribeTopic="plant2";
String mqttServer="192.168.xx.xxx";  //eg.*192.168.93.112*
String mqttPort="1883";

/* WizFi360 */
#define WIZFI360_EVB_PICO
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial2(6, 7); // RX, TX
#endif

/* Baudrate */
#define SERIAL_BAUDRATE   115200
#define SERIAL2_BAUDRATE  115200

int status = WL_IDLE_STATUS;  // the Wifi radio's status

unsigned long lastMillis = 0;

void setup() {
  // initialize serial for debugging
  Serial.begin(SERIAL_BAUDRATE);
  // initialize serial for WizFi360 module
  Serial2.begin(SERIAL2_BAUDRATE);
  // initialize WizFi360 module
  WiFi.init(&Serial2);
  Serial.println("...FLORAbot v1 Sensor Instance...");
  
  // check for the presence of WiFi function.
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi functionality not present");
    // don't continue
    while (true);
  }
  
  //Initialize Sparkfun Sensor Event
  Wire.setSDA(12);  //Connect to respective GPIO on WizFi360-EVB-Pico
  Wire.setSCL(13);  //Connect to respective GPIO on WizFi360-EVB-Pico
  Wire.begin();

  //This begins the CCS811 sensor and prints error status of .begin()
  CCS811Core::status returnCode = myCCS811.begin();
  Serial.print("CCS811 begin exited with: ");
  //Pass the error code to a function to print the results
  printDriverError( returnCode );
  Serial.println();

  //Initialize BME280
  //For I2C, enable the following and disable the SPI section
  myBME280.settings.commInterface = I2C_MODE;
  myBME280.settings.I2CAddress = 0x77;
  myBME280.settings.runMode = 3; //Normal mode
  myBME280.settings.tStandby = 0;
  myBME280.settings.filter = 4;
  myBME280.settings.tempOverSample = 5;
  myBME280.settings.pressOverSample = 5;
  myBME280.settings.humidOverSample = 5;
  delay(10);  //Make sure sensor had enough time to turn on. BME280 requires 2ms to start up.
  myBME280.begin();

  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }
  Serial.println("You're connected to the network");
  Network_detail();
  getWizFi360_mac();
  Serial.println("Connecting to MQTT broker...");
  init_MQTTconn();
}

void loop() {
  StaticJsonDocument<240> sensor;  //Allocated 240 bytes to JSON.
  // Publish message every 4 seconds
    if (millis() - lastMillis > 4000) {  
    
    // get sensor data
    S_EVENT();
    delay(1000);
    VWC();
    delay(200);
    UVI();
    delay(500);

    //Formating data.
    int co2 = myCCS811.getCO2();
    int voc = myCCS811.getTVOC();
    int t = myBME280.readTempC();
    int h = myBME280.readFloatHumidity();

    //calculate plant mood index (PMI) based on sensor-fusion approximation.//TODO.
    plant_mood = "Good"; //Static string.

    // set sensor packet in MQTT message.
    sensor["CO2"] = co2;
    sensor["TVOC"] = voc;
    sensor["Temp"] = t;
    sensor["RH"] = h;
    sensor["SV"] = sen_voltage;
    sensor["VWC"] = vol_water_cont;
    sensor["UVI"] = String(state);
    sensor["PMI"] = String(plant_mood);
    
    String buffer;
    size_t n = serializeJson(sensor, buffer);
    sendData("AT+MQTTPUB=\"" + buffer + "\"","OK",1500,false);
    lastMillis = millis();
  }
}

void Network_detail() {
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to
  byte bssid[6];
  WiFi.BSSID(bssid);
  char buf[20];
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", bssid[5], bssid[4], bssid[3], bssid[2], bssid[1], bssid[0]);
  Serial.print("BSSID: ");
  Serial.println(buf);

  // print received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI): ");
  Serial.println(rssi);
}

void getWizFi360_mac() {
  // print EVB-Pico WiFi IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print MAC address
  byte mac[6];
  WiFi.macAddress(mac);
  char buf[20];
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
  Serial.print("MAC address: ");
  Serial.println(buf);
}

void init_MQTTconn() {
  if (sendData("AT+MQTTSET=\"" + mqttUserName + "\",\"" + mqttPassword + "\",\"" + mqttClientID + "\"," + mqttAliveTime + "","OK",1500,false)) {
    Serial.println("SET: OK");
    if (sendData("AT+MQTTTOPIC=\"" + mqttPublishTopic + "\",\"" + mqttSubscribeTopic + "\"","OK",1500,false)) {
      Serial.println("TOPIC: OK");
      if (sendData("AT+MQTTCON=0,0,\"" + mqttServer + "\"," + mqttPort + "","0,CONNECT",2500,false)) {
        Serial.println("CONN: OK");
      } else {
        Serial.println("CONN: ERROR");
      }
    }
    else {
      Serial.println("CONN: ERROR");
    }
  } else {
    Serial.println("SET: ERROR");
  }
}

boolean sendData(String command, String key, int timeout, boolean debug)
{
    String result = "";
    boolean success = false;
    // clean Serial2 buffer from previous comms
    while(Serial2.available())
      Serial2.read();
    // send the new command    
    Serial2.print(command + "\r\n");
    // and wait until sending has finished
    Serial2.flush();
    // calculate backoffTime
    long backoffTime = millis() + timeout;
    // try to get an answer  
    while(backoffTime > millis())
    {
      // if new data on serial
      while(Serial2.available())
      {
        // read the next character
        char c = Serial2.read();
        // and add to result string 
        result+=c;
      }
      // if the result string contains they keyword we are looking for, exit
      if(result.indexOf(key) >= 0) {
        while(Serial2.available())
          Serial2.read();
        delay(25);
        backoffTime = millis();
        success = true;
      } else {
        // else, keep on waiting
        success = false;       
      }     
    }

    if(debug)
    {
      if(result.indexOf(key) >= 0) {
        Serial.println(":yes:"+result);
      } else {
        Serial.println(":no:"+result);
      }
    }
    return success;
}
//-------->>

//Init. SENSOR-EVENT
void S_EVENT()
{
  if (myCCS811.dataAvailable())
  {
    //Calling this function updates the global tVOC and eCO2 variables
    myCCS811.readAlgorithmResults();    
    printInfoSerial();  //printInfoSerial fetches the values of tVOC and eCO2

    float BMEtempC = myBME280.readTempC();
    float BMEhumid = myBME280.readFloatHumidity();

    Serial.print("Applying new values (deg C, %): ");
    Serial.print(BMEtempC);
    Serial.print(",");
    Serial.println(BMEhumid);
    Serial.println();
    myCCS811.setEnvironmentalData(BMEhumid, BMEtempC);  //Send temperature data to the CCS811
  }
  else if (myCCS811.checkForStatusError())
  {
    printSensorError(); //If the CCS811 found an internal error, print it.
  }
}
//-------->>

void printInfoSerial()
{
  //getCO2() gets the previously read data from the library
  Serial.println("CCS811 data:");
  Serial.print(" CO2 concentration : ");
  Serial.print(myCCS811.getCO2());
  Serial.println(" ppm");

  //getTVOC() gets the previously read data from the library
  Serial.print(" TVOC concentration : ");
  Serial.print(myCCS811.getTVOC());
  Serial.println(" ppb");

  Serial.println("BME280 data:");
  Serial.print(" Temperature: ");
  Serial.print(myBME280.readTempC(), 2);
  Serial.println(" degrees C");

  Serial.print(" %RH: ");
  Serial.print(myBME280.readFloatHumidity(), 2);
  Serial.println(" %");

  Serial.println();
}
//-------->>

//printDriverError decodes the CCS811Core::status type and prints the
//type of error to the serial terminal.
//Save the return value of any function of type CCS811Core::status, then pass
//to this function to see what the output was.
void printDriverError( CCS811Core::status errorCode )
{
  switch ( errorCode )
  {
    case CCS811Core::SENSOR_SUCCESS:
      Serial.print("SUCCESS");
      break;
    case CCS811Core::SENSOR_ID_ERROR:
      Serial.print("ID_ERROR");
      break;
    case CCS811Core::SENSOR_I2C_ERROR:
      Serial.print("I2C_ERROR");
      break;
    case CCS811Core::SENSOR_INTERNAL_ERROR:
      Serial.print("INTERNAL_ERROR");
      break;
    case CCS811Core::SENSOR_GENERIC_ERROR:
      Serial.print("GENERIC_ERROR");
      break;
    default:
      Serial.print("Unspecified error.");
  }
}

//printSensorError gets, clears, then prints the errors
//saved within the error register.
void printSensorError()
{
  uint8_t error = myCCS811.getErrorRegister();

  if ( error == 0xFF ) //comm error
  {
    Serial.println("Failed to get ERROR_ID register.");
  }
  else
  {
    Serial.print("Error: ");
    if (error & 1 << 5) Serial.print("HeaterSupply");
    if (error & 1 << 4) Serial.print("HeaterFault");
    if (error & 1 << 3) Serial.print("MaxResistance");
    if (error & 1 << 2) Serial.print("MeasModeInvalid");
    if (error & 1 << 1) Serial.print("ReadRegInvalid");
    if (error & 1 << 0) Serial.print("MsgInvalid");
    Serial.println();
  }
}
//------->>

//Function to return calculated UV-Index from GUVA-S12SD sensor.
void UVI()
{
  float Voltage;
  int UV_index;
  
  Voltage = (float(analogRead(A1))/4095.0)*3.3;
  UV_index= Voltage/0.1;

 //Get UVI_STATE
 if(UV_index<=2){ 
   state = "LOW";
  }
  else if(UV_index > 2 && UV_index <=5){
   state = "MID";
  }
  else if(UV_index>5 && UV_index<=7){
   state = "HIGH";
  }
  else if(UV_index>7 && UV_index<=10){
   state = "EXTREME";

  }
  else{
   state = "FATAL";
  }
}
//-------->>

//Function to calculate volumetric water content 
//using Capacitive Soil Moisture Sensor v1.2
void VWC()
{
  sen_voltage = (float(analogRead(A0))/4095.0)*3.3;
  vol_water_cont = ((1.0/sen_voltage)*slope)+intercept; // calc of theta_v (vol. water content)
  delay(100); // slight delay between readings
}
