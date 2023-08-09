#include "DistanceSensor.h"

// TwoWire Wire_0 = TwoWire(0);
TwoWire Wire_1 = TwoWire(1);

/**
 * @brief Check the I2C address and set up ADS1115.
 *
 * @param [in] IO_SDA1 Wire 0 SDA GPIO
 * @param [in] IO_SCL1 Wire 0 SCL GPIO
 * @param [in] IO_SDA2 Wire 1 SDA GPIO
 * @param [in] IO_SCL2 Wire 2 SCL GPIO
 * @param [out]
 */
void DistanceSensor::SetUp(const byte &IO_SDA1, const byte &IO_SCL1, const byte &IO_SDA2, const byte &IO_SCL2)
{
    // Begin Two Wire
    // 400000U is the highest frequency of the ESP32 I2C Bus
    Wire.begin(IO_SDA1, IO_SCL1, 400000U);
    Wire_1.begin(IO_SDA2, IO_SCL2, 400000U);
    // Initialize data
    for (int i = 0; i < 8; i++)
    {
        Zeros[i] = 10000;
        Slope[i] = 1;
    }
    // Scan the address and setup ADS1115.
    for (int i = 0; i < 2; i++)
    {
        TwoWire *wire;
        if (i == 0)
            wire = &Wire;
        else
            wire = &Wire_1;
        for (uint16_t Addr = 0x48; Addr < 0x4C - i; Addr++)
        {
            // TwoWire(0) Connect 4 Sensor
            // TwoWire(1) Connect 3 Sensor
            // Test Address
            wire->beginTransmission(Addr);
            byte error = wire->endTransmission();
            // Begin ads1115 hardware
            if (error == 0)
                isConnect[Addr - 0x48 + i * 4] = ads[Addr - 0x48 + i * 4].begin(Addr, wire);
            // Print result
            if (Serial_Print_Connection)
            {
                if (error == 0 && isConnect[Addr - 0x48 + i * 4])
                    Serial.print(F("ADS begin at Wire"));
                else if (error == 0)
                    Serial.print(F("ADS begin failed at Wire"));
                else if (error == 4)
                    Serial.print(F("Unknown error at Wire"));
                else
                    Serial.print(F("Can't find sensor at Wire"));
                String S = String(i) + " 0x" + String(Addr, HEX);
                Serial.println(S);
            }
        }
    }
    for (int i = 0; i < 8; i++)
    {
        if (isConnect[i])
            ConnectCount++;
    }
}

/**
 * @brief Update single sensor value.
 * @param [in] SensorID - Sensor ID
 */
void DistanceSensor::UpdateSensor(uint8_t SensorID)
{
    // Check if sensor connect.
    if (!isConnect[SensorID])
        return;
    // Read Value
    ADCread[0][SensorID] = ads[SensorID].readADC_SingleEnded(0);
    // Cut-off voltage check
    if (ADCread[0][SensorID] < Cut_Off_Voltage)
    {
        ADCread[0][SensorID] = Cut_Off_Voltage;
        ADCfilt[SensorID] = Cut_Off_Voltage;
        Distance[SensorID] = Cut_Off_Distance;
        return;
    }
    // Threshold check.
    bool TH1 = abs(ADCread[0][SensorID] - ADCread[1][SensorID]) < surges_TH;
    bool TH2 = abs(ADCread[0][SensorID] - ADCread[2][SensorID]) < surges_TH;
    ADCSurgeOut[0][SensorID] = (!TH1 && !TH2) ? ADCSurgeOut[1][SensorID] : ADCread[0][SensorID];
    if (!TH1 && !TH2)
        return;
    // Apply Butterworth filter.
    ADCfilt[SensorID] = ADCfilt[SensorID] * BW_C[0] + ADCread[0][SensorID] * BW_C[1] + ADCread[1][SensorID] * BW_C[2];
    // Reset filter if sense great movement (for reducing the stabilization time.)
    if (abs(ADCfilt[SensorID] - ADCread[0][SensorID]) > BW_Reset_TH)
    {
        if (TH1)
            ADCfilt[SensorID] = ADCread[0][SensorID] * 0.5 + ADCread[1][SensorID] * 0.5;
        else
            ADCfilt[SensorID] = ADCread[0][SensorID] * 0.5 + ADCread[2][SensorID] * 0.5;
    };
    // Calculate distance.
    Distance[SensorID] = Slope[SensorID] * Ideal_Slope * (1 / ADCfilt[SensorID] - 1 / Zeros[SensorID]);
}

/**
 * @brief Update Wire(WireNum) sensor reading
 * @param [in] WireNum - 0 to update Wire(0) sensor, 1 to update Wire(1) sensor.
 * @param [out] MaxMin - Max value, min value and difference of the sensors.
 */
void DistanceSensor::Update(uint8_t WireNum)
{
    // Move the raw data buffer
    if (isUpdate[0] && isUpdate[1])
    {
        memset(isUpdate, 0, sizeof(isUpdate));
        memmove(&ADCread[1][0], &ADCread[0][0], sizeof(ADCread[0][0]) * 8 * 19);
        memmove(&ADCSurgeOut[1][0], &ADCSurgeOut[0][0], sizeof(ADCSurgeOut[0][0]) * 8 * 19);
    }
    // Update
    if (!isUpdate[WireNum])
    {
        for (int i = WireNum * 4; i < WireNum * 4 + 4; i++)
        {
            UpdateSensor(i);
        }
        isUpdate[WireNum] = true;
    }
    // Calculate
    if (isUpdate[0] && isUpdate[1])
        Generate_Result();
}

/**
 * @brief Generate MaxMin result after all data update.
 * @note Do the calibration as well when all data is updated.
 */
void DistanceSensor::Generate_Result()
{
    // Check whether all data update.
    if (!isUpdate[0] || !isUpdate[1])
    {
        Serial.println(F("Error : DistanceSensor::GenerateResult was called before sensor update properly."));
        return;
    }

    // Print raw data.
    if (Serial_Print_Raw_Data)
    {
        String S = "";
        for (int i = 0; i < 8; i++)
        {
            if (isConnect[i])
                S += String(ADCread[0][i]) + ",";
            else
                S += "0,";
        }
        Serial.println(S);
    }
    else if (Serial_Print_Distance)
    {
        String S = "";
        for (int i = 0; i < 8; i++)
        {
            if (isConnect[i])
                S += String(Distance[i]) + ",";
            else
                S += "0,";
        }
        Serial.println(S);
    }

    // If recieve set zero command.
    if (SetZeros)
    {
        Cal_Zero();
    }
    else
    {
        // Calculate maximum displacement.
        Mm.Clear();
        Mm.Add(&Distance[0], 8);
        if (pReceive->isSenseConnect) // If connect to sub ruler, consider the sub ruler value.
            Mm.Add(&pReceive->Distance[0], 7);
        if (Mm.MaxVal >= Cut_Off_Distance)
            Mm.Diff = Cut_Off_Distance;
    }
}

/**
 * @brief Update all sensor.
 */
void DistanceSensor::Update()
{
    Update(0);
    Update(1);
}

/**
 * @brief Collect \b Cal_Zero_Data_Length of ADC data and set the average value as zeros.
 */
void DistanceSensor::Cal_Zero()
{
    // Step 1. Set output to default while calibrating ============================================================
    Mm.Diff = 0;
    // Step 2. Collect data.=======================================================================================
    CollectData();
    // Step 3. Calculate data collection progress. ===============================================================
    if (CollectDataCount < Cal_Zero_Data_Length)
        SetZeroProgress = (CollectDataCount <= 1) ? 0 : (CollectDataCount - 1) / Cal_Zero_Data_Length;
    // Step 4. If data collection is complete ====================================================================
    else
    {
        for (int i = 0; i < 8; i++)
        {
            if (isConnect[i])
            {
                // Step 4.1. Calculate sensor's average reading and set it as zeros ================================
                Zeros[i] = CollectDataSum[i] / Cal_Zero_Data_Length;
                // Step 4.2. Set current value to zeros ============================================================
                ADCfilt[i] = Zeros[i];
            }
        }
        // Step 4.3. Print Result ===================================================================================
        if (Serial_Print_Calibration)
        {
            String S = "[Dist] Set Zero Complete : ";
            for (int i = 0; i < 8; i++)
            {
                if (isConnect[i])
                    S += String(Zeros[i], 3) + ",";
                else
                    S += "0,";
            }
            Serial.println(S);
        }
        // Step 4.4. Calibration done. Reset all buffer. ============================================================
        SetZeroProgress = 1;
        SetZeros = false;
        CollectDataCount = 0;
    }
}

/**
 * @brief Check if data within threshold and save data into buffer.
 */
void DistanceSensor::CollectData()
{
    // Initialize Collection
    if (CollectDataCount == 0)
    {
        memcpy(&CollectDataStart[0], &ADCread[0][0], sizeof(CollectDataStart[0]) * 8);
        memset(&CollectDataSum[0], 0, sizeof(CollectDataSum[0]) * 8);
    }
    for (int i = 0; i < 8; i++)
    {
        if (isConnect[i])
        {
            // Check the stability
            if (abs(ADCread[0][i] - CollectDataStart[i]) > Cal_TH || ADCread[0][i] == Cut_Off_Voltage)
            {
                CollectDataCount = 0;
                return;
            }
            else
            {
                CollectDataSum[i] += ADCread[0][i];
            }
        }
    }
    CollectDataCount++;
}

/**
 * @brief Command to calibrate the sensor's zero position.
 *
 * @param [in] OnOff True to start calibrating the zero position. False to stop the calibration.
 * @param [in] isForced True if do the reset zero position anyway.
 *                      False if the command is from the system auto calibration detected and need to consider the \b Enable_Auto_Reset setting.
 */
void DistanceSensor::Reset(bool OnOff, bool isForced)
{
    if (!OnOff)
    {
        SetZeros = false;
        CollectDataCount = 0;
        SetZeroProgress = 1;
    }
    else if (isForced || Enable_Auto_Reset)
    {
        SetZeros = true;
        CollectDataCount = 0;
        SetZeroProgress = 0;
    }
}