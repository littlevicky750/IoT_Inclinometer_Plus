#include "SLED.h"

Adafruit_NeoPixel strip(LED_COUNT, IO_LED, NEO_GRB + NEO_KHZ800);

void SLED::Initialize()
{
    strip.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
    strip.show();  // Turn OFF all pixels ASAP
    strip.setBrightness(32);
    isBegin = true;
}

/**
 * @brief Set the LED color.
 * 
 * @param channel # of LED
 * @param color LED clolr
 * @param Period LED flash period ( 0 : clear the current priority setting , 1 : always on )
 * @param Priority Priority of the light setting
 * @param Times Clear the setting after flashing for specific Times ( 0 if never clear ).
 */
void SLED::Set(byte channel, byte color, byte Period, int Priority, byte Times /* 0 never clear */)
{
    Priority = min(max(Priority, 0), 6);
    Color[channel][Priority] = (Period == 0) ? K : color;
    Count[channel][Priority] = Period;
    Close[channel][Priority] = Times;
}

/**
 * @brief Set the LED color.
 * 
 * @param channel # of LED
 * @param color LED color (WRGBMCYK)
 * @param Period LED flash period ( 0 : clear the current priority setting , 1 : always on )
 * @param Priority Priority of the light setting
 */
void SLED::Set(byte channel, byte color, byte Period, int Priority)
{
    Set(channel, color, Period, Priority, 0);
}

void SLED::Update()
{
    if (Block)
        return;
    count++;
    bool anyLight = false;
    uint32_t SetColor;
    for (int i = 0; i < LED_COUNT; i++)
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
        if (P >= 0 && (count % Count[i][P] == i || Count[i][P] == 1))
        {
            SetColor = strip.Color((Color[i][P] % 2 == 1) * 255, (Color[i][P] % 4 / 2 == 1) * 255, (Color[i][P] / 4 == 1) * 255);
            strip.setPixelColor(i, SetColor);
            if (Close[i][P] == 1)
                Set(i, 0, 0, P);
            else if (Close[i][P] != 0)
                Close[i][P]--;
        }
        else
        {
            SetColor = strip.Color(0, 0, 0);
            strip.setPixelColor(i, strip.Color(0, 0, 0));
        }
        anyLight = (P >= 0) ? true : anyLight;
    }
    if (!anyLight)
    {
        for (int i = 0; i < LED_COUNT; i++)
        {
            strip.setPixelColor(i, SetColor);
            if (Pin[i][1] != 0 && count % 80 == i)
                strip.setPixelColor(i, strip.Color(0, 1, 0));
        }
    }
    strip.show();
}

void SLED::Clear()
{
    strip.clear();
    strip.show();
    Block = true;
}

/**
 * @brief Set LED brightness
 * 
 * @param Brightness Brightness scale (0 ~ 255)
 */
void SLED::SetBrightness(byte Brightness)
{
    strip.setBrightness(Brightness);
}
