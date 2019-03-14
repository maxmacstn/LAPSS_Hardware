#include <Arduino.h>

#include "PMS.h"
#include <LoRa.h>
#include "LapssNode.h"



//Define variable
#define SCK 5   // GPIO5 - SX1278's SCK
#define MISO 19 // GPIO19 - SX1278's MISO
#define MOSI 27 // GPIO27 - SX1278's MOSI
#define SS 18   // GPIO18 - SX1278's CS
#define RST 14  // GPIO14 - SX1278's RESET
#define DI0 26  // GPIO26 - SX1278's IRQ (interrupt request)
#define BAND 915E6
#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 120       /* Time ESP32 will go to sleep (in seconds) */

//Init Object
PMS pms(Serial2);
PMS::DATA data;
LapssNode node;


void setup()
{
  Serial.begin(9600);   // Serial to PC
  Serial2.begin(9600);  // Serial for PMS Dust sensor
  
  //Init LoRa Chip
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(BAND))
  {
    Serial.println("Starting LoRa failed!");
    while (1)
      ;
  }
  node.setup(LoRa,0);
  
  Serial.println("init lora Ok");
  
  // pms.passiveMode();    // Switch to passive mode
  //   pms.wakeUp();

  node.setTemp(23.5);
  node.setHumidity(70);
  node.setPM25(30);
  node.setPM1(60);
  node.setPM10(30);
  // Serial.printf("%x\n",node.getDataCRC8());

  // Serial.printf("Struct size = %d\n", sizeof(node.data));


  //Print the data as Hex
  uint8_t *addr = &(node.data).ID;
    uint8_t len = sizeof(node.data);
    while (len--) {
		uint8_t inbyte = *addr++;
			Serial.printf("%02X ",inbyte);
			}
    
  node.sendPacket();

}

void loop()
{

  // Serial.println("Send read request...");
  // pms.requestRead();

  // Serial.println("Wait max. 1 second for read...");
  // if (pms.readUntil(data))
  // {
  //   Serial.print("PM 1.0 (ug/m3): ");
  //   Serial.println(data.PM_AE_UG_1_0);

  //   Serial.print("PM 2.5 (ug/m3): ");
  //   Serial.println(data.PM_AE_UG_2_5);

  //   Serial.print("PM 10.0 (ug/m3): ");
  //   Serial.println(data.PM_AE_UG_10_0);
  // }
  // else
  // {
  //   Serial.println("No data.");
  // }

  // delay(3000);
}
