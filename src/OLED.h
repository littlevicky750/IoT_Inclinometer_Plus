#ifndef OLED_H
#define OLED_H
#include <U8g2lib.h>
#include "DistanceSensor.h"
#include "Measure.h"
#include "IMU42688.h"
#include "RealTimeClock.h"
#include "Serial_Communication_Pogo.h"
#ifndef CS1
#define CS1 10
#endif
#ifndef CS2
#define CS2 42
#endif
#ifndef IO_OLED_RST
#define IO_OLED_RST 41
#endif

class OLED
{
private:
    uint8_t IO_VCC = 0;
    bool isU8g2Begin = false;
    bool isH = true;
    int BlockTime = 0;
    int Timer = 0;
    String BlockInfo;
    void Clear();
    void Main();
    void DoBlock();
    const unsigned char * Connect_Icon(bool isSetting);
    // For QC
    void ShowAddress();
    void Seven_Sensor_Info();
    void One_Sensor_Info();
    

public:
    uint8_t Page = 0;
    DistanceSensor *pDS;
    RealTimeClock *pRTC;
    Serial_Communication_Pogo *pPOGO;
    Measure *pMeasure;
    IMU42688 *pIMU;
    int *pBatt;
    void TurnOn(byte VCC);
    void InitDisplay();
    void TurnOff();
    void Do_RST();
    void Block(String Info, int time);
    void Update();
};

#endif