#include <Arduino.h>
#include <LoRa.h>
#include "LapssNode.h"
#include "DHT.h"
#include "PMS.h"

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
#define PMS_UPDATE_INTERVAL 6  /* Fetch dust sensors every TIME_TO_SLEEP * PMS_UPDATE_INTERVAL seconds */
#define PMS_ENA_Pin 4
#define ONBOARD_LED 2
#define DHT_Pin 15

//Init Object
PMS pms(Serial2);
PMS::DATA data;
LapssNode node;
DHT dht;

RTC_DATA_ATTR uint16_t PM25 = 0;
RTC_DATA_ATTR uint32_t bootCount = 0;
RTC_DATA_ATTR uint16_t PM1 = 0;
RTC_DATA_ATTR uint16_t PM10 = 0;


void sendData()
{
  Serial.printf("Sensors Data:\n");
  Serial.printf("\tTemperature: %.2f\n", node.data.TEMP);
  Serial.printf("\tHumidity: %.2f\n", node.data.HUMIDITY);
  Serial.printf("\tPM 2.5 (ug/m3): %d\n", node.data.PM25);
  Serial.printf("\tPM 1 (ug/m3): %d\n", node.data.PM1);
  Serial.printf("\tPM 10 (ug/m3): %d\n", node.data.PM10);

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

  //Init LoRa Chip
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(BAND))
  {
    Serial.println("Starting LoRa failed!");
    while (1)
      ;
  }
  node.setup(LoRa, 1);
  Serial.println("init lora Ok");

  fetchDHT();
  if (bootCount % PMS_UPDATE_INTERVAL == 0){
    fetchPMS();
  }else{ 
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
    digitalWrite(ONBOARD_LED,LOW);

  esp_deep_sleep_start();
}

void loop()
{
}
