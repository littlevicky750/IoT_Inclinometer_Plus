#ifndef Battery_H
#define Battery_H

#include <Arduino.h>
class Battery
{
private:
    byte p;

public:
    int Percent = 0;
    void SetPin(byte Pin)
    {
        p = Pin;
        pinMode(p, INPUT);
    }
    void Update()
    {
        int TimeStamp = millis();
        int Count = 0;
        int Sum = 0;
        while (millis() - TimeStamp < 100)
        {
            int B = analogRead(p);
            
            if (B != 0)
            {
                Sum += B;
                Count++;
            }
            delay(1);
        }
        if (Count != 0)
        {
            // Serial.println(Sum / Count);
            // 4.2 -> 2370, 3.6-> 2120
            Percent = (((float)Sum / Count + 1.0 - 2120.0)  * 100.0/ 150.0 + 0.5);
            Percent = min(max(Percent,0),100);
            Percent = (float)Sum / Count;
        }
            
    }
};

#endif