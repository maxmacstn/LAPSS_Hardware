#include <Arduino.h>
#include "LapssGateway.h"
#include "LoRa.h"
#include "SSD1306.h"
#include "WiFiManager.h"
#include "ArduinoJson.h"
#include "HTTPClient.h"

//Define variable
#define SCK 5   // GPIO5 - SX1278's SCK
#define MISO 19 // GPIO19 - SX1278's MISO
#define MOSI 27 // GPIO27 - SX1278's MOSI
#define SS 18   // GPIO18 - SX1278's CS
#define RST 14  // GPIO14 - SX1278's RESET
#define DI0 26  // GPIO26 - SX1278's IRQ (interrupt request)
#define BAND 915E6
#define HR_TO_RESET 12

LapssGateway gateway; //init Lapss core library
SSD1306 display (0x3c, 4, 15);
WiFiManager wifiManager;
const String lapssEndpoint = "https://lapsscentral.azurewebsites.net/api/sensors";
const String emonAPIKey = "7392d102d9a61972b6893f5f8afae81f"; //api key


String rssi = "RSSI -";
String packSize = "-";
String packet = " ";
float temperature = 0.0;
float humidity = 0.0;
uint16_t PM1 = 0;
uint16_t PM25 = 0;
uint16_t PM10 = 0;
uint8_t node_id;


unsigned long lastUpdate = millis();

void showDebugMessage(String text){
  display.clear ();
  display.setTextAlignment (TEXT_ALIGN_LEFT);
  display.setFont (ArialMT_Plain_10);
  display.drawString (0, 0, text);
  display.display();
  Serial.println(text);

}

void sendData(){
  const int JSONSIZE = JSON_OBJECT_SIZE(7);
  StaticJsonDocument<JSONSIZE> jsonDoc;
  jsonDoc["name"] =  "Node "+ String(node_id,DEC);
  jsonDoc["pm25Level"] = PM25;
  jsonDoc["pm10Level"] = PM10;
  jsonDoc["pm1Level"] = PM1;
  jsonDoc["humidity"] = humidity;
  jsonDoc["temp"] = temperature;
  serializeJsonPretty(jsonDoc, Serial);

  HTTPClient http; //Declare object of class HTTPClient
  String postData;
  http.begin(lapssEndpoint); //Specify request destination
  // http.setTimeout(5);
  serializeJson(jsonDoc,postData);
  http.addHeader("Content-Type", "application/json");
  Serial.println(postData);

  int httpCode =   http.POST(postData);


  if (httpCode == 200)
  {
    showDebugMessage("Send OK");
    delay(500);
  }
  else
  {
    showDebugMessage("Send Failed Error code :"+ String(httpCode,DEC));
    delay(500);
  }

  http.end(); //Close connection


  //EMON CMS


  postData = "node=LAPSSNode&json={temperature:" + String(temperature, 1) + ",humidity:" + String(humidity, 1) + ",PM25:" + String(PM25,DEC) + "}&apikey=" + emonAPIKey;
  http.begin("http://emoncms.org/input/post?" + postData); //Specify request destination

  httpCode = http.GET();         //Send the request
  String payload = http.getString(); //Get the response payload

  // Serial.println(httpCode); //Print HTTP return code
  // Serial.println(postData);  //Print request response payload

  if (httpCode == 200)
  {
    showDebugMessage("EMON Send OK");
    delay(500);
  }
  else
  {
    showDebugMessage("EMON Send Failed Error code :"+ String(httpCode,DEC));
    delay(500);
  }

  http.end(); //Close connection

}

//When WiFi Manager enters config mode (Failed to connect to wifi)
void setConfigModeCallback(WiFiManager *myWiFiManager){
  showDebugMessage("Enter Wifi setup mode : " + myWiFiManager->getConfigPortalSSID());
}

void displayData () {
  display.clear ();
  display.setTextAlignment (TEXT_ALIGN_LEFT);
  display.setFont (ArialMT_Plain_10);
  display.drawString (0, 0, rssi);

  display.setFont(ArialMT_Plain_16);
  display.drawString (0, 23, String(temperature,1)+"°c  " + String(humidity,1) + "%\n" + "PM25: "+ PM25 +" ug/cm3");

  int lastUpdateSec = (millis() - lastUpdate) /1000;

  display.setTextAlignment (TEXT_ALIGN_RIGHT);
  display.setFont (ArialMT_Plain_10);
  display.drawString (127, 0, String(lastUpdateSec) +"s ago");


  display.display ();
}


void onReceive(int packetSize){
   // received a packet
  Serial.print("Received packet \n");

  if (packetSize != sizeof(gateway.data)){       //Wrong size of data
    Serial.println("Size mismatch");
    return;
  }

  uint8_t data[packetSize];
  
  // read packet
  for (int i = 0; i < packetSize; i++) {
    data[i] = (uint8_t)LoRa.read();
  }
  if(!gateway.processPacket(data)){   //invalid CRC
      showDebugMessage("Data corrupted");
    return;
  }
  
  rssi = "RSSI "+ String(LoRa.packetRssi(),DEC);

  Serial.printf("Received from node ID : %d with RSSI : %d\n",gateway.data.ID, LoRa.packetRssi());
  Serial.printf("\ttemp: %.2f\n\thumidity: %.2f\n\tPM1: %d\n\tPM2.5: %d\n\tPM10: %d\n",gateway.data.TEMP,gateway.data.HUMIDITY,gateway.data.PM1,gateway.data.PM25,gateway.data.PM10);
  
  node_id = gateway.data.ID;
  temperature = gateway.data.TEMP;
  humidity = gateway.data.HUMIDITY;
  PM25 = gateway.data.PM25;
  PM10 = gateway.data.PM10;
  PM1  = gateway.data.PM1;
  lastUpdate = millis();
  displayData();
  sendData();
}

void setup()
{
  Serial.begin(9600);   // Serial to PC
  Serial2.begin(9600);  // Serial for PMS Dust sensor

  //Init oled
  pinMode (16, OUTPUT);
  digitalWrite (16, LOW); // set GPIO16 low to reset OLED
  delay (50);
  digitalWrite (16, HIGH); // while OLED is running, GPIO16 must go high,
  display.init ();
  display.flipScreenVertically ();
  display.setFont (ArialMT_Plain_10);
  
  //Init LoRa Chip
  showDebugMessage("Init LoRa");
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(BAND))
  {
    Serial.println("Starting LoRa failed!");
    while (1)
      ;
  }
  Serial.println("init lora Ok");

  //Init Wifi
  showDebugMessage("Connecting to Wifi");
  wifiManager.setAPCallback(setConfigModeCallback);
  if (wifiManager.autoConnect("TTGO-ESP32 LoRa"));
    showDebugMessage("Wifi Connected");

  delay (1500);
  


}

void loop(){
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
   onReceive(packetSize);
  }
  delay(100);
  displayData();
  if (millis() > 1000*60*60*HR_TO_RESET){
    ESP.restart();
  }
}
