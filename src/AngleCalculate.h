#ifndef AngleCalculate_H
#define AngleCalculate_H
#include <Arduino.h>
#include "LEDFlash.h"

class AngleCalculate
{
private:
    float StableAngle[2] = {200};
    float MeasureAngle[3] = {0};
    size_t StableArraySize = sizeof(StableAngle);
    size_t MeasureArraySize = sizeof(MeasureAngle);
    byte StableCount = 0;

public:
    const byte Sleep = 0;
    const byte Not_Stable = 1;
    const byte Measuring = 2;
    const byte Done = 3;
    byte MeasureCount = 0;
    byte State = 0;
    byte *G;
    LEDFlash *pLED;
    float ResultAngle[4] = {0};
    float *InputAngle;

    void Switch(bool OnOff)
    {
        if (OnOff && (State == Sleep || State == Done))
        {
            State = Not_Stable;
            memset(StableAngle, 200, StableArraySize);
            memset(MeasureAngle, 0, MeasureArraySize);
            StableCount = 0;
            MeasureCount = 0;
            pLED->Set(0, pLED->G, 4, 3);
        }
        if (!OnOff)
        {
            State = Sleep;
            memset(StableAngle, 200, StableArraySize);
            memset(MeasureAngle, 0, MeasureArraySize);
            StableCount = 0;
            MeasureCount = 0;
            pLED->Set(0, pLED->K, 0, 3);
        }
    }

    byte Input()
    {
        if (State == Sleep || State == Done)
        {
            return State;
        }
        float Angle0 = *(InputAngle + (*G + 1) % 3);
        float Angle1 = *(InputAngle + (*G + 2) % 3);
        if (StableCount < 5 || abs(StableAngle[0] - Angle0) > 0.1 || abs(StableAngle[1] - Angle1) > 0.1)
        {
            StableCount++;
            if (abs(StableAngle[0] - Angle0) > 0.05 || abs(StableAngle[1] - Angle1) > 0.05)
            {
                StableAngle[0] = Angle0;
                StableAngle[1] = Angle1;
                StableCount = 0;
            }
            memset(MeasureAngle, 0, MeasureArraySize);
            MeasureCount = 0;
            State = Not_Stable;
            pLED->Set(0, pLED->G, 4, 3);
            return Not_Stable;
        }
        else
        {
            MeasureAngle[0] += *InputAngle;
            MeasureAngle[1] += *(InputAngle + 1);
            MeasureAngle[2] += *(InputAngle + 2);
            MeasureCount++;
            if (MeasureCount == 10)
            {
                ResultAngle[0] = MeasureAngle[0] / MeasureCount;
                ResultAngle[1] = MeasureAngle[1] / MeasureCount;
                ResultAngle[2] = MeasureAngle[2] / MeasureCount;
                memset(StableAngle, 200, StableArraySize);
                memset(MeasureAngle, 0, MeasureArraySize);
                StableCount = 0;
                MeasureCount = 0;
                State = Done;
                pLED->Set(0, pLED->G, 1, 3);
                return Done;
            }
            State = Measuring;
            pLED->Set(0, pLED->G, 2, 3);
            return Measuring;
        }
    }
};

#endif