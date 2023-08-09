#ifndef LongPressSwich_H
#define LongPressSwich_H

#include <Arduino.h>
#include "OLED.h"
#include "SDCard.h"
#include "SerialDebug.h"
#include "LEDFlash.h"
#include "OLED.h"
#include "Battery.h"
#include "esp_bt_main.h"
#include "esp_bt.h"
#include "Preferences.h"
RTC_DATA_ATTR int bootCount = -1;

class LongPressSwich
{
private:
    int LEDFlashCount = 0;
    int OffClock = 0;
    byte SWPin[2];
    byte ButPin;
    LEDFlash *pLED;
    Battery *pBattery;
    OLED *pOLED;
    int Sleep_T[5] = {5, 15, 30, 60, 90};
    Preferences Sleep_pref;

public:
    SDCard *pSD;
    int *LastTriggure;
    int SleepTimeSelect = 1;
    void On(gpio_num_t WakeUpPin, byte Swich_Pin1, byte Swich_Pin2, LEDFlash &LED, Battery &Bat, OLED &oled)
    {
        esp_sleep_enable_ext0_wakeup(WakeUpPin, 0);
        bootCount++;
        SWPin[0] = Swich_Pin1;
        SWPin[1] = Swich_Pin2;
        pinMode(SWPin[0], OUTPUT);
        digitalWrite(SWPin[0], HIGH);
        pinMode(SWPin[1], OUTPUT);
        digitalWrite(SWPin[1], HIGH);
        // Begine from sleeping
        if (bootCount == 0)
        {
            oled.Clear();
            Wire.end();
            digitalWrite(SWPin[0], LOW);
            digitalWrite(SWPin[1], LOW);
            esp_deep_sleep_start();
        }
        // Setting
        Serial.setRxBufferSize(256);
        Serial.begin(115200);
        Serial.println("Start");
        pOLED = &oled;
        pBattery = &Bat;
        pLED = &LED;
        ButPin = WakeUpPin;
        // Battary Test
        pBattery->Update();
        pinMode(IO_OLED_RST, OUTPUT);
        digitalWrite(IO_OLED_RST, LOW);
        delay(1);
        digitalWrite(IO_OLED_RST, HIGH);
        if (!TestVersion)
        {
            if (pBattery->Percent < 0)
            {
                pLED->Set(0, LED.R, 1, 5);
                pLED->Update();
                pOLED->ShowLowPower();
                while (!digitalRead(ButPin))
                    delay(10);
                esp_deep_sleep_start();
            }
        }

        pLED->Set(0, LED.W, 1, 5);
        pLED->Update();
        // Detect 3 s long press
        pinMode(ButPin, INPUT);
        while (millis() < 3000 && !digitalRead(ButPin))
        {
            delay(100);
        }
        if (millis() < 3000)
        {
            pOLED->Clear();
            digitalWrite(SWPin[0], LOW);
            digitalWrite(SWPin[1], LOW);
            esp_deep_sleep_start();
        }
        Sleep_pref.begin("Sleep");
        SleepTimeSelect = Sleep_pref.getInt("time", 1);
        Sleep_pref.end();
    }

    void Off_Clock_Start()
    {
        OffClock = millis();
    }

    void Off_Clock_Stop()
    {
        OffClock = 0;
    }

    void Off_Clock_Check()
    {
        bool PressSleep = false;
        if (OffClock == 0)
            PressSleep = false;
        else if (millis() - OffClock > 3000)
            PressSleep = true;
        else
        {
            pLED->Set(0, pLED->W, 1, 5);
        }
        bool TimeOffSleep = ((millis() - *LastTriggure > Sleep_T[SleepTimeSelect] * 60 * 1000) && (OffClock == 0) && millis() > 15 * 60 * 1000 && !TestVersion);
        bool LowPowerOff = (pBattery->Percent < 0);

        if (!PressSleep && !TimeOffSleep && !LowPowerOff)
        {
            return;
        }
        if (TestVersion)
        {
            LowPowerOff = false;
        }

        if (PressSleep)
        {
            pOLED->Block("Shutting Down");
            Debug.println("Command Sleep");
        }
        else if (LowPowerOff)
        {
            pOLED->Block("Low Battery");
            Debug.println("Low Battery Sleep");
        }
        else if (TimeOffSleep)
        {
            pOLED->Block("Auto Sleep");
            Debug.println("Auto Sleep");
        }
        if (PressSleep || TimeOffSleep || LowPowerOff)
        {
            pLED->Set(0, pLED->W, 1, 5);
            Debug.println("[Battery] Battery " + String(pBattery->Percent) + " %");
            int ForShow = millis();
            if (pSD)
            {
                // Save Debug message.
                String T = "";
                pSD->Save("", T);
            }
            while (millis() - ForShow < 2500)
            {
                if (TimeOffSleep)
                {
                    if (digitalRead(ButPin) == 0)
                    {
                        *LastTriggure = millis();
                        pLED->Set(0, 0, 0, 5);
                        return;
                    }
                    delay(1);
                }
            }
            pOLED->TurnOff();
            while (digitalRead(ButPin) == 0)
            {
                // Wait until release Button
            }
            esp_bt_controller_disable;
            esp_bluedroid_disable;
            Wire.end();
            SPI.end();
            Serial.end();
            pLED->Set(1, pLED->W, 1, 5);
            pLED->Update();
            digitalWrite(SWPin[0], LOW);
            digitalWrite(SWPin[1], LOW);
            Serial.println("Sleep");
            esp_deep_sleep_start();
        }
    }

    void SleepTimeAdjust(bool isAdd)
    {
        if (isAdd && SleepTimeSelect < 4)
        {
            SleepTimeSelect++;
            Sleep_pref.begin("Sleep");
            Sleep_pref.putInt("time", SleepTimeSelect);
            Sleep_pref.end();
        }
        if(!isAdd && SleepTimeSelect > 0)
        {
            SleepTimeSelect--;
            Sleep_pref.begin("Sleep");
            Sleep_pref.putInt("time", SleepTimeSelect);
            Sleep_pref.end();
        }
    }
};
#endif