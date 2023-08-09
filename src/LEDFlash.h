#ifndef LEDFlash_H
#define LEDFlash_H
#include <Arduino.h>
#include "SerialDebug.h"
#define LED_NUM 2

class LEDFlash
{
private:
    byte Pin[LED_NUM][3] = {0};
    byte Count[LED_NUM][7];
    byte Color[LED_NUM][7];
    byte Close[LED_NUM][7];
    int count = 0;

public:
    const byte K = 0; // 000
    const byte R = 1; // 001
    const byte G = 2; // 010
    const byte Y = 3; // 011
    const byte B = 4; // 100
    const byte M = 5; // 101
    const byte C = 6; // 110
    const byte W = 7; // 111
    void SetPin(byte channel, byte IO_R, byte IO_G, byte IO_B)
    {
        for (int i = 0; i < 3; i++)
        {
            if (Pin[channel][i] != 0)
            {
                digitalWrite(Pin[channel][i], HIGH);
            }
        }
        Pin[channel][0] = IO_R;
        Pin[channel][1] = IO_G;
        Pin[channel][2] = IO_B;
        memset(Count, 0, sizeof(Count));
        memset(Color, 0, sizeof(Color));
        memset(Close, 0, sizeof(Close));
        for (int i = 0; i < 3; i++)
        {
            if (Pin[channel][i] != 0)
            {
                pinMode(Pin[channel][i], OUTPUT);
                digitalWrite(Pin[channel][i], HIGH);
            }
        }
    }

    void Set(byte channel, byte color, byte Period, int Priority, byte Times /* 0 never clear */)
    {
        Priority = min(max(Priority, 0), 6);
        Color[channel][Priority] = (Period == 0) ? K : color;
        Count[channel][Priority] = Period;
        Close[channel][Priority] = Times;
    }

    void Set(byte channel, byte color, byte Period, int Priority)
    {
        Set(channel, color, Period, Priority, 0);
    }

    void Update()
    {
        count++;
        bool anyLight = false;
        for (int i = 0; i < LED_NUM; i++)
        {
            int P;
            // Find the current highest priority flash
            for (P = 6; P > -1; P--)
            {
                if (Count[i][P] != 0)
                    break;
            }
            // Cancle all non-perminate less priority flash
            for (int j = P - 1; j > -1; j--)
            {
                if (Close[i][j] != 0)
                    Set(i, 0, 0, j);
            }
            // Flash
            if (P > 0 && count % Count[i][P] == i)
            {
                if (Pin[i][0] != 0)
                    digitalWrite(Pin[i][0], !(Color[i][P] % 2 == 1));
                if (Pin[i][1] != 0)
                    digitalWrite(Pin[i][1], !(Color[i][P] % 4 / 2 == 1));
                if (Pin[i][2] != 0)
                    digitalWrite(Pin[i][2], !(Color[i][P] / 4 == 1));
                
                if (Close[i][P] == 1)
                    Set(i, 0, 0, P);
                else if (Close[i][P] != 0)
                    Close[i][P]--;
            }
            else
            {
                if (Pin[i][0] != 0)
                    digitalWrite(Pin[i][0], HIGH);
                if (Pin[i][1] != 0)
                    digitalWrite(Pin[i][1], HIGH);
                if (Pin[i][2] != 0)
                    digitalWrite(Pin[i][2], HIGH);
            }
            anyLight = (P > 0) ? true : anyLight;
        }
        if (!anyLight)
        {
            for (int i = 0; i < LED_NUM; i++)
            {
                if(Pin[i][1] != 0 && count % 80 == i)
                     digitalWrite(Pin[i][1], LOW);
            }
        }
    }
};

#endif