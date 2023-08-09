#ifndef Measure_H
#define Measure_H
#include <Arduino.h>

#define Measure_Num 17
#define Stable_Num 17

#include <MaxMin.h>

class Measure
{
private:
    const bool Print_Result = true;                          /** @brief True to print result for debug.*/
    const bool Print_Stable_Situation = true;                /** @brief True to print stabilization situation for debug.*/
    float StableStart[Stable_Num] = {0};                     /** @brief Sensor stabilization identification reference position.*/
    float Stable_TH[Stable_Num] = {0};                       /** @brief Sensor stabilization identification threshold.*/
    float Measure_TH[Stable_Num] = {0};                      /** @brief Sensor moving identification threshold.*/
    float *Stable[Stable_Num];                               /** @brief Array of pointer to the data for identify the stabalization of the sensors.*/
    float *Input[Measure_Num];                               /** @brief Array of pointer to the measure data.*/
    float Measure[Measure_Num] = {0};                        /** @brief Measure buffer.*/
    bool isUpdate[Measure_Num] = {0};                        /** @brief Bool array to indicate if the data had been update.*/
    byte StableCount = 0;                                    /** @brief Stabilization identification Data collection counter.*/
    byte MeasureCount = 0;                                   /** @brief Measure Data collection counter.*/
    const byte StableCountNum = 5;                           /** @brief Require data length for stabilization identification .*/
    const byte MeasureCountNum = 20;                         /** @brief Require data length for measurement.*/
    const float Stable_Default_Value = 1000;                 /** @brief Default stabilization identification reference position.*/
    const size_t StableStartArraySize = sizeof(StableStart); /** @brief Array size of stable data.*/
    const size_t MeasureArraySize = sizeof(Measure);         /** @brief Array size of measure data.*/

    /**
     * @brief Set the To Default value
     *
     */
    void SetToDefault()
    {
        for (int i = 0; i < Stable_Num; i++)
        {
            StableStart[i] = Stable_Default_Value;
        }
        memset(Measure, 0, MeasureArraySize);
        StableCount = 0;
        MeasureCount = 0;
    }

public:
    const byte Sleep = 0;
    const byte Not_Stable = 1;
    const byte Measuring = 2;
    const byte Done = 3;
    byte State = 0;                  /** @brief Measurement State.*/
    float MeasurePercent = 0;        /** @brief Measurement data collect percentage.*/
    float Result[Measure_Num] = {0}; /** @brief Measurement result output*/

    /**
     * @brief Set the pointers to the float which are used for measurement.
     *
     * @param pInput Pointer to the first input float.
     * @param Start Save the first pointer at #Start of the pointer array.
     * @param Length Keep saving the pointer for #Length times.
     */
    void SetInput(float *pInput, int Start, int Length)
    {
        int Length_M = (Start + Length > Measure_Num) ? Measure_Num - Start : Length;
        for (int i = 0; i < Length_M; i++)
        {
            Input[Start + i] = pInput + i;
        }
    }

    /**
     * @brief Set the pointers to the float which are used for stablization identification
     *
     * @param pInput Pointer to the first input float.
     * @param Start Save the first pointer at #Start of the pointer array.
     * @param Length Keep saving the pointer for #Length times.
     * @param S_TH Relative stablization identification threshold.
     * @param M_TH Relative movement identification (while measuring) threshold.
     */
    void SetStable(float *pInput, int Start, int Length, float S_TH, float M_TH)
    {
        int Length_S = (Start + Length > Stable_Num) ? Stable_Num - Start : Length;
        for (int i = 0; i < Length_S; i++)
        {
            Stable[Start + i] = pInput + i;
            Measure_TH[Start + i] = M_TH;
            Stable_TH[Start + i] = S_TH;
        }
    }

    /**
     * @brief Set the pointers to the float which are used for both stablization identification and measurement.
     *
     * @param pInput Pointer to the first input float.
     * @param Start Save the first pointer at #Start of the pointer array.
     * @param Length Keep saving the pointer for #Length times.
     * @param S_TH Relative stablization identification threshold.
     * @param M_TH Relative movement identification (while measuring) threshold.
     */
    void Set(float *pInput, int Start, int Length, float S_TH, float M_TH)
    {
        SetInput(pInput, Start, Length);
        SetStable(pInput, Start, Length, S_TH, M_TH);
    }

    /**
     * @brief Enable or disable the measuremnet procedure
     *
     * @param OnOff True to start measurement, false to stop the measurement.
     */
    void Switch(bool OnOff)
    {
        if (OnOff && (State == Sleep || State == Done))
        {
            State = Not_Stable;
            SetToDefault();
        }
        if (!OnOff)
        {
            State = Sleep;
            SetToDefault();
        }
    }

    /**
     * @brief Indicete the data update situcation.
     *
     * @param Start Poisition of the float in the array that had been update successifully.
     * @param Length Number of float that had been update successiflly
     */
    void DataIsUpdte(int Start, int Length)
    {
        for (int i = 0; i < Length; i++)
        {
            isUpdate[Start + i] = true;
        }
    }

    byte Update()
    {
        // Check State
        if (State == Sleep || State == Done)
        {
            return State;
        }
        // Check Stable
        bool isStable = (StableCount >= StableCountNum);
        // Check if all Update
        for (int i = 0; i < Stable_Num; i++)
        {
            if (!isUpdate[i])
            {
                return isStable ? Measuring : Not_Stable;
            }
        }
        for (int i = 0; i < Stable_Num; i++)
        {
            isUpdate[i] = false;
        }
        // Check Stable when measureing
        if (isStable)
        {
            for (int i = 0; i < Stable_Num; i++)
            {
                // Check if pointer set properly
                if (Stable[i])
                {
                    if (abs(*Stable[i] - StableStart[i]) > Measure_TH[i])
                    {
                        if (Print_Stable_Situation)
                            Serial.printf("Data %d exceed measure threshold : %f / %f .\n", i, *Stable[i], StableStart[i]);
                        isStable = false;
                        break;
                    }
                }
            }
        }
        // Check or Collect Data
        if (!isStable)
        {
            StableCount++;
            // Check Stable
            isStable = true;
            for (int i = 0; i < Stable_Num; i++)
            {
                // Check if pointer set properly
                if (Stable[i])
                {
                    // Check if each data stay within threshold
                    if (abs(*Stable[i] - StableStart[i]) > Stable_TH[i])
                    {
                        if (Print_Stable_Situation && StableStart[i] != 1000)
                            Serial.printf("Data %d exceed stable threshold : %f / %f .\n", i, *Stable[i], StableStart[i]);
                        isStable = false;
                        break;
                    }
                }
            }
            // If exceed threshold, record current data as the StableStart
            if (!isStable)
            {
                for (int i = 0; i < Stable_Num; i++)
                {
                    StableStart[i] = (Stable[i]) ? *Stable[i] : 0;
                }
                StableCount = 0;
            }
            memset(Measure, 0, MeasureArraySize);
            MeasureCount = 0;
            State = Not_Stable;
            return Not_Stable;
        }
        else
        {
            // Measure
            for (int i = 0; i < Measure_Num; i++)
            {
                Measure[i] += (Input[i]) ? *Input[i] : 0;
            }
            MeasureCount++;
            MeasurePercent = (float)MeasureCount / MeasureCountNum;
            // If measure complete
            if (MeasureCount == MeasureCountNum)
            {
                if (Print_Result)
                {
                    String S = "";
                    for (int i = 0; i < Measure_Num; i++)
                    {
                        Result[i] = Measure[i] / MeasureCountNum;
                        S += String(Result[i], 1) + " ,";
                    }
                    Serial.println(S);
                }

                // Set to default
                SetToDefault();
                State = Done;
                return Done;
            }
            State = Measuring;
            return Measuring;
        }
    }
};

#endif