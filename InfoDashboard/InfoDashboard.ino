#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
#include "WifiParameters.h"

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

///////////////////////////////////////
// format of WifiParameters.h:
//#define WIFI_SSID "xxx"
//#define WIFI_PASSWD "yyy"
///////////////////////////////////////

#define SERVER_IP "192.168.0.20"
#define SERVER_PORT "5000"

SoftwareSerial ESP8266(8, 9); // Rx,  Tx

void setup() {
  Serial.begin(9600);
  ESP8266.begin(9600);
  lcd.begin(20, 4);

  connectToWifi();
  lcd.begin(20, 4);
}

void loop() {
  String result = getInfo();
  Serial.println(result);

  show(result);
  delay(1000);
}

String getInfo() {
  ESP8266.flush();
  ESP8266.println(String("AT+CIPSTART=\"TCP\",\"") + SERVER_IP + String("\",") + SERVER_PORT);
  if(ESP8266.find("Error"))
  {
    Serial.println("AT+CIPSTART error");
    return;
  }

  String resp="";
  String getStr = "GET /info\r\n\r\n";

  ESP8266.println("AT+CIPSEND="+String(getStr.length()));

  delay(200);
  if(ESP8266.find(">"))
  {
    ESP8266.print(getStr);
    Serial.println(getStr);

    long int time = millis();
    long int timeout = 1000;
    int sizeLimit = 256;
    int cnt = 0;
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

  short startIdx = resp.indexOf("+IPD");
  if(startIdx == -1)
    return "Bad response";

  startIdx = resp.indexOf(":", startIdx);
  if(startIdx == -1)
    return "Bad response";

  short endIdx = resp.lastIndexOf("CLOSED");
  if(endIdx == -1)
    return "Bad response";

  resp = resp.substring(startIdx+1, endIdx-1);
  
  return resp;
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

void splitLines(String& data, String &line1, String &line2, char len) {
  if(data.length() <= len) {
    line1 = data;
    line2 = "";
  } else {
    line1 = data.substring(0, len);
    line2 = data.substring(len);
  }
}

void show(String& result) {
  lcd.clear();
  short firstEnd = result.indexOf(";");
  if(firstEnd != -1)
  {
    String line1, line2;

    String part = result.substring(0, firstEnd);
    splitLines(part, line1, line2, 20);
    lcd.setCursor(0,0);
    lcd.print(line1);
    lcd.setCursor(0,1);
    lcd.print(line2);
    
    part = result.substring(firstEnd+1, result.length()-1);
    splitLines(part, line1, line2, 20);
    lcd.setCursor(0,2);
    lcd.print(line1);
    lcd.setCursor(0,3);
    lcd.print(line2);
  } else {
    lcd.print(result);
  }
}
