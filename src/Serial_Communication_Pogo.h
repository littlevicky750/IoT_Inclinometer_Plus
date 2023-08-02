#ifndef Serial_Communication_Pogo_H
#define Serial_Communication_Pogo_H
#include <Arduino.h>
#include "esp_mac.h"
#include <HardwareSerial.h>
#include "Serial_Communication_Package.h"
#include "DistanceSensor.h"

extern HardwareSerial Serial2;

class Serial_Communication_Pogo
{
private:
    byte IO_TX[2] = {0};
    byte IO_RX[2] = {0};
    int Failed_Count = 0;
    bool initialize = false;
    Message_Package Zero_Pack;

public:
    Message_Package T_Pack;
    Message_Package R_Pack;
    DistanceSensor *pDS;
    float *pAngle;
    uint8_t Connect;
    const uint8_t No_Connect = 0x00;
    const uint8_t Connect_1 = 0x01;
    const uint8_t Connect_2 = 0x02;
    const uint8_t Bad_Connect = 0xee;
    void ReadMac()
    {
        esp_read_mac(T_Pack.Address, ESP_MAC_BT);
        /*
        Serial.print("Address = ");
        for (int i = 0; i < 6; i++)
            Serial.print(T_Pack.Address[i], HEX);
        Serial.println();
        */
    }

    void Setup(byte Pin_TX, byte Pin_RX, bool ConnectType /*Parallel = 0, Serial = 1*/)
    {
        pinMode(Pin_TX, OUTPUT);
        pinMode(Pin_RX, OUTPUT);
        digitalWrite(Pin_TX, HIGH);
        digitalWrite(Pin_RX, LOW);
        pinMode(Pin_RX, INPUT_PULLDOWN);
        IO_TX[ConnectType] = Pin_TX;
        IO_RX[ConnectType] = Pin_RX;
        if (!initialize)
        {
            ReadMac();
            Serial2.setRxBufferSize(256);
            Serial2.begin(9600, SERIAL_8N1, 45, 46);
            initialize = true;
        }
    }
    void Update()
    {
        //*
        if (T_Pack.Connect_Port == 0)
        {
            if (digitalRead(IO_RX[0]) == 1)
            {
                T_Pack.Connect_Port = Connect_1;
                Serial2.setPins(IO_RX[0], IO_TX[0], -1, -1);
                Serial2.flush();
            }
            else if (digitalRead(IO_RX[1]) == 1)
            {
                T_Pack.Connect_Port = Connect_2;
                Serial2.setPins(IO_RX[1], IO_TX[1], -1, -1);
                Serial2.flush();
            }
        }
        else
        {
            Failed_Count++;
            // Set Value
            memcpy(T_Pack.Distance,&(pDS->Distance[0]),sizeof(T_Pack.Distance));
            T_Pack.Angle = *pAngle;
            T_Pack.isSettingZero = pDS->SetZeros;
            memcpy(T_Pack.haveSensor, pDS->isConnect,sizeof(pDS->isConnect));
            // Send Value
            Serial2.write((uint8_t *)&T_Pack, sizeof(T_Pack));
            // Receive Value
            while (Serial2.available())
            {
                int k = Serial2.read((uint8_t *)&R_Pack, sizeof(R_Pack));
                if (k != sizeof(R_Pack))
                {
                }
                else if (R_Pack.Package_Strat != 0xef)
                {
                    Serial.print("Packet Start Error : 0x");
                    Serial.println(R_Pack.Package_Strat, HEX);
                }
                else if (R_Pack.Package_End != 0xab)
                {
                    Serial.print("Packet End Error : 0x");
                    Serial.println(R_Pack.Package_End, HEX);
                }
                else
                {
                    if (Connect != No_Connect)
                    {
                        Serial.println(">>>>>>>>>>>>>>>>>>>");
                        if (Connect == Bad_Connect)
                            Serial.println("Bad Connect");
                        else
                            Serial.println(Connect);
                        for (int i = 0; i < 6; i++)
                            Serial.print(R_Pack.Address[i], HEX);
                        Serial.println();
                        Serial.println("<<<<<<<<<<<<<<<<<<<");
                    }
                    Failed_Count = 0;
                }
            }
            if (Failed_Count == 0 && Connect == No_Connect)
            {
                if (T_Pack.Connect_Port != R_Pack.Connect_Port)
                    Connect = Bad_Connect;
                else if (T_Pack.Connect_Port == Connect_1)
                    Connect = Connect_1;
                else if (T_Pack.Connect_Port == Connect_2)
                {
                    Connect = Connect_2;
                    pDS->Reset(true, false);
                }
                T_Pack.isSenseConnect = true;
                Serial.println("Serial2 Connect");
            }
            if ((Connect == No_Connect && Failed_Count == 5) || (Connect != No_Connect && Failed_Count == 2))
            {
                Serial2.setPins(45, 46, -1, -1);
                pinMode(IO_TX[T_Pack.Connect_Port - 1], OUTPUT);
                pinMode(IO_RX[T_Pack.Connect_Port - 1], OUTPUT);
                digitalWrite(IO_TX[T_Pack.Connect_Port - 1], HIGH);
                digitalWrite(IO_RX[T_Pack.Connect_Port - 1], LOW);
                pinMode(IO_RX[T_Pack.Connect_Port - 1], INPUT_PULLDOWN);
                if (Connect != No_Connect)
                {
                    Serial.println("Serial2 Disconnect");
                    pDS->Reset(false, false);
                }
                Connect = No_Connect;
                Failed_Count = 0;
                T_Pack = Zero_Pack;
                R_Pack = Zero_Pack;
            }
        }
    }
};

#endif