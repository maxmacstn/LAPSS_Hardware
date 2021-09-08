#include <Arduino.h>
#include <LoRa.h>
#include "LapssNode.h"
#include "DHT.h"
#include "PMS.h"
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
#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 720      /* Time ESP32 will go to sleep (in seconds) */
#define PMS_UPDATE_INTERVAL 1  /* Fetch dust sensors every TIME_TO_SLEEP * PMS_UPDATE_INTERVAL seconds */
#define PMS_ENA_Pin 4
#define ONBOARD_LED 2
#define DHT_Pin 15
#define NODE_ID 2
#define STANALONE_MODE true

//Init Object
PMS pms(Serial2);
PMS::DATA data;
LapssNode node;
DHT dht;

RTC_DATA_ATTR uint16_t PM25 = 0;
RTC_DATA_ATTR uint32_t bootCount = 0;
RTC_DATA_ATTR uint16_t PM1 = 0;
RTC_DATA_ATTR uint16_t PM10 = 0;

//For standalone mode
WiFiManager wifiManager;
const String lapssEndpoint = "https://lapsscentral.azurewebsites.net/api/sensors";
const String emonAPIKey = "7392d102d9a61972b6893f5f8afae81f"; //api key

void sendDataWiFi()
{
  const int JSONSIZE = JSON_OBJECT_SIZE(7);
  StaticJsonDocument<JSONSIZE> jsonDoc;
  jsonDoc["name"] = "Node " + String(NODE_ID, DEC);
  jsonDoc["pm25Level"] = node.data.PM25;
  jsonDoc["pm10Level"] = node.data.PM10;
  jsonDoc["pm1Level"] = node.data.PM1;
  jsonDoc["humidity"] = node.data.HUMIDITY;
  jsonDoc["temp"] = node.data.TEMP;
  serializeJsonPretty(jsonDoc, Serial);

  HTTPClient http; //Declare object of class HTTPClient
  String postData;
  http.begin(lapssEndpoint); //Specify request destination
  // http.setTimeout(5);
  serializeJson(jsonDoc, postData);
  http.addHeader("Content-Type", "application/json");
  Serial.println(postData);

  int httpCode = http.POST(postData);

  if (httpCode == 200)
  {
    Serial.println("Send OK");
    delay(500);
  }
  else
  {
    Serial.println("Error " + httpCode);
  }

  http.end(); //Close connection

  //EMON CMS

  // postData = "node=LAPSSNode&json={temperature:" + String(node.data.TEMP, 1) + ",humidity:" + String(node.data.HUMIDITY, 1) + ",PM25:" + String(node.data.PM25, DEC) + "}&apikey=" + emonAPIKey;
  // http.begin("http://emoncms.org/input/post?" + postData); //Specify request destination

  // httpCode = http.GET();             //Send the request
  // String payload = http.getString(); //Get the response payload

  // // Serial.println(httpCode); //Print HTTP return code
  // // Serial.println(postData);  //Print request response payload

  // if (httpCode == 200)
  // {
  //   Serial.println("EMON Send OK");
  //   delay(500);
  // }
  // else
  // {
  //   Serial.println("EMON Send Failed Error code :" + String(httpCode, DEC));
  //   delay(500);
  // }

  // http.end(); //Close connection
}

void sendData()
{
  Serial.printf("Sensors Data:\n");
  Serial.printf("\tTemperature: %.2f\n", node.data.TEMP);
  Serial.printf("\tHumidity: %.2f\n", node.data.HUMIDITY);
  Serial.printf("\tPM 2.5 (ug/m3): %d\n", node.data.PM25);
  Serial.printf("\tPM 1 (ug/m3): %d\n", node.data.PM1);
  Serial.printf("\tPM 10 (ug/m3): %d\n", node.data.PM10);

  if (node.data.HUMIDITY < 10 && node.data.TEMP < 10){
    Serial.println("Temp/Humidity readings error!");
    return;
  }

  if (STANALONE_MODE)
  {
    sendDataWiFi();
  }
  else
  {

    //Print the data as Hex
    Serial.printf("Data bytes: ");
    uint8_t *addr = &(node.data).ID;
    uint8_t len = sizeof(node.data);
    while (len--)
    {
      uint8_t inbyte = *addr++;
      Serial.printf("%02X ", inbyte);
    }
    Serial.println("");
    node.sendPacket();
  }
  delay(1000);
}

//Fetch data from sensors and send to gateway.
void fetchDHT()
{
  //Setup DHT
  dht.setup(DHT_Pin);
  dht.getMinimumSamplingPeriod();
  delay(10000);
  float temperature = dht.getTemperature();
  float humidity = dht.getHumidity();

  node.setTemp(temperature);
  node.setHumidity(humidity);
}

void fetchPMS()
{
  //Setup PMS Sensor
  pinMode(PMS_ENA_Pin, OUTPUT);
  digitalWrite(PMS_ENA_Pin, 1);
  delay(5000);

  // **Passive Mode**
  // pms.passiveMode(); // Switch to passive mode
  // pms.wakeUp();
  // Serial.print("Warming up PMS Dust sensor");
  // for (int i = 0; i < 60; i++)
  // {
  //   Serial.printf(". ");
  //   delay(1000);
  // }
  // Serial.println();
  // Serial.println("Sensors read request..");
  // pms.requestRead();
  // if (!pms.readUntil(data))
  // {
  //   Serial.println("Can't get dust sensor data");
  //   return;
  // }

  for(int i = 0 ; i < 120; i++){
    if (pms.read(data))
    {
      Serial.print("PM 1.0 (ug/m3): ");
      Serial.println(data.PM_AE_UG_1_0);
      Serial.print("PM 2.5 (ug/m3): ");
      Serial.println(data.PM_AE_UG_2_5);
      Serial.print("PM 10.0 (ug/m3): ");
      Serial.println(data.PM_AE_UG_10_0);
      Serial.println();
    }
    delay(1000);
  }


  PM25 = data.PM_AE_UG_2_5;
  PM1 = data.PM_AE_UG_1_0;
  PM10 = data.PM_AE_UG_10_0;

  node.setPM25(PM25);
  node.setPM1(PM1);
  node.setPM10(PM10);

  //Sleep the sensors
  pms.sleep();
  digitalWrite(PMS_ENA_Pin, LOW);
  Serial.println("Sleep the PMS sensor");
}

void setup()
{
  Serial.begin(9600);  // Serial to PC
  Serial2.begin(9600); // Serial for PMS Dust sensor

  Serial.println("\n\nBootCount = " + String(bootCount, DEC));

  //Turn on onboard LED to indicate that it's working
  pinMode(ONBOARD_LED, OUTPUT);
  digitalWrite(ONBOARD_LED, HIGH);

  if (STANALONE_MODE)
  {
    wifiManager.setTimeout(60);
    if(!wifiManager.autoConnect("KLASS Standalone")){
      bootCount++;
      //Sleep ESP32
      esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
      Serial.println("Sleep the MCU - WiFi Connection Failed");
      digitalWrite(ONBOARD_LED, LOW);
      esp_deep_sleep_start();
    }
  }
  else
  {
    //Init LoRa Chip
    SPI.begin(SCK, MISO, MOSI, SS);
    LoRa.setPins(SS, RST, DI0);
    if (!LoRa.begin(BAND))
    {
      Serial.println("Starting LoRa failed!");
      while (1)
        ;
    }
    Serial.println("init lora Ok");
  }

  node.setup(LoRa, NODE_ID);

  fetchDHT();
  if (bootCount % PMS_UPDATE_INTERVAL == 0)
  {
    fetchPMS();
  }
  else
  {
    //if isn't the time for fetching new dust senser value, return last value instead.
    node.setPM25(PM25);
    node.setPM1(PM1);
    node.setPM10(PM10);
  }

  bootCount++;
  sendData();
  //Sleep ESP32
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Sleep the MCU");
  digitalWrite(ONBOARD_LED, LOW);

  esp_deep_sleep_start();
}

void loop()
{
}
