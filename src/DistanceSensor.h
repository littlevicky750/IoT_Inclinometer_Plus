/**
 * @file DistanceSensor.h
 * @author Vicky Hung
 * @brief Scan, read, filt and calibrate the distance information from IR sensor and ads1115.
 * @version 0.1
 * @date 2023-07-31
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef DistanceSensor_H
#define DistanceSensor_H
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_ADS1X15.h>
#include "Serial_Communication_Package.h"
#include <MaxMin.h>

class DistanceSensor
{
private:
    /******************************************************************************************************
     * Parameter setting.
     *******************************************************************************************************/
    // For Debug
    const bool Serial_Print_Connection = true;  /** @brief True if require printing the I2C scanning result in Setup.*/
    const bool Serial_Print_Raw_Data = false;   /** @brief True if require printing the Raw Data while Update.*/
    const bool Serial_Print_Distance = false;   /** @brief True if require printing the Distance while Update.*/
    const bool Serial_Print_Calibration = true; /** @brief True if require printing the calibration result.*/

    // Filter Setting

    const int surges_TH = 50;              /** @brief Surges filter threshold @deprecated &Delta;Distance &asymp; surges_TH * 0.005 mm (@ ADCread &asymp; 8000).*/
    const int BW_Reset_TH = 500;           /** @brief Reset the Butterworth filter if the raw data exceed the BW_Reset_TH.*/
    const float BW_C[3] = {0.8, 0.1, 0.1}; /** @brief Buttorworth filter coefficient @note c0 + c1 + c2 = 1.0 @note c1 = c2*/
    const int Cut_Off_Voltage = 2000;      /** @brief Cut-off voltage*/
    const int Cut_Off_Distance = 100;      /** @brief Default distance when ads reading under the cut-off voltage.*/

    // Calibration Setting
    const uint8_t Cal_Zero_Data_Length = 20; /** @brief Number of data require for calibrating zeros.*/
    const int Cal_TH = 100;                  /** @brief Stabilization identification threshold when doing calibration.*/

    /*****************************************************************************************************
     * Parmeter load from flash memory.
     ******************************************************************************************************/
    const float Ideal_Slope = 330000; /** @brief Ideal relationship between distance and 1 / adc reading.*/
    float Zeros[8] = {0};             /** @brief Voltage reading when distance = 0.*/
    float Slope[8] = {1};             /** @brief Scale relationship between distance and 1/ADC_Read.*/

    /*****************************************************************************************************
     * Sensor Value.
     ******************************************************************************************************/
    Adafruit_ADS1115 ads[8];       /** @brief Sensor driver for the Adafruit ADS1115 ADC breakout.*/
    uint16_t ADCread[20][8] = {0}; /** @brief ADS1115 ADC raw value buffer.*/
    bool isUpdate[2] = {0};        /** @brief isUpdate[i] = True if sensor connect to Wire(i) is update.*/

    /*****************************************************************************************************
     * Calibration buffer.
     ******************************************************************************************************/
    uint16_t CollectDataStart[8] = {0};
    uint16_t CollectDataCount = 0;
    unsigned int CollectDataSum[8] = {0};
    void CollectData();
    void Cal_Zero();
    void UpdateSensor(uint8_t SensorID);
    void Generate_Result();

public:
    /*****************************************************************************************************
     * Pointer.
     ******************************************************************************************************/
    Message_Package *pReceive;

    /*****************************************************************************************************
     * Basic.
     ******************************************************************************************************/
    MaxMin Mm;
    uint16_t ADCSurgeOut[20][8] = {0}; /** @brief ADS1115 ADC reading with surge filter.*/
    float ADCfilt[8] = {0};            /** @brief Filted ADS1115 ADC reading.*/
    float Distance[8] = {0};           /** @brief Distance value calculate from ADCfilt.*/
    bool isConnect[8] = {false};       /** @brief isConnect [ i ] = true if Sensor [ i ] is connected.*/
    uint8_t ConnectCount = 0;          /** @brief Numbers of connected ADS1115.*/
    void SetUp(const byte &IO_SDA1, const byte &IO_SCL1, const byte &IO_SDA2, const byte &IO_SCL2);
    void Update();
    void Update(uint8_t WireNum);

    /*****************************************************************************************************
     * Calibration
     ******************************************************************************************************/
    bool Enable_Auto_Reset = true;             /** @brief True if enable auto zero position calibration.*/
    bool SetZeros = Enable_Auto_Reset;         /** @brief True when doing zero position calibration.*/
    float SetZeroProgress = Enable_Auto_Reset; /** @brief Zero position calibration's data collection progerss.*/

    void Reset(bool OnOff, bool isForced);
};

#endif