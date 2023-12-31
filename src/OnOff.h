#ifndef OnOff_H
#define OnOff_H
#include <Arduino.h>
#include <OLED.h>
#include <SLED.h>
#include <Preferences.h>

class OnOff
{
private:
    const int LP_Clock = 2000;             /** @brief Time to trigger Long Press power off.*/
    const int SH_Clock = 2500;             /** @brief Time period for showing the power off image.*/
    int TO_Clock[5] = {5, 15, 30, 60, 90}; /** @brief Time to trigger the system time out power off.*/
    int OffClock = 0;
    Preferences Sleep_pref;
    OLED *pOLED;
    SLED *pLED;
    byte ButPin;
    byte EN_Pin;

public:
    int LastEdit = 0;
    int SleepTimeSelect = 1;

    /**
     * @brief System power on procedure.
     *
     * @param WakeUpPin Button to detect wake up.
     * @param IO_EN Power controll GPIO. Set high when system power on.
     * @param LED Pointer to SLED.
     * @param Bat Pointer to battery.
     * @param oled Pointer to oled.
     */
    void On(byte WakeUpPin, byte IO_EN, SLED &LED, OLED &oled)
    {
        // Enable VCC
        pinMode(WakeUpPin, INPUT);
        pinMode(IO_EN, OUTPUT);
        digitalWrite(IO_EN, HIGH);
        // Reset OLED
        oled.Do_RST();
        // Light Up LED
        LED.Initialize();
        LED.Set(0, LED.W, 1, 4);
        LED.Set(1, LED.W, 1, 4);
        LED.Update();
        // Serial begin
        Serial.begin(115200);
        // Detect long press
        while (millis() < LP_Clock && digitalRead(WakeUpPin))
        {
            delay(100);
        }
        // If is not long press
        if (millis() < LP_Clock)
        {
            // Print Error
            Serial.println("Wake up count Failed");
            // Turn Off VCC
            digitalWrite(IO_EN, LOW);
            // Wait for power off
            while (true)
            {
            }
        }
        //
        pOLED = &oled;
        pLED = &LED;
        ButPin = WakeUpPin;
        EN_Pin = IO_EN;
        LED.Set(1, 0, 0, 4);
        LED.Update();
        Sleep_pref.begin("Sleep");
        SleepTimeSelect = Sleep_pref.getInt("time", 1);
        Sleep_pref.end();
    }

    /**
     * @brief Start the long press detection clock
     */
    void Off_Clock_Start()
    {
        OffClock = millis();
    }

    /**
     * @brief Stop the long press detection clock
     */
    void Off_Clock_Stop()
    {
        OffClock = 0;
    }

    /**
     * @brief Check if system should power off
     *
     */
    void Off_Clock_Check()
    {
        // Check whether power off require
        bool PressSleep = (OffClock == 0) ? false : (millis() - OffClock) > LP_Clock;
        bool TimeOffSleep = ((millis() - LastEdit > TO_Clock[SleepTimeSelect] * 60 * 1000) && millis() > 10 ^ 7);
        if (OffClock != 0)
            pLED->Set(0, pLED->W, 1, 4);
        if (!PressSleep && !TimeOffSleep)
            return;

        // Show Power Off Command
        if (PressSleep)
        {
            pOLED->Block("Shutting Down", SH_Clock * 2);
            Serial.println(F("Command Sleep"));
        }
        else if (TimeOffSleep)
        {
            pOLED->Block("Auto Sleep", SH_Clock * 2);
            Serial.println(F("Auto Sleep"));
        }

        // Power Off
        if (PressSleep || TimeOffSleep)
        {
            // Turn On LED
            pLED->Set(0, pLED->W, 1, 4);

            // Wait for Powe Off Display finish
            int ForShow = millis();
            while (millis() - ForShow < SH_Clock)
            {
                // If user press button when showing time out power off, stop the power off procedure.
                if (TimeOffSleep)
                {
                    if (millis() - LastEdit < SH_Clock)
                    {
                        pOLED->flag.BlockTime = millis();
                        pLED->Set(0, 0, 0, 4);
                        return;
                    }
                }
            }
            // Power Off Confirm
            pOLED->TurnOff();
            // Wait for button release
            while (digitalRead(ButPin))
            {
            }
            // Let user know we had power off
            pLED->Clear();
            Serial.println("Sleep");
            // Power Off
            digitalWrite(EN_Pin, LOW);
            while (true)
                ;
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
        if (!isAdd && SleepTimeSelect > 0)
        {
            SleepTimeSelect--;
            Sleep_pref.begin("Sleep");
            Sleep_pref.putInt("time", SleepTimeSelect);
            Sleep_pref.end();
        }
    }
};

#endif