#ifndef LapssNode_h
#define LapssNode_h

#include <LoRa.h>
#include <Arduino.h>

/**
 * LAPSS - Large Area Particle Sensing System Project 
 * This class is for LoRa Node 
 * 
 *
 **/


class LapssNode{

    public:

        struct DATA
        {
        uint8_t ID;
        float TEMP;
        uint8_t HUMIDITY;
        uint8_t PM25;
        uint8_t PM1;
        uint8_t PM10;
        uint8_t CRC8;
        }__attribute__((packed));   //Disable memory 4-byte alignment

        DATA data;

        void setup(LoRaClass& lora, uint8_t id);
        void setTemp(float);
        void setHumidity(uint8_t);
        void setPM25(uint8_t);
        void setPM1(uint8_t);
        void setPM10(uint8_t);
        uint8_t getDataCRC8();
        void sendPacket();


    private:
        LoRaClass* _LoRa;
        uint8_t genCRC8();
        void updateCRC8();
};


#endif
