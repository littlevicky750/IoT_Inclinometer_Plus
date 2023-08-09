#ifndef Serial_Communication_Package_H
#define Serial_Communication_Package_H
#include <Arduino.h>

struct Message_Package
{
    uint8_t Package_Strat = 0xef;
    uint8_t Connect_Port = 0x00;
    bool isSenseConnect = false;
    uint8_t Address[6];
    float Angle = 200;
    float Distance[7];
    bool haveSensor[7] = {false};
    bool isSettingZero = false;
    uint8_t Package_End = 0xab;
};

#endif