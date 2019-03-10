#include <LapssNode.h>

void LapssNode::setup(LoRaClass &lora, uint8_t id)
{
    this->_LoRa = &lora;
    data.ID = id;
}

void LapssNode::setTemp(float temp)
{
    data.TEMP = temp;
    updateCRC8();
}

void LapssNode::setHumidity(uint8_t humidity)
{
    data.HUMIDITY = humidity;
    updateCRC8();
}

void LapssNode::setPM25(uint8_t pm25)
{
    data.PM25 = pm25;
    updateCRC8();
}

void LapssNode::setPM1(uint8_t pm1)
{
    data.PM1 = pm1;
    updateCRC8();
}

void LapssNode::setPM10(uint8_t pm10)
{
    data.PM10 = pm10;
    updateCRC8();
}

uint8_t LapssNode::getDataCRC8() { return genCRC8(); }

uint8_t LapssNode::genCRC8()
{
    uint8_t *addr = &data.ID;
    uint8_t len = sizeof(data) - sizeof(data.CRC8); //Exclude CRC8 from data
    uint8_t crc = 0;
    while (len--)
    {
        uint8_t inbyte = *addr++;
        for (uint8_t i = 8; i; i--)
        {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix)
                crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    data.CRC8 = crc;

    return crc;
}

void LapssNode::updateCRC8()
{
    data.CRC8 = genCRC8();
}

void LapssNode::sendPacket()
{
    LoRa.beginPacket(); // start packet

    uint8_t *addr = &data.ID;
    uint8_t len = sizeof(data);
    while (len--)
    {
        uint8_t dataByte = *addr++;
        LoRa.write(dataByte); // Send each byte from data
    }
    LoRa.endPacket(); // finish packet and send it
}