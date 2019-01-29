#include <SoftwareSerial.h>
#include "DiffEmonLib.h"
#include "WifiParameters.h"

///////////////////////////////////////
// format of WifiParameters.h:
//#define WIFI_SSID "xxx"
//#define WIFI_PASSWD "yyy"
///////////////////////////////////////

#define SERVER_IP "192.168.0.20"
#define SERVER_PORT "5000"

SoftwareSerial ESP8266(2, 3); // Rx,  Tx

typedef struct {
  unsigned char id;
  unsigned char inputPin;
  unsigned char zeroPin;
  unsigned char amps;
  unsigned short calibration;
  char sendLimit;
} MeterParam;

typedef struct {
  MeterParam param;
  DiffEnergyMonitor emon;
  
  double maxMeasure;
  char counter;
} MeterInstance;

MeterInstance meterInstances[2];

void setup() {
  Serial.begin(9600);
  ESP8266.begin(9600);
  
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
  m->param.zeroPin = A2;
  m->param.amps = 30;
  m->param.calibration = 667;
  m->maxMeasure = 0.0;
  m->counter = 0;
  m->param.sendLimit = 2;
  m = &meterInstances[1];
  m->param.id = '1';
  m->param.inputPin = A1;
  m->param.zeroPin = A3;
  m->param.amps = 30;
  m->param.calibration = 667;
  m->maxMeasure = 0.0;
  m->counter = 0;
  m->param.sendLimit = 2;
}

void setupInput() {
  for(unsigned char i = 0;i < sizeof(meterInstances)/sizeof(meterInstances[0]);i++) {
    MeterInstance *m = &meterInstances[i];
    pinMode(m->param.zeroPin, INPUT);
    pinMode(m->param.inputPin, INPUT);
    m->emon.current(m->param.inputPin-A0, m->param.zeroPin-A0,m->param.amps);
  }
}

void processInput(MeterInstance *m) {
  double irms = m->emon.calcIrms(m->param.calibration);
  if(irms > m->maxMeasure) {
    m->maxMeasure=irms;
  }
  m->counter++;
  Serial.println(m->param.inputPin + String(": ") + irms);

  if(m->counter >= m->param.sendLimit) {
    sendMeasurement(m->param.id, m->maxMeasure);
    m->maxMeasure = 0.0;
    m->counter = 0;
  }
}

void sendMeasurement(char id, double measurement) {
  ESP8266.flush();
  ESP8266.println(String("AT+CIPSTART=\"TCP\",\"") + SERVER_IP + String("\",") + SERVER_PORT);
  if(ESP8266.find("Error"))
  {
    Serial.println("AT+CIPSTART error");
    return;
  }

  String getStr = "GET /measure?id="+ String(id)+"&amps="+ String(measurement)+"\r\n\r\n";

  ESP8266.println("AT+CIPSEND="+String(getStr.length()));

  delay(100);
  if(ESP8266.find(">"))
  {
    ESP8266.print(getStr);
    Serial.print(getStr);
    
    long int time = millis();
    long int timeout = 1000;
    int sizeLimit = 256;
    int cnt = 0;
    String resp="";
    while(millis() - time < timeout && cnt < sizeLimit && !resp.endsWith("CLOSED"))
    {
      while (ESP8266.available() > 0 && !resp.endsWith("CLOSED")) 
      {
        char c = ESP8266.read();
        resp += c;
        cnt++;
      }
    }
  }
  else
  {
    Serial.println("AT+CIPCLOSE");
    ESP8266.println("AT+CIPCLOSE");     
  } 
}
