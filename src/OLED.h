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

struct OLED_Flag
{
    uint8_t Page = 0;
    uint8_t Cursor = 0;
    int BlockTime;
};

class OLED
{
private:
    uint8_t IO_VCC = 0;
    bool isU8g2Begin = false;
    bool isH = true;
    uint8_t R_now = 6;
    int Timer = 0;
    String BlockInfo;
    int FlashCount = 0;
    // Special Operation
    void Clear();
    void DoBlock();
    // Each Page
    void Main();
    void Menu();
    void Flatness();
    void Bluetooth();
    void Cal_Menu();
    void Cal_Check();
    // Function call by page
    const unsigned char * Connect_Icon(bool isSetting);
    void DrawFloat_9x15(int x, int y, String f);
    void DrawFloat_12x20(int x, int y, String f);
    void DrawArrow();
    void YesNo(bool IsH, bool Select);
    uint8_t BLEShow(bool Animation);
    bool Flash(int Due, int Period);
    bool DoR(bool isMain);
    // QC Page
    void Show_Sensor_Address();
    void Seven_Sensor_Info();
    void One_Sensor_Info();
    

public:
    DistanceSensor *pDS;
    RealTimeClock *pRTC;
    Serial_Communication_Pogo *pPOGO;
    Measure *pMeasure;
    IMU42688 *pIMU;
    uint8_t *pBLEState;
    int *pBatt;
    OLED_Flag flag;
    void TurnOn(byte VCC);
    void InitDisplay();
    void TurnOff();
    void Do_RST();
    void Block(String Info, int time);
    void Update();
};

#endif