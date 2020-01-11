#include <SoftwareSerial.h>
#include "WifiParameters.h"

#define SERVER_IP "192.168.0.20"
#define SERVER_PORT "5001"

#define RELAY_PIN 10

SoftwareSerial ESP8266(8, 9); // Rx,  Tx

void setup() {
  setCirculator(false);
  pinMode(RELAY_PIN, OUTPUT);
  
  Serial.begin(9600);
  ESP8266.begin(9600);

  connectToWifi();
  delay(1000);
}

void loop() {
  String result = getInfo();
  Serial.println(result);

  setCirculator(result == "1");

  delay(10000);
}

const String errorServerNetwork = "-1";
const String errorComm = "-2";
const String badResponse1 = "-3";
const String badResponse2 = "-4";
const String tcpStart = String("AT+CIPSTART=\"TCP\",\"") + SERVER_IP + String("\",") + SERVER_PORT;

String getInfo() {
  ESP8266.flush();
  ESP8266.println(tcpStart);
  delay(10);
  if(ESP8266.find("ERROR"))
  {
    Serial.println("AT+CIPSTART error");
    return errorServerNetwork;
  }

  const String getStr = "GET /info HTTP/1.0\r\n\r\n";

  ESP8266.println("AT+CIPSEND="+String(getStr.length()));

  delay(100);
  if(!ESP8266.find(">"))
  {
    Serial.println("AT+CIPCLOSE");
    ESP8266.println("AT+CIPCLOSE");     
    return errorComm;
  }
  
  ESP8266.print(getStr);
  //Serial.println(getStr);

  const String closed = "CLOSED";
  const long int timeout = 1000;
  const int sizeLimit = 300;

  String resp;
  resp.reserve(sizeLimit);

  long int time = millis();
  int cnt = 0;
  while(millis() - time < timeout && cnt < sizeLimit && !resp.endsWith(closed))
  {
    while (ESP8266.available() > 0 && cnt < sizeLimit && !resp.endsWith(closed)) 
    {
      char c = ESP8266.read();
      resp += c;
      cnt++;
    }
  }

  //Serial.println(resp);
  const String ipd = "+IPD";
  const String colon = ":";

  short ipdIndex = -1;
  short lastStartIndex = -1;
  short idxIndex = -1;
  short rounds = 0;
  do {
    ipdIndex = resp.indexOf(ipd, lastStartIndex!= -1 ? lastStartIndex : 0);
    //Serial.println(ipdIndex);
    if(ipdIndex != -1)
    {
      short startIdx = resp.indexOf(colon, ipdIndex);
      resp.remove(ipdIndex, startIdx - ipdIndex + 1);
    }
    rounds++;
    if(rounds > 5)
      return badResponse1;
  } while(ipdIndex!= -1);
  
  short endIdx = resp.lastIndexOf(closed);
  if(endIdx!= -1) {
    resp.remove(endIdx);
  }

  short contentIdx = resp.lastIndexOf("\r\n\r\n");
  if(contentIdx == -1)
    return badResponse2;

  resp.remove(0, contentIdx+4);
  //Serial.println(resp);

  return resp;
}
void setCirculator(bool state) {
    digitalWrite(RELAY_PIN, state ? LOW : HIGH);
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
