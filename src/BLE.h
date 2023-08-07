#ifndef BLE_H
#define BLE_H
#include "NimBLEDevice.h"
#include <Arduino.h>
#include "IMU42688.h"
#include "RealTimeClock.h"
#include "SLED.h"

struct BLEState
{
    uint8_t Address[6];        /** @brief BLE advertising address.*/
    uint8_t isConnect = false; /** @brief Indicate BLE connection status.*/
    uint8_t OnOff = true;      /** @brief BLE connsction or advertising status control.*/
    uint8_t Send_Info = false; /** @brief Waiting to send info.*/
    uint8_t ExpertMode = true; /** @brief Default false. True if BLE recieve correct expert mode key.*/
};

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer);
    void onDisconnect(BLEServer *pServer);

public:
    BLEState *State;
    SLED *pLED;
    int *LastEdit;
};

class SetTimesCallBacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic);

public:
    RealTimeClock *pRTC;
};

class SetUnitCallBacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic);

public:
    IMU42688 *pIMU;
};

class InputKeyCallBacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic);

public:
    BLEState *State;
};

class BLE
{
public:
    BLEState State;
    String AddrStr = "";
    RealTimeClock *pRTC;
    IMU42688 *pIMU;
    SLED *pLED;
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
    BLECharacteristic *SetUniChar;
    BLECharacteristic *SetKeyChar;

    MyServerCallbacks ServerCB;
    SetTimesCallBacks SetTimeCB;
    SetUnitCallBacks SetUnitCB;
    InputKeyCallBacks SetKeyCB;
    uint8_t Pre_OnOff = true;
};
#endif