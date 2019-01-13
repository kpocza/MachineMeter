#include <SoftwareSerial.h>
#include <EmonLib.h>
#include "WifiParameters.h"

///////////////////////////////////////
// format of WifiParameters.h:
//#define WIFI_SSID "xxx"
//#define WIFI_PASSWD "yyy"
///////////////////////////////////////

SoftwareSerial ESP8266(2, 3); // Rx,  Tx

typedef struct {
  unsigned char id;
  unsigned char inputPin;
  unsigned char amps;
  unsigned short calibration;
  char sendLimit;
} MeterParam;

typedef struct {
  MeterParam param;
  EnergyMonitor emon;
  
  double sumMeasure;
  char counter;
} MeterInstance;

MeterInstance meterInstances[2];

void setup() {
  Serial.begin(9600);
  ESP8266.begin(115200);
  
  setupParams();
  setupInput();
  connectToWifi();
}

void loop() {
  for(unsigned char i = 0;i < sizeof(meterInstances)/sizeof(meterInstances[0]);i++) {
    processInput(&meterInstances[i]);
  }
  delay(1000);
}

void connectToWifi() {
  Serial.println("Connecting to Wifi");
  ESP8266.println("AT+RST");
  delay(2000);
  
  unsigned char tries=0;
  
  while(true)
  {
    Serial.print(".");
    ESP8266.println(String("AT+CWJAP=\"") + WIFI_SSID + String("\",\"") + WIFI_PASSWD + String("\""));
      
    ESP8266.setTimeout(5000);
    if(ESP8266.find("WIFI CONNECTED\r\n")==1)
    {
      Serial.println("WIFI CONNECTED");
      break;
    }
  }
}

void setupParams() {
  MeterInstance *m = &meterInstances[0];
  m->param.id = '0';
  m->param.inputPin = A0;
  m->param.amps = 30;
  m->param.calibration = 667;
  m->sumMeasure = 0.0;
  m->counter = 0;
  m->param.sendLimit = 2;
  m = &meterInstances[1];
  m->param.id = '1';
  m->param.inputPin = A1;
  m->param.amps = 30;
  m->param.calibration = 667;
  m->sumMeasure = 0.0;
  m->counter = 0;
  m->param.sendLimit = 2;
}

void setupInput() {
  for(unsigned char i = 0;i < sizeof(meterInstances)/sizeof(meterInstances[0]);i++) {
    MeterInstance *m = &meterInstances[i];
    pinMode(m->param.inputPin, INPUT);
    m->emon.current(0, m->param.amps);
  }
}

void processInput(MeterInstance *m) {
  double irms = m->emon.calcIrms(m->param.calibration);
  m->sumMeasure+=irms;
  m->counter++;

  if(m->counter >= m->param.sendLimit) {
    double avg = m->sumMeasure / m->counter;
    m->sumMeasure = 0.0;
    m->counter = 0;
    sendMeasurement(m->param.id, avg);
    Serial.print(m->param.inputPin + String(": "));
    Serial.println(irms);
  }
}

void sendMeasurement(char id, double measurement) {
  ESP8266.flush();
  ESP8266.println("AT+CIPSTART=\"TCP\",\"192.168.0.206\",5000");
  if(ESP8266.find("Error"))
  {
    Serial.println("AT+CIPSTART error");
    return;
  }

  String getStr = "GET /measure?id="+ String(id)+"&amps="+ String(measurement)+"\r\n\r\n";

  ESP8266.println("AT+CIPSEND="+String(getStr.length()));

  if(ESP8266.find(">"))
  {
    ESP8266.print(getStr);
    Serial.println(getStr);
    delay(500);
    while (ESP8266.available()) 
    {
      ESP8266.readStringUntil('\n');
    }
  }
  else
  {
    Serial.println("AT+CIPCLOSE");
    ESP8266.println("AT+CIPCLOSE");     
  } 
}
