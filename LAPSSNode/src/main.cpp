#include <Arduino.h>
#include <LoRa.h>
#include "LapssNode.h"
#include "DHT.h"
#include "PMS.h"
#include "WiFiManager.h"
#include "ArduinoJson.h"
#include "HTTPClient.h"
#include "ArduinoUnit.h"

//Define variable
#define SCK 5   // GPIO5 - SX1278's SCK
#define MISO 19 // GPIO19 - SX1278's MISO
#define MOSI 27 // GPIO27 - SX1278's MOSI
#define SS 18   // GPIO18 - SX1278's CS
#define RST 14  // GPIO14 - SX1278's RESET
#define DI0 26  // GPIO26 - SX1278's IRQ (interrupt request)
#define BAND 915E6
#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 300      /* Time ESP32 will go to sleep (in seconds) */
#define PMS_UPDATE_INTERVAL 3  /* Fetch dust sensors every TIME_TO_SLEEP * PMS_UPDATE_INTERVAL seconds */
#define PMS_ENA_Pin 4
#define ONBOARD_LED 2
#define DHT_Pin 15
#define NODE_ID 2
#define STANALONE_MODE false

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


void sendDataWiFi(){
  const int JSONSIZE = JSON_OBJECT_SIZE(7);
  StaticJsonDocument<JSONSIZE> jsonDoc;
  jsonDoc["name"] =  "Node "+ String(NODE_ID,DEC);
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
  serializeJson(jsonDoc,postData);
  http.addHeader("Content-Type", "application/json");
  Serial.println(postData);

  int httpCode =   http.POST(postData);


  if (httpCode == 200)
  {
    Serial.println("Send OK");
    delay(500);
  }
  else
  {
    Serial.println("Error "+ httpCode);

  }

  http.end(); //Close connection
}

void sendData()
{
  Serial.printf("Sensors Data:\n");
  Serial.printf("\tTemperature: %.2f\n", node.data.TEMP);
  Serial.printf("\tHumidity: %.2f\n", node.data.HUMIDITY);
  Serial.printf("\tPM 2.5 (ug/m3): %d\n", node.data.PM25);
  Serial.printf("\tPM 1 (ug/m3): %d\n", node.data.PM1);
  Serial.printf("\tPM 10 (ug/m3): %d\n", node.data.PM10);

  if(STANALONE_MODE){
      sendDataWiFi();
  }
  else{

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

  pms.passiveMode(); // Switch to passive mode
  pms.wakeUp();
  Serial.print("Warming up PMS Dust sensor");
  for (int i = 0; i < 60; i++)
  {
    Serial.printf(". ");
    delay(1000);
  }
  Serial.println();
  Serial.println("Sensors read request..");
  pms.requestRead();
  if (!pms.readUntil(data))
  {
    Serial.println("Can't get dust sensor data");
    return;
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

  Serial.println("\n\nBootCount = " + String(bootCount,DEC));

  //Turn on onboard LED to indicate that it's working
  pinMode(ONBOARD_LED,OUTPUT);
  digitalWrite(ONBOARD_LED,HIGH);

  if(STANALONE_MODE){
    wifiManager.autoConnect("KLASS Standalone");
  }
  else{

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

  

  // fetchDHT();
  // if (bootCount % PMS_UPDATE_INTERVAL == 0){
  //   fetchPMS();
  // }else{ 
  //   //if isn't the time for fetching new dust senser value, return last value instead.
  //   node.setPM25(PM25);
  //   node.setPM1(PM1);
  //   node.setPM10(PM10);
  // }

  // bootCount++;
  // sendData();
  // //Sleep ESP32
  // esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  // Serial.println("Sleep the MCU");
  //   digitalWrite(ONBOARD_LED,LOW);

  // esp_deep_sleep_start();
}

void loop()
{

  Test::run();
}



class TestCase {

    public:
  uint8_t ID;
  float TEMP;
  float HUMIDITY;
  uint16_t PM25;
  uint16_t PM1;
  uint16_t PM10;


  TestCase(){
    ID = random(256);
    TEMP = random(-5000,5000)/100.0;
    HUMIDITY = random(0,10000)/100.0;
    PM25 = random(0,500);
    PM1 = random(0,500);
    PM10 = random(0,500);
  }

};


test(ok) 
{

  int totalCase = 1000;

  for(int c = 0; c < totalCase; c++){

  TestCase test1 = TestCase();
  Serial.printf("id:%3d t:%6.2f h:%6.2f PM25:%3d PM1:%3d PM10:%3d\n", test1.ID, test1.TEMP,test1.HUMIDITY,test1.PM25, test1.PM1 , test1.PM10);
  // Serial.printf("id:%3d t:%6.2f h:%6.2f PM25:%3d PM1:%3d PM10:%3d\n", test2.ID, test2.TEMP,test2.HUMIDITY,test2.PM25, test2.PM1 , test2.PM10);

  node.data.ID = 0;
  node.setTemp(test1.TEMP);
  node.setHumidity(test1.HUMIDITY);
  node.setPM1(test1.PM1);
  node.setPM10(test1.PM10);
  node.setPM25(test1.PM25);
  

  uint8_t result[16];
  node.getPacket(*result);

  assertEqual(result[15], node.getDataCRC8());
  // for(int i = 0; i < 16; i++){
  //   Serial.printf("%x ", result[i]);
  // }

  }

 
}

test(bad) 
{

  int totalCase = 1000;

  for(int c = 0; c < totalCase; c++){

  TestCase test1 = TestCase();
  Serial.printf("id:%3d t:%6.2f h:%6.2f PM25:%3d PM1:%3d PM10:%3d\n", test1.ID, test1.TEMP,test1.HUMIDITY,test1.PM25, test1.PM1 , test1.PM10);
  // Serial.printf("id:%3d t:%6.2f h:%6.2f PM25:%3d PM1:%3d PM10:%3d\n", test2.ID, test2.TEMP,test2.HUMIDITY,test2.PM25, test2.PM1 , test2.PM10);

  node.data.ID = 0;
  node.setTemp(test1.TEMP);
  node.setHumidity(test1.HUMIDITY);
  node.setPM1(test1.PM1);
  node.setPM10(test1.PM10);
  node.setPM25(test1.PM25);
  

  uint8_t result[16];
  node.getPacket(*result);

  assertNotEqual(result[15], node.getDataCRC8() + 1);
  // for(int i = 0; i < 16; i++){
  //   Serial.printf("%x ", result[i]);
  // }

  }

 
}
