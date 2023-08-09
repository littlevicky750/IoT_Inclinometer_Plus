/**
 * @file IMU42688.cpp
 * @author Vicky Hung
 * @brief Read and calibrate the WC_IMU-X-Y-Z
 * @version final
 * @date 2023-07-21
 *
 * @copyright Wonder Construct
 *
 */

#include "IMU42688.h"
#include <Preferences.h>

WC_IMU IMU;
Preferences pref;

/**
 * @brief Initialize Serial1 and read calibrated data from memory
 * @param [in] Rx : The GPIO for Serial1 RX. default -1.
 * @param [in] Tx : The GPIO for Serial1 TX. default -1.
 */
void IMU42688::Initialize(byte Rx, byte Tx)
{
    // Serial1 begin
    Serial1.setRxBufferSize(256);
    Serial1.begin(921600, SERIAL_8N1, Rx, Tx);

    // Get calibrate data from flash memory
    pref.begin("Angle_Cal", false);
    e[0] = pref.getFloat("Ex", 0.0);
    e[1] = pref.getFloat("Ey", 0.0);
    e[2] = pref.getFloat("Ez", 0.0);
    s[0] = pref.getFloat("Sx", 1.0);
    s[1] = pref.getFloat("Sy", 1.0);
    s[2] = pref.getFloat("Sz", 1.0);
    b[0] = pref.getFloat("Bx", 0.0);
    b[1] = pref.getFloat("By", 0.0);
    b[2] = pref.getFloat("Bz", 0.0);
    unit = pref.getInt("Unit", 0);
    pref.end();

    // Preint data load from flash mamory.
    Cal_Info_From_Flash = "s[0] = " + String(s[0], 6) + ", ";
    Cal_Info_From_Flash += "s[1] = " + String(s[1], 6) + ", ";
    Cal_Info_From_Flash += "s[2] = " + String(s[2], 6) + "\n";
    Cal_Info_From_Flash += "b[0] = " + String(b[0], 6) + ", ";
    Cal_Info_From_Flash += "b[1] = " + String(b[1], 6) + ", ";
    Cal_Info_From_Flash += "b[2] = " + String(b[2], 6) + "\n";
    Cal_Info_From_Flash += "e[0] = " + String(e[0], 6) + ", ";
    Cal_Info_From_Flash += "e[1] = " + String(e[1], 6) + ", ";
    Cal_Info_From_Flash += "e[2] = " + String(e[2], 6) + "\n";
    // Serial.println(Cal_Info_From_Flash);
    // Serial.println(unit);
} // end Initialize()

/**
 * @brief Update IMU data and check the IMU warm-up situation
 * @param [out] \b AngleCal : Calibrated real-time angle degree {X, Y, Z}
 * @param [out] \b AngleCalShow : Calibrated special filted angle degree for interface display  {X, Y, Z}
 * @param [out] \b SensorTemperature : Filted sensor temperature (C).
 * @param [out] \b Gravity : Current Gravity Direction.
 * @param [out] \b GravityPrevious : Gravity Direction for interface rotation.
 * @param [out] \b fWarmUp : IMU warm-up % (from 0 to 100).
 * @retval Return byte to indicate if update successfully.
 * @retval 0 - Update success.
 * @retval 1 - IMU not warm up.
 * @retval 2 - Recieve data formatting error.
 * @retval 3 - Recieve angle value located outside the threshold.
 * @retval >4 - Times that IMU coped failed. Usually because Serial1 not available.
 * @note If keep returning byte > 2, the TX RX order in Initialize().
 */
byte IMU42688::Update()
{
    // Action 1: Read Data *********************************************
    // Read multiple time in case of wrong coping
    for (int j = 0; j < IMU_C * 2; j++)
    {
        // Step 1: Read Data from Serial1 --------------------------
        CopeFailed++;
        // Serial.print("Reading");
        int StartTime = millis();
        while (Serial1.available() && millis() - StartTime < 1000 && !IMU.newData)
        {
            // Serial.print(".");
            IMU.readSerialData(Serial1.read());
        }
        // Serial.println("");

        float AngleCope[6] = {0};
        byte Gravity_cope = 0;

        // Step 2: Check if Got New Data -------------------------------------
        if (IMU.newData)
        {
            CopeFailed = 0;
            IMU.parseData();
        }
        else
        {
            ErrorCode = CopeFailed - 1 + Err_IMU_Cope_Failed;
            goto NextLoop;
        }

        // Step 3: Read Angle and do basic check -----------------------------
        Gravity_cope = IMU.RecievedIMUData[10];

        // 1. Check Gravity
        if (Gravity_cope < 0 || Gravity_cope > 6)
        {
            ErrorCode = Err_IMU_Receive_Data_Error;
            goto NextLoop;
        }

        // 2. Check Temperature (If sensor temperature have chances to exceed 60 degree, remember to change the check range.)
        if (IMU.RecievedIMUData[9] < 10 || IMU.RecievedIMUData[9] > 60)
        {
            ErrorCode = Err_IMU_Receive_Data_Error;
            goto NextLoop;
        }

        // 3. Check angle data
        for (size_t i = 0; i < 6; i++)
        {
            AngleCope[i] = IMU.RecievedIMUData[i];
            if (AngleCope[i] == 0 || abs(AngleCope[i]) > 180)
            {
                ErrorCode = Err_IMU_Receive_Data_Error;
                goto NextLoop;
            }
        }

        // Step 4: Update data ------------------------------------------------

        // 1. Update gravity
        Gravity = Gravity_cope;
        if (Gravity_cope == 4 || Gravity_cope == 1)
            GravityPrevious = Gravity_cope;

        // 2. Add butterworth filter to temperature (To provide a fluent warm up running bar.)
        SensorTemperatureCollect[1] = SensorTemperatureCollect[0];
        SensorTemperatureCollect[0] = IMU.RecievedIMUData[9];
        SensorTemperature = SensorTemperature * 0.96 + SensorTemperatureCollect[0] * 0.02 + SensorTemperatureCollect[1] * 0.02;

        // 3. Update angle data
        memmove(&Angle[0], &AngleCope[0], sizeof(Angle));
        memmove(&AngleShow[0], &AngleCope[3], sizeof(Angle));

        // 4. Calculate and update calibrated angle data
        for (int i = 0; i < 3; i++)
        {
            AngleCal[i] = (Angle[i] + e[i]) * s[i];
            AngleCalShow[i] = (AngleShow[i] + e[i]) * s[i];
        }

        ErrorCode = IMU_Update_Success;
        Have_New_Data_for_Collect = true;

        // Step 5: Print data to serial "!!! FOR DEBUG ONLY !!!" -------------------------------------
        // Write ALL result onto ONE string and print them together to save the Serial,print time consume
        if (Serial_Print_Update_Data)
        {
            String S_Print = "";
            S_Print += "X : " + String(AngleShow[0], 3) + ", ";
            S_Print += "Y : " + String(AngleShow[1], 3) + ", ";
            S_Print += "Z : " + String(AngleShow[2], 3) + ", ";
            S_Print += "X : " + String(Angle[0], 3) + ", ";
            S_Print += "Y : " + String(Angle[1], 3) + ", ";
            S_Print += "Z : " + String(Angle[2], 3) + ", ";
            S_Print += "Acc1 : " + String(IMU.RecievedIMUData[6]) + ", ";
            S_Print += "Acc2 : " + String(IMU.RecievedIMUData[7]) + ", ";
            S_Print += "Acc3 : " + String(IMU.RecievedIMUData[8]) + ", ";
            S_Print += "Traw : " + String(IMU.RecievedIMUData[9]) + ", ";
            S_Print += "T : " + String(SensorTemperature) + ", ";
            S_Print += "g : " + String(Gravity);
            Debug.println(S_Print);
        }
        break;
    NextLoop:
        // If any error happened, try to read and cope again. (giving IMU_C * 2 chances.)
        // Serial.println(ErrorCode);
        delay(10);
    } // end Read Data

    // Action 2: Initialize sensor temperature relative value ****************************************
    if (Skip_Warm_Up)
    {
        if (fWarmUp != 100 && millis() < 10000)
        {
            ErrorCode = Err_IMU_Not_Warm_Up;
            fWarmUp = (millis()) * 100 / 10000;
            pLED->Set(0, pLED->Y, 1, 3);
        }
        else if (fWarmUp != 100)
        {
            fWarmUp = 100;
            pLED->Set(0, 0, 0, 3);
        }
    }
    else
    {
        // Set sensor temperature buttorworth filter
        if (fWarmUp == 0 && ErrorCode == IMU_Update_Success)
        {
            SensorTemperatureCollect[1] = SensorTemperatureCollect[0];
            SensorTemperature = SensorTemperatureCollect[0];
            fWarmUp++;
        }
        // Set start-warming temperature and temperature stabilization detecting clock
        if (fWarmUp < 3)
        {
            StartTemperature = SensorTemperature;
            SteadyTempCalStart = millis();
            fWarmUp++;
            // Light up IMU warm up hint LED.
            if (pLED)                        // If pLED had been set properly
                pLED->Set(0, pLED->Y, 1, 3); // Set yellow light on LED 0
        }

        // Action 3: Calculate warm-up runing bar *****************************************************
        // Once the warm up complete, the system won't get into warm up situation again no matter how the sensor temperature goes.
        else if (fWarmUp != 100)
        {
            // Step 1: Check if the temperature reach the defined warm-up temperature. ------------------------------------
            if (SensorTemperature < WarmUpTemperature)
            {
                // Step 2: Calculate the warm up running-bar from the sensor temperature. ---------------------------------
                // Used the below calculation to make the warm-up running bar look stable.
                int fWarmUp_t = pow((SensorTemperature - StartTemperature) / (WarmUpTemperature - StartTemperature), 2) * 100;
                // Make sure warm-up running bar goes positive.
                if (fWarmUp_t > fWarmUp)
                {
                    fWarmUp = fWarmUp_t;
                    // Debug.println(String(fWarmUp));
                }

                // Step 3: Calculate the temperature stability, and used it as the secondary warm-up reference. -------------------
                // Define the sensor is warm-up if temperature remain in threshold.
                if (millis() - SteadyTempCalStart < SteadyTempCalPeriod * 1000)
                {
                    MaxTemp = max(MaxTemp, SensorTemperature);
                    minTemp = min(minTemp, SensorTemperature);
                }
                else
                {
                    // Used the below calculation to guess the warm-up percentage from the temperature difference.
                    fWarmUp_t = SteadyTempTH / (MaxTemp - minTemp) * 100;
                    if (fWarmUp_t >= 100)
                        goto Warm_Up_Finish;

                    // Make sure the warm-up running bar goes positive.
                    if (fWarmUp_t > fWarmUp)
                    {
                        fWarmUp = fWarmUp_t;
                        // Debug.println(String(fWarmUp));
                    }

                    // Step 4: Prevent the auto shut-down when waiting for warm-up. ---------------------------------------
                    // Reset the clock every 1.5 minute is enough.
                    // Reseting the clock too often may block the button interrupt.
                    if (fWarmUpTime) // If the pointer to the auto time-off shut-down clock is properly set
                        *fWarmUpTime = millis();

                    // Step 4: Calculate finish. Reset. ----------------------------------------------------------
                    SteadyTempCalStart = millis();
                    MaxTemp = 0;
                    minTemp = WarmUpTemperature;
                }
            }
            else // Step 5: If warm-up complete. -------------------------------------------------------------
            {
            Warm_Up_Finish:
                // Set fWarmUp to 100 to prevent the running-bar from exceeding the frame.
                fWarmUp = 100;
                // Start the auto time-off shut-down clock.
                if (fWarmUpTime) // If the pointer to the auto time-off shut-down clock is properly set
                    *fWarmUpTime = millis();
                // Turn off IMU warm-up LED hint light.
                if (pLED)                  // If pLED had been set properly,
                    pLED->Set(0, 0, 0, 3); // turn off LED hint light.
            }
        } // end Warm Up Calculation
    }
    return ErrorCode;
} // end Update()

/**
 * @brief Get the horizontal bubble level angle.
 * @return float - Degree angle refer to the horizontal bubble level.
 */
float IMU42688::getHorizontal()
{
    float Ha = (Angle[1] + e[1]) * s[1];
    if (Gravity == 1 || Gravity == 4)
    {
        Ha = -(Angle[2] + e[2]) * s[2];
    }
    if (Ha > 180)
        Ha -= 360;
    else if (Ha < -180)
        Ha += 360;
    return Ha;
}

/**
 * @brief Get the vertical bubble level angle.
 * @return float - Degree angle refer to the vertical bubble level.
 */
float IMU42688::getVertical()
{
    float Va = (Angle[0] + e[0]) * s[0];
    if (Gravity == 0 || Gravity == 3)
    {
        Va = 90 - (Angle[2] + e[2]) * s[2];
    }
    if (Va > 180)
        Va -= 360;
    else if (Va < -180)
        Va += 360;
    return Va;
}

/**
 * @brief Get the special filted horizontal bubble level angle.
 * @attention For interface shown only. Shouldn't used in any kinds of calculation.
 * @return float - Degree angle refer to the horizontal bubble level.
 */
float IMU42688::getHorizontalFilt()
{
    float Ha = (AngleShow[1] + e[1]) * s[1];
    if (Gravity == 1 || Gravity == 4)
    {
        Ha = -(AngleShow[2] + e[2]) * s[2];
    }
    if (Ha > 180)
        Ha -= 360;
    else if (Ha < -180)
        Ha += 360;
    return Ha;
}

/**
 * @brief Get the special filted vertical bubble level angle.
 * @attention For interface shown only. Shouldn't used in any kinds of calculation.
 * @return float - Degree angle refer to the vertical bubble level.
 */
float IMU42688::getVerticalFilt()
{
    float Va = (AngleShow[0] + e[0]) * s[0];
    if (Gravity == 0 || Gravity == 3)
    {
        Va = 90 - (AngleShow[2] + e[2]) * s[2];
    }
    if (Va > 180)
        Va -= 360;
    else if (Va < -180)
        Va += 360;
    return Va;
}

/**
 * @brief Convert float degree into String.
 *
 * @param [in] degree Angle for convert (degree).
 * @return String - Angle in degree with length = 6, precision = 2.
 */
String IMU42688::String_degree(float degree)
{
    char A[8];
    dtostrf(degree, 7, 2, A);
    return A;
}

/**
 * @brief Convert float degree into radian String.
 *
 * @param [in] degree Angle for convert (degree).
 * @return String -Angle in radian with length = 6, precision = 4.
 */
String IMU42688::String_rad(float degree)
{
    char A[8];
    dtostrf(degree * PI / 180.0, 7, 4, A);
    return A;
}

/**
 * @brief Convert float degree into mm/m String.
 *
 * @param [in] degree Angle for convert (degree).
 * @return String - Angle in tangent*1000 (mm/m) with length = 6, precision = 1.
 * @note This function return the tangent value to the closet horizontal or vertical reference (-90, 0, 90, or 180 degree)
 */
String IMU42688::String_mm(float degree)
{
    char A[8];
    if (abs(degree) < 45 || abs(degree) > 135)
    {
        float mm = round(tan(degree * PI / 180.0) * 5000) / 5.0;
        dtostrf(mm, 7, 1, A);
    }
    else if (abs(degree - 90) < 45)
    {
        float mm = round(tan((degree - 90) * PI / 180.0) * 5000) / 5.0;
        dtostrf(mm, 7, 1, A);
    }
    else if (abs(degree + 90) < 45)
    {
        float mm = round(tan((degree + 90) * PI / 180.0) * 5000) / 5.0;
        dtostrf(mm, 7, 1, A);
    }
    else
        strcpy(A, "-------");
    return A;
}

/**
 * @brief Convert float degree into String with current angle setting unit.
 *
 * @param [in] degree Angle for convert (degree).
 * @return String - Angle in current unit setlect with length = 6.
 * @retval If unit = 0, return degree with precision = 2.
 * @retval If unit = 1, return mm/m with precision = 1.
 * @retval If unit = 2, return radian with precision = 4.
 * @retval Otherwise, return "------".
 */
String IMU42688::String_now_unit(float degree)
{
    switch (unit)
    {
    case 0:
        return String_degree(degree);
    case 1:
        return String_mm(degree);
    case 2:
        return String_rad(degree);
    }
    return "------";
}

/**
 * @brief Set the interface display imu angle unit.
 * @param [in] Info : String to set the display unit. Case-insensitive. Ignore space.
 * @param [out] unit : Interface levelness display unit.
 *
 * @deprecated Set the unit and save it to the preference memory (Remain the unit setting after system restart).
 * @note If Info include "unit=deg", function will set the display unit into degree. (unit = 0)
 * @note else if Info include "unit=mm/m", function will set the display unit into mm/m. (unit = 1)
 * @note else if Info include "unit=rad", function will set the display unit into radian. (unit = 2)
 * @note Otherwies, do nothing.
 */
void IMU42688::SetUnit(String Info)
{
    Info.toLowerCase();
    Info.replace(" ", "");
    int new_unit = unit;
    if (Info.indexOf("unit=deg") != -1)
    {
        new_unit = 0;
    }
    else if (Info.indexOf("unit=mm/m") != -1)
    {
        new_unit = 1;
    }
    else if (Info.indexOf("unit=rad") != -1)
    {
        new_unit = 2;
    }

    if (new_unit != unit)
    {
        unit = new_unit;
        pref.begin("Angle_Cal", false);
        pref.putInt("Unit", unit);
        pref.end();
        Serial.println(new_unit);
    }
}

/**
 * @brief Collect the angle data if IMU stay stable.
 * @note Will cancel the calibration if the IMU's attitude change severe such that the gravity direction change.
 */
void IMU42688::CollectCalData()
{
    // Check if the angle data already been load.
    if (!Have_New_Data_for_Collect)
        return;
    else
        Have_New_Data_for_Collect = false;

    // Initialize the collection
    if (CalibrateCount == 0)
    {
        StartCalG = Gravity;
        memmove(&StartCalA[0], &Angle[0], sizeof(StartCalA));
    }

    // If the IMU's attitude change severe such that the gravity direction change, cancel the calibration.
    else if (Gravity != StartCalG)
    {
        CalStop();
        SetCalLED(0);
        return;
    }

    // Check the stability.
    for (int i = 0; i < 3; i++)
    {
        // If the IMU's reading outside the threshold, restart the collection.
        if (i != StartCalG % 3 && abs(Angle[i] - StartCalA[i] > Cal_TH))
        {
            CalibrateCount = 1;
            memmove(&StartCalA[0], &Angle[0], sizeof(StartCalA));
            memmove(&SumCalA[0], &Angle[0], sizeof(SumCalA));
            return;
        }
        // else, add the value to the buffer.
        SumCalA[i] += Angle[i];
    }
    CalibrateCount++;
}

/**
 * @brief Calculate the average of the array excluding the angle located outside 2 angle array StDev.
 *
 * @param [in] Angle : Pointer to the float angle array
 * @param [in] Count : Pointer to the bool array for saving the angle value distinguish result.
 * @param [in] Countlength : Lenght of the angle array.
 * @return float : Average value of the input array.
 */
float IMU42688::Avg_in_2StDev(float *Angle, bool *Count, int Countlength)
{
    bool ChangeCount = true;
    double Avg;
    // Calculate until all angel stay inside 2 StDev.
    while (ChangeCount)
    {
        // 1. Calculate the mean value of the angle array.
        float Sum = 0;
        int SumCount = 0;
        for (int i = 1; i < Countlength; i++)
        {
            if (*(Count + i))
            {
                Sum += *(Angle + i);
                SumCount++;
            }
        }
        Avg = Sum / SumCount;
        // 2. Calculate the standard diviation
        Sum = 0;
        for (int i = 1; i < Countlength; i++)
        {
            if (*(Count + i))
                Sum += pow(*(Angle + i) - Avg, 2);
        }
        float Stdev = pow(Sum / SumCount, 0.5);
        // 3. Check if any angle located outside 2 StDev
        ChangeCount = false;
        for (int i = 1; i < Countlength; i++)
        {
            if (*(Count + i))
            {
                if (abs(*(Angle + i) - Avg) > 2 * Stdev)
                {
                    *(Count + i) = false;
                    ChangeCount = true;
                }
            }
        }
    }
    return Avg;
}

/**
 * @brief Set the current attitude as ideal attitude.
 *
 */
void IMU42688::QuickCalibrate()
{
    // Collect angle data.
    CollectCalData();

    // If collection complete, calculate the angle difference between the measure result and ideal value.
    // 1. Identify the axis to calibration in the gravity direction.
    // 2. Identify the ideal value of the axis to calibration.
    // 3. Compare the ideal value and measure value and get the error.
    // 4. Save the error to the flash memory.
    if (CalibrateCount == CalAvgNum)
    {
        pref.begin("Angle_Cal", false);
        switch (StartCalG)
        {
        case 0:
            e[2] = 90 - SumCalA[2] / CalAvgNum;
            pref.putFloat("Ez", e[2]);
            break;
        case 1:
            e[2] = -SumCalA[2] / CalAvgNum;
            pref.putFloat("Ez", e[2]);
            break;
        case 2:
            e[0] = -SumCalA[0] / CalAvgNum;
            e[1] = -SumCalA[1] / CalAvgNum;
            pref.putFloat("Ex", e[0]);
            pref.putFloat("Ey", e[1]);
            break;
        case 3:
            e[2] = -90 - SumCalA[2] / CalAvgNum;
            pref.putFloat("Ez", e[2]);
            break;
        case 4:
            if (SumCalA[2] > 0)
                e[2] = 180 - SumCalA[2] / CalAvgNum;
            else
                e[2] = -180 - SumCalA[2] / CalAvgNum;
            ;
            pref.putFloat("Ez", e[2]);
            break;
        case 5:
            if (SumCalA[0] > 0)
                e[0] = 180 - SumCalA[0] / CalAvgNum;
            else
                e[0] = -180 - SumCalA[0] / CalAvgNum;
            if (SumCalA[1] > 0)
                e[1] = 180 - SumCalA[1] / CalAvgNum;
            else
                e[1] = -180 - SumCalA[1] / CalAvgNum;
            pref.putFloat("Ex", e[0]);
            pref.putFloat("Ey", e[1]);
            break;
        }
        pref.end();
        CalibrateCheck = 2;
        SetCalLED(3);
    }
}

/**
 * @brief Calibrate the zero position and the scale for all three axis.
 * @warning Didn't eliminate the human operation error. Not recommand to use this function to calibrate.
 *
 */
void IMU42688::FullCalibrate()
{
    // Check if the require the current gravity direction infromation.
    if (Gravity > 2 || FullCalComplete[Gravity])
    {
        return;
    }

    CollectCalData();

    // Calculate the angle value when measurement complete
    if (CalibrateCount == CalAvgNum)
    {
        // Get the Average
        FullCalAngle[4 - StartCalG * 2] = SumCalA[(StartCalG + 1) % 3] / CalAvgNum;
        FullCalAngle[5 - StartCalG * 2] = SumCalA[(StartCalG + 2) % 3] / CalAvgNum;

        // Reset data collect buffer.
        CalibrateCount = 0;
        memset(&SumCalA, 0, sizeof(SumCalA));

        // Measure on #n direction complete ( n = Gravity )
        // If calibration procedure isn't finished, back to confirm page.
        FullCalComplete[Gravity] = true;
        CalibrateCheck = 0;
        SetCalLED(2);
    }

    // If all measurment complete, calculate and save the parameter to flash memory.
    if (FullCalComplete[0] && FullCalComplete[1] && FullCalComplete[2])
    {
        pref.begin("Angle_Cal", false);
        char prefkey[9][3] = {"Sx", "Sy", "Sz", "Bx", "By", "Bz", "Ex", "Ey", "Ez"};
        for (int i = 0; i < 3; i++)
        {
            // Calculate the result ------------------------------------------------
            // 1. Calculate Scale factor
            //  - Ideal angle reding between two result is 90.
            //  - Used the scale facter to eliminate the scale error of the IMU
            s[i] = 90.0 * pow(-1, i) / (FullCalAngle[i + 3] - FullCalAngle[i]);
            // 2. Calculate zero position bias
            //  - Compare the angle difference between ideal 0 and measure result.
            //  - Used bias to eliminate this error.
            b[i] = -FullCalAngle[i];
            // 3. Set zero position error into bias.
            e[i] = b[i];

            // Save the Result to flash ---------------------------------------------
            pref.putFloat(&prefkey[i][0], s[i]);
            pref.putFloat(&prefkey[i + 3][0], b[i]);
            pref.putFloat(&prefkey[i + 6][0], e[i]);

            // Print the result to debug file ------------------------------------------
            Debug.print(String(prefkey[i]) + " = " + String(s[i], 6) + ", ");
            Debug.print(String(prefkey[i + 3]) + " = " + String(b[i], 6) + ", ");
            Debug.println(String(prefkey[i + 6]) + " = " + String(e[i], 6));
        }
        Debug.println("Temperature now = " + String(SensorTemperature) + " C");
        pref.end();

        // Calibration Complete
        CalibrateCheck = 2;
        SetCalLED(4);
    }
}

/**
 * @brief Make a full calibration on Z Axis.
 * @warning This function should only be available to manufactorer.
 * @warning This calibration should be done in lab.
 * @note Calibration Procedure :
 * @note 1. Measure the axis Z reading when axis x point to world up direction for 10 times.
 * @note 2. Measure the axis Z reading when axis x point to world down direction for 10 times.
 * @note 3. Calculate the result and finish.
 */
void IMU42688::FullCalibrate_Z()
{
    // Step 1 : Check if the IMU is placed along the required diection. =============
    if (Gravity % 3 != 0 || FullCalComplete[Gravity])
    {
        return;
    }

    // Step 2 : Collect angle data ==================================================
    CollectCalData();

    // Step 3 : Save result to buffer ===============================================
    if (CalibrateCount == CalAvgNum) // When complete collection,
    {
        // Save result to buffer.
        CalibrateCollect[Gravity][CalibrateCollectCount[Gravity]] = SumCalA[2] / CalAvgNum;
        CalibrateCollectCount[Gravity]++;

        // Reset data collect buffer.
        CalibrateCount = 0;
        memset(&SumCalA, 0, sizeof(SumCalA));

        // Measure No. n complete. (n = CalibrateCollectCount[Gravity])
        // If calibration procedure isn't finished, back to confirm page.
        CalibrateCheck = 0;
        SetCalLED(2);
    }

    // Step 4 : Averaging the result =================================================
    if (CalibrateCollectCount[Gravity] == 10) // If complete 10 times collections on curent directions,
    {
        // Average the reading excluding the data outside 2 standard diviation.
        bool CalDevCount[10]; /** @brief bool array for \b Avg_in_2StDev calaulation.*/
        memset(&CalDevCount, true, sizeof(CalDevCount));
        FullCalAngle[(int)Gravity / 3] = Avg_in_2StDev(&CalibrateCollect[Gravity][0], &CalDevCount[0], 10);
        // Rrecord complete.
        FullCalComplete[Gravity] = true;
    }

    // Step 5 : Calaulate rand save the result ==========================================
    if (FullCalComplete[0] && FullCalComplete[3])
    {
        // Calculate the result ------------------------------------------------
        // 1. Calculate Scale factor
        //  - Ideal angle reding between two result is 180.
        //  - Used the scale facter to eliminate the scale error of the IMU
        s[2] = 180 / (FullCalAngle[0] - FullCalAngle[1]);
        // 2. Calculate zero position bias
        //  - The ideal 0 position located at the middle of two reading.
        //  - Used bias to eliminate this error.
        b[2] = -(FullCalAngle[0] + FullCalAngle[1]) / 2.0;
        // 3. Set zero position error into bias.
        e[2] = b[2];

        // Save  result to flash memory --------------------------------------
        pref.begin("Angle_Cal", false);
        pref.putFloat("Sz", s[2]);
        pref.putFloat("Bz", b[2]);
        pref.putFloat("Ez", e[2]);
        pref.end();
        // Print result to debug file. --------------------------------------
        Debug.print("Sz = " + String(s[2], 6) + ", ");
        Debug.print("Bz = " + String(b[2], 6) + ", ");
        Debug.print("Ez = " + String(e[2], 6) + ", ");
        Debug.println("Temperature now = " + String(SensorTemperature) + " C");
        // Step 6 : Calibration Complete ================================================
        CalibrateCheck = 2;
        SetCalLED(5);
    }
}

/**
 * @brief Reset the IMU zero position errors back to origion setting.
 * @note Reset the IMU zero position errors estimate from \b QuickCalibration back to the biases estimate
 * from \b FullCalibrate() or \b FullCalibrate_Z() , and save the result in flash memory.
 */
void IMU42688::ClearZeros()
{
    e[0] = b[0];
    e[1] = b[1];
    e[2] = b[2];
    pref.begin("Angle_Cal", false);
    pref.putFloat("Ex", b[0]);
    pref.putFloat("Ey", b[1]);
    pref.putFloat("Ez", b[2]);
    pref.end();
    CalibrateCheck = 2;
    SetCalLED(3);
}

/**
 * @brief Calibrate the IMU with selected method.
 * @attention Only execute when \b CalibrateCheck == 1.
 */
void IMU42688::Calibrate()
{
    if (CalibrateCheck != 1)
        return;
    if (Cursor == 1)
    {
        QuickCalibrate();
    }
    else if (Cursor == 2)
    {
        ClearZeros();
    }
    else if (Cursor == 3)
    {
        FullCalibrate();
    }
    else if (Cursor == 4)
    {
        FullCalibrate_Z();
    }
}

/**
 * @brief Set all parameters related to the calibration procedure to default.
 */
void IMU42688::CalStop()
{
    // UI relative parameter.
    Cursor = 0;
    CursorStart = 0;
    CalibrateCheck = -1;
    YesNo = false;

    // Calibration data collected relative parameter.
    CalibrateCount = 0;
    FullCalStep = 0;
    memset(&SumCalA, 0, sizeof(SumCalA));
    memset(&FullCalComplete, false, sizeof(FullCalComplete));
    memset(&CalibrateCollectCount, 0, sizeof(CalibrateCollectCount));
    memset(&CalibrateCollect, 0, sizeof(CalibrateCollect));
}

/**
 * @brief Set the LED display light for calibration.
 *
 * @param [in] Flash : Number of times that the LED flash.
 * @note If \b Flash == 0, turn 2 LED off.
 * @note If \b Flash == 1, command 2 LED to flash in cyan in turn.
 * @note If \b Flash > 1, turn LED #0 off and make LED #1 flash in white for \b Flash times.
 */
void IMU42688::SetCalLED(uint8_t Flash)
{
    // System will crash (Load prohibit or Panic) if pointer wasn't setted properly.
    // Check pLED setting first.
    if (pLED)
    {
        switch (Flash)
        {
        case 0: // Turn Off
            pLED->Set(0, 0, 0, Cal_LED_Pri);
            pLED->Set(1, 0, 0, Cal_LED_Pri);
            break;
        case 1: // Flash 2 LED in cyan in turn.
            pLED->Set(0, pLED->C, 2, Cal_LED_Pri);
            pLED->Set(1, pLED->C, 2, Cal_LED_Pri);
            break;
        default: // Flash LED #1 in white for specific times.
            pLED->Set(0, 0, 0, Cal_LED_Pri);
            pLED->Set(1, pLED->W, 2, Cal_LED_Pri, Flash);
            break;
        }
    }
    else
    {
        Serial.println(F("IMU42688::pLED setting error."));
    }
} // end SetCalLED(int8_t Flash)

/**
 * @brief Calibration UI action.
 * @attention Not including the "Back to page ---" action.
 * @exception \b CalibrateSelect() doesn't include the function "Back to page ---" ( i.e. action taken when user press select in calibration select page when \b Cursor = 0).
 * You will need to write the "Back to page ---" function in UI code individually.
 * @param [in] Do : Button press action.  0 : Select, 1:Up, 2: Down, 3 :Min, 4 : Add
 * @param [out] \b Cursor : Calibration method selection cursor.
 * @param [out] \b CursorStart : Method selection list start displaying flag
 * @param [out] \b YesNo : Yes No Cursor on action confirm page.
 * @param [out] \b CalibrationCheck : Indicate the user get into and pass through the confirm page.
 * @note Can rewrite or delete this function and relative parameter if change the user interface logic.
 */
void IMU42688::CalibrateSelect(byte Do)
{
    switch (CalibrateCheck)
    {
    case -1: // Calibration method selection

        if (Do == 1 && Cursor > 0)                                                                         // If user press up and still exist space for cursor to go up,
            Cursor--;                                                                                      // cursor move up.
        if (Do == 2 && Cursor < ((ExpertMode) ? 4 : 2))                                                    // If user press down and still exist space for cursor to go down,
            Cursor++;                                                                                      // cursor move down.
        if (Do == 0 && Cursor != 0)                                                                        // If user press select and cursor doesn't point to "back to page ---" selection,
            CalibrateCheck = 0;                                                                            // go into the calibration confirm page.
        CursorStart = max(min((int)Cursor, (int)CursorStart), (int)Cursor - Calibrate_Display_Length + 1); // Calculate the list start displaying flag.
        YesNo = (Cursor > 2);                                                                              // Set Default Yes No Cursor for action confirm page.
        break;
    case 0: // Action Confirm

        /* In full calibration, stop calibration WHEN USER PRESS SELECT when
         1. Gravity > 2, since the full calibration only need the data from Gravity = 0, 1, 2.
         2. Current gracity direction's data had been collected already.
        */
        if (Cursor == 3 && (Gravity > 2 || FullCalComplete[Gravity]))
        {
            if (Do == 0)
                CalStop();
        }
        /* In Z direction full calibration, stop calibration WHEN USER PRESS SELECT when
         1. Gravity = 1, 2, 4, or 5, since the z direction full calibration only need the data from Gravity = 0 and 3.
         2. Current gracity direction's data had been collected already.
        */
        else if (Cursor == 4 && (Gravity % 3 != 0 || FullCalComplete[Gravity]))
        {
            if (Do == 0)
                CalStop();
        }
        else
        {
            if (Do == 3)           // Press Minus
                YesNo = false;     // Yes No Cursor point to NO
            if (Do == 4)           // Press Add
                YesNo = true;      // Yes No Cursor Point to YES
            if (Do == 0 && !YesNo) // If Select No
                CalStop();         // Stop the calibration.
            if (Do == 0 && YesNo)  // If Select Yes
            {
                CalibrateCheck = 1; // Finish the confirmation and start calibration data collect.
                SetCalLED(1);       // Set data collecting hint light.
            }
        }
        break;
    case 1:
        if (Do >= 0 && Do < 5) // If have any action when collecing data.
        {
            CalStop();    // Stop calibration.
            SetCalLED(0); // Turn off hint light immidately.
        }
        break;
    }
} // end CalibrateSelect(byte Do)
