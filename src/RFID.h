#ifndef RFID_H
#define RFID_H
#include "MFRC522_I2C.h"
#include "SerialDebug.h"
#include "LEDFlash.h"

#ifndef RFID_RST
#define RFID_RST -1
MFRC522 mfrc522(0x28, 1);
#else
MFRC522 mfrc522(0x28, RFID_RST);
#endif

class RFID
{
private:
    bool isRFIDConnect = true;
    int ReConnectCount = 0;
    int Count = 0;
    int LastDetect = 0;
    byte IO_B = -1;
    

public:
    String ID = "-NULL-";
    LEDFlash *pLED;
    bool Reset_at_Next_Call = false;
    void Initialize(byte IO_Buzzer)
    {
        mfrc522.PCD_Init();
        byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
        IO_B = IO_Buzzer;
        if (IO_B != -1)
        {
            pinMode(IO_B, OUTPUT);
            digitalWrite(IO_B, LOW);
        }
        // When 0x00 or 0xFF is returned, communication probably failed
        if ((v == 0x00) || (v == 0xFF))
        {
            if (ReConnectCount == 0)
            {
                Debug.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
            }
            isRFIDConnect = false;
            ID = "-ERROR";
        }
        else
        {
            Debug.print(F("[RFID] MFRC522 Software Version: 0x"));
            Debug.print(String(v, HEX));
            if (v == 0x91)
                Debug.println(F(" = v1.0"));
            else if (v == 0x92)
                Debug.println(F(" = v2.0"));
            else
                Debug.println(F(" (unknown)"));
            isRFIDConnect = true;
            ReConnectCount = 0;
            ID = (ID == "-ERROR") ? "-NULL-" : ID;
        }
    }
    bool Update()
    {
        if (!isRFIDConnect)
        {
            ReConnectCount++;
            if (ReConnectCount == 100)
            {
                Initialize(IO_B);
                ReConnectCount = 0;
            }
            return true;
        }
        int t1 = millis();
        bool isNewCard = mfrc522.PICC_IsNewCardPresent();
        if(millis()-t1 > 200)
        {
            Serial.println("RFID Time Out");
            //mfrc522.PCD_Reset();
            Reset();
            return false;
        }

        if(!isNewCard)
            return true;

        if (mfrc522.PICC_ReadCardSerial())
        {
            // Now a card is selected. The UID and SAK is in mfrc522.uid.
            // Dump UID
            Serial.print(F("Card UID:"));
            String OldID = ID;
            for (byte i = 0; i < mfrc522.uid.size; i++)
            {
                ID.setCharAt(i * 2, "0123456789ABCDEF"[mfrc522.uid.uidByte[i] / 16]);
                ID.setCharAt(i * 2 + 1, "0123456789ABCDEF"[mfrc522.uid.uidByte[i] % 16]);
            }
            Serial.println(ID);
            if (IO_B != -1 && ID != OldID)
            {
                pLED->Set(0, pLED->C, 2, 5, 3);
                digitalWrite(IO_B, HIGH);
                delay(50);
                digitalWrite(IO_B, LOW);
                delay(80);
                digitalWrite(IO_B, HIGH);
                delay(50);
                digitalWrite(IO_B, LOW);
            }
            LastDetect = millis();
        }
        return true;
    }
    void Reset()
    {
        if (isRFIDConnect)
        {
            if (RFID_RST != -1)
            {
                digitalWrite(RFID_RST, LOW);
                delay(100);
            }
            mfrc522.PCD_Init();
        }
    }
    void PowerOff()
    {
        digitalWrite(RFID_RST, LOW);
        delay(100);
    }
};

#endif