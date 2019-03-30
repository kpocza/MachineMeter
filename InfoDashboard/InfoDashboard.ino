#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
#include "WifiParameters.h"

// test
// LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// prod
 LiquidCrystal lcd(2, 3, 4, 5, 11, 12);

///////////////////////////////////////
// format of WifiParameters.h:
//#define WIFI_SSID "xxx"
//#define WIFI_PASSWD "yyy"
///////////////////////////////////////

#define SERVER_IP "192.168.0.20"
#define SERVER_PORT "5000"

SoftwareSerial ESP8266(8, 9); // Rx,  Tx
int counter;
int beginCounter;
String oldResult;

void setup() {
  Serial.begin(9600);
  ESP8266.begin(9600);
  lcd.begin(20, 4);
  lcd.setCursor(0,0);
  lcd.print("Connecting");
  
  connectToWifi();
  lcd.begin(20, 4);
  lcd.setCursor(0,0);
  lcd.print("Connected ");

  counter = 0;
  beginCounter = 0;
  oldResult = "";
  delay(1000);
}


void loop() {
  String result = getInfo();
  Serial.println(result);

  if(result!= oldResult || counter%3 == 0)
  {
    if(beginCounter%3 == 0) {
      lcd.begin(20, 4);
    }
    show(result);
    beginCounter++;
  }

  counter++;
  oldResult = result;
  
  delay(10000);
}

String getInfo() {
  ESP8266.flush();
  ESP8266.println(String("AT+CIPSTART=\"TCP\",\"") + SERVER_IP + String("\",") + SERVER_PORT);
  delay(10);
  if(ESP8266.find("ERROR"))
  {
    Serial.println("AT+CIPSTART error");
    return "Server/network error";
  }

  String resp="";
  String getStr = "GET /info HTTP/1.0\r\n\r\n";

  ESP8266.println("AT+CIPSEND="+String(getStr.length()));

  delay(100);
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
    return "Comm error";
  }

  //Serial.println(resp);

  String relevant = "";
  short ipdIndex = -1;
  short lastStartIndex = -1;
  short idxIndex = -1;
  short rounds = 0;
  do {
    ipdIndex = resp.indexOf("+IPD", lastStartIndex!= -1 ? lastStartIndex : 0);
    if(ipdIndex != -1)
    {
      short startIdx = resp.indexOf(":", ipdIndex);
      if(startIdx == -1)
        return "Bad response";
        
      if(lastStartIndex!= -1)
        relevant = relevant + resp.substring(lastStartIndex, ipdIndex);

      lastStartIndex = startIdx + 1;
    }
    else
    {
      if(lastStartIndex!= -1)
      {
        short endIdx = resp.lastIndexOf("CLOSED");
        relevant = relevant + resp.substring(lastStartIndex, endIdx-1);
        break;
      }
    }
    rounds++;
    if(rounds > 5)
      return "Bad response";
  } while(ipdIndex!= -1);
  
  short contentIdx = relevant.lastIndexOf("\r\n\r\n");
  if(contentIdx == -1)
    return "Bad response";

  resp = relevant.substring(contentIdx+4);
  
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
