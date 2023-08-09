#ifndef BLE_H
#define BLE_H
#include "NimBLEDevice.h"
#include <Arduino.h>
#include "IMU42688.h"
#include "RealTimeClock.h"
#include "LEDFlash.h"

struct BLEStatus
{
    uint8_t Address[6];
    uint8_t isConnect = false;
    uint8_t OnOff = true;
};

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer);
    void onDisconnect(BLEServer *pServer);
public:
    BLEStatus *Status;
    LEDFlash *pLED;
    int *LastEdit;
};

class SetTimesCallBacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic);

public:
    RealTimeClock *pRTC;
};

class BLE
{
public:
    BLEStatus Status;
    String AddrStr = "";
    RealTimeClock *pRTC;
    LEDFlash *pLED;
    void Initialize(int &LastEdit);
    void Send(float *SendFloat);
    void DoSwich();

private:
    BLEServer *pServer;
    BLECharacteristic *AngleXChar;
    BLECharacteristic *AngleYChar;
    BLECharacteristic *AngleZChar;
    BLECharacteristic *DisChar[15];
    BLECharacteristic *SetClkChar;

    MyServerCallbacks ServerCB;
    SetTimesCallBacks SetTimeCB;
    uint8_t Pre_OnOff = true;
};
#endif