#include <Arduino.h>
#include "LapssGateway.h"
#include "LoRa.h"

//Define variable
#define SCK 5   // GPIO5 - SX1278's SCK
#define MISO 19 // GPIO19 - SX1278's MISO
#define MOSI 27 // GPIO27 - SX1278's MOSI
#define SS 18   // GPIO18 - SX1278's CS
#define RST 14  // GPIO14 - SX1278's RESET
#define DI0 26  // GPIO26 - SX1278's IRQ (interrupt request)
#define BAND 915E6

LapssGateway gateway; //init Lapss core library


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
    Serial.println("Data corrupted");
    return;
  }
  
  Serial.printf("Received from node ID : %d with RSSI : %d\n",gateway.data.ID, LoRa.packetRssi());
  Serial.printf("\ttemp: %.2f\n\thumidity: %.2f\n\tPM1: %d\n\tPM2.5: %d\n\tPM10: %d\n",gateway.data.TEMP,gateway.data.HUMIDITY,gateway.data.PM1,gateway.data.PM25,gateway.data.PM10);

}

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
  
  
  Serial.println("init lora Ok");
  


}

void loop(){
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
   onReceive(packetSize);
  }
}
