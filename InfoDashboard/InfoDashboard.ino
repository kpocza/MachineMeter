#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include "WifiParameters.h"

// test
//LiquidCrystal_I2C lcd(0x27,2,1,0,4,5,6,7); // 0x27 is the default I2C bus address of the backpack-see article

// prod
LiquidCrystal_I2C lcd(0x27,2,1,0,4,5,6,7); // 0x27 is the default I2C bus address of the backpack-see article

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
  lcd.setBacklightPin(3,POSITIVE); // BL, BL_POL
  lcd.setBacklight(HIGH);
  lcd.home();
  lcd.print("Connecting");
  
  connectToWifi();
  lcd.begin(20, 4);
  lcd.home();
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
    if(beginCounter%15 == 0) {
      lcd.begin(20, 4);
    }
    show(result);
    beginCounter++;
  }

  counter++;
  oldResult = result;
  
  delay(10000);
}

const String errorComm = "Comm error";
const String errorServerNetwork = "Server/network error";
const String badResponse1 = "Bad response - 1";
const String badResponse2 = "Bad response - 2";

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
    resp.remove(endIdx-1);
  }

  //Serial.println(resp);

  short contentIdx = resp.lastIndexOf("\r\n\r\n");
  if(contentIdx == -1)
    return badResponse2;

  resp.remove(0, contentIdx+4);
  //Serial.println(resp);

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
    lcd.home();
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
