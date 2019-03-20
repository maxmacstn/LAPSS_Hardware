#ifndef LapssGateway_h
#define LapssGateway_h

#include <LoRa.h>
#include <Arduino.h>

/**
 * LAPSS - Large Area Particle Sensing System Project 
 * This class is for LoRa Gateway
 * 
 *
 **/


class LapssGateway{

    public:

        struct DATA
        {
        uint8_t ID;
        float TEMP;
        float HUMIDITY;
        uint16_t PM25;
        uint16_t PM1;
        uint16_t PM10;
        uint8_t CRC8;
        }__attribute__((packed));   //Disable memory 4-byte alignment

        DATA data;

        void setup(LoRaClass& lora, uint8_t id);
        void setTemp(float);
        void setHumidity(float);
        void setPM25(uint16_t);
        void setPM1(uint16_t);
        void setPM10(uint16_t);
        uint8_t getDataCRC8();
        int processPacket(uint8_t *message);


    private:
        LoRaClass* _LoRa;
        uint8_t genCRC8(uint8_t addr[]);
        void updateCRC8();
};


#endif
