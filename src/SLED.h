#ifndef SLED_H
#define SLED_H
#include <Adafruit_NeoPixel.h>
#ifndef IO_LED
#define IO_LED 40
#endif
#define LED_COUNT 2

class SLED
{
private:
    bool Block = false;
    bool isBegin = false;
    byte Pin[LED_COUNT][3] = {0};
    byte Count[LED_COUNT][7];
    byte Color[LED_COUNT][7];
    byte Close[LED_COUNT][7];
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
    void Initialize();
    void Update();
    void Clear();
    void Set(byte channel, byte color, byte Period, int Priority, byte Times /* 0 never clear */);
    void Set(byte channel, byte color, byte Period, int Priority);
    void SetBrightness(byte Brightness);
};

#endif