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
#define TIME_TO_SLEEP 420      /* Time ESP32 will go to sleep (in seconds) */
#define PMS_ENA_Pin 4
#define DHT_Pin 15

//Init Object
PMS pms(Serial2);
PMS::DATA data;
LapssNode node;
DHT dht;

//Fetch data from sensors and send to gateway.
void fetchSensors()
{
  Serial.println("Sensors read request..");
  pms.requestRead();
  if (!pms.readUntil(data))
  {
    Serial.println("Can't get dust sensor data");
    return;
  }

  float temperature = dht.getTemperature();
  float humidity = dht.getHumidity();
  uint16_t PM25 = data.PM_AE_UG_2_5;
  uint16_t PM1 = data.PM_AE_UG_1_0;
  uint16_t PM10 = data.PM_AE_UG_10_0;

  node.setTemp(temperature);
  node.setHumidity(humidity);
  node.setPM25(PM25);
  node.setPM1(PM1);
  node.setPM10(PM10);

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

  //Sleep the sensors
  pms.sleep();
  digitalWrite(PMS_ENA_Pin,LOW);
  Serial.println("Sleep the sensor");
  delay(5000);

}

void setup()
{
  Serial.begin(9600);  // Serial to PC
  Serial2.begin(9600); // Serial for PMS Dust sensor

  //Init LoRa Chip
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(BAND))
  {
    Serial.println("Starting LoRa failed!");
    while (1)
      ;
  }
  node.setup(LoRa, 0);
  Serial.println("init lora Ok");

  //Setup DHT
  dht.setup(DHT_Pin);

  //Setup PMS Sensor
  pinMode(PMS_ENA_Pin, OUTPUT);
  digitalWrite(PMS_ENA_Pin, 1);

  pms.passiveMode(); // Switch to passive mode
  pms.wakeUp();
  Serial.print("Warming up PMS Dust sensor");
  for (int i = 0; i < 30; i++)
  {
    Serial.printf(". ");
    delay(1000);
  }
  Serial.println();

  fetchSensors();

  //Sleep ESP32
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    Serial.println("Sleep the MCU");

  esp_deep_sleep_start();
}

void loop()
{

}
