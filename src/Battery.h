#ifndef Battery_H
#define Battery_H

#include <Arduino.h>
class Battery
{
public:
    int Percent = 0;
private:
    byte p;
    int V[2] = {0};
    float V_BW = 0;
    void V2P(float V_read)
    {
        // 4.2 -> 2370, 3.6-> 2120
        Percent = ((V_read - 2120.0) * 100.0 / 250.0 + 0.5);
        Percent = min(max(Percent, 0), 100);
    }
public:
    void SetPin(byte Pin)
    {
        p = Pin;
        pinMode(p, INPUT);
        V[0] = analogRead(p);
        V_BW = analogRead(p);
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
            V2P((float)Sum / Count + 1.0);
            V[0] = Sum / Count + 1.0;
            V_BW = V[0];
        }
    }
    void Update_BW()
    {
        V[1] = V[0];
        V[0] = analogRead(p);
        V_BW = V_BW * 0.96 + V[0] * 0.02 + V[1] * 0.02;
        V2P(V_BW);
        // Serial.println(V_BW);
    }
};

#endif