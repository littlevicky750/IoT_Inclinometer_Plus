#ifndef Battery_H
#define Battery_H

#include <Arduino.h>
#include <Preferences.h>
class Battery
{
private:
    byte p;
    int Full_V;
    Preferences pref;

public:
    int Percent = 0;
    bool SetMax = false;
    bool Restore = false;
    void SetPin(byte Pin)
    {
        p = Pin;
        pinMode(p, INPUT);
        pref.begin("Battery", true);
        Full_V = pref.getShort("V", 2520);
        pref.end();
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
        if (SetMax)
        {
            Full_V = (Sum / Count) / 10 * 10;
            pref.begin("Battery", false);
            pref.putShort("V", Full_V);
            pref.end();
            Serial.println(Full_V);
            SetMax = false;
        }
        if (Restore)
        {
            Full_V = 2520;
            pref.begin("Battery", false);
            pref.putShort("V", Full_V);
            pref.end();
            Serial.println(Full_V);
            Restore = false;
        }
        if (Count != 0)
        {
            // Serial.println(Sum / Count);
            Percent = (((float)Sum / Count + 1.0 - 2170.0) * 100.0 / (Full_V - 2170) + 0.5);
            Percent = min(max(Percent, 0), 100);
        }
    }
};

#endif