#include "OLED.h"
#include "OLED_Bitmap.h"
#include <U8g2lib.h>

U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI display(U8G2_R0, /*CLK*/ 36, /*SDA*/ 35, /*CS*/ 3, /*DC*/ 15, /*RST*/ 41);

/**
 * @brief Do oled powe up RST pin adjust proceduer.
 * @attention Will not power on VDD and AFh
 *
 */
void OLED::Do_RST()
{
    pinMode(IO_OLED_RST, OUTPUT);
    digitalWrite(IO_OLED_RST, LOW);
    delay(1);
    digitalWrite(IO_OLED_RST, HIGH);
    delay(1);
}

void OLED::TurnOn(byte VCC)
{
    IO_VCC = VCC;
    if (!isU8g2Begin)
    {
        pinMode(CS1, OUTPUT);
        pinMode(CS2, OUTPUT);
        digitalWrite(CS1, LOW);
        digitalWrite(CS2, LOW);
        Do_RST();
        pinMode(IO_VCC, OUTPUT);
        digitalWrite(IO_VCC, HIGH);
        delay(10);
        if (!display.begin())
        {
            Serial.println(F("SSD1306 allocation failed"));
            return;
        }
        isU8g2Begin = true;
    }
    display.clearBuffer();
    display.sendBuffer();
    digitalWrite(CS1, HIGH);
    digitalWrite(CS2, HIGH);
}

void OLED::InitDisplay()
{
    display.clearBuffer();
    display.setDisplayRotation(U8G2_R2);
    display.drawXBM(0, 0, 128, 64, Open_Logo);
    digitalWrite(CS1, LOW);
    digitalWrite(CS2, LOW);
    display.sendBuffer();
    digitalWrite(CS1, HIGH);
    digitalWrite(CS2, HIGH);
    BlockTime = millis() + 3000;
}

void OLED::TurnOff()
{
    BlockTime = millis() + 1000000;
    delay(200);
    display.clear();
    digitalWrite(CS1, LOW);
    digitalWrite(CS2, LOW);
    display.sendBuffer();
    // display.initDisplay();
    delay(1);
    display.setPowerSave(1);
    delay(1);
    digitalWrite(CS1, HIGH);
    digitalWrite(CS2, HIGH);
    if (IO_VCC != 0)
    {
        digitalWrite(IO_VCC, LOW);
        pinMode(IO_VCC, INPUT);
    }
}

void OLED::Block(String Info, int time)
{
    BlockInfo = Info;
    BlockTime = millis() + time;
}

void OLED::DoBlock()
{
    if (BlockInfo == "")
    {
        BlockTime = millis();
        return;
    }
    else
    {
        display.clearBuffer();
        int Rotation = (pIMU->Gravity % 3 == 2) ? pIMU->GravityPrevious : pIMU->Gravity;
        switch (Rotation)
        {
        case 0:
            display.setDisplayRotation(U8G2_R1);
            break;
        case 3:
            display.setDisplayRotation(U8G2_R3);
            break;
        case 4:
            display.setDisplayRotation(U8G2_R2);
            break;
        default:
            display.setDisplayRotation(U8G2_R0);
            break;
        }
        display.setFont(u8g2_font_helvB10_tr);
        const unsigned int Mx = display.getDisplayWidth();
        const unsigned int My = display.getDisplayHeight();
        const unsigned int a = display.getAscent() - display.getDescent();
        int s[10] = {0};
        int s_now = BlockInfo.indexOf(" ");
        int s_next, w;
        int i = 0;
        if (s_now == -1)
        {
            s_now = BlockInfo.length();
            i++;
            s[i] = s_now + 1;
        }
        while (s_now < BlockInfo.length())
        {
            w = display.getStrWidth(BlockInfo.substring(s[i], s_now).c_str());
            s_next = -1;
            while (w <= Mx)
            {
                s_now += s_next + 1;
                if (s_now > BlockInfo.length())
                    break;
                s_next = BlockInfo.substring(s_now + 1).indexOf(" ");
                if (s_next == -1)
                    s_next = BlockInfo.substring(s_now + 1).length();
                w = display.getStrWidth(BlockInfo.substring(s[i], s_now + s_next + 1).c_str());
            }
            i++;
            s[i] = s_now + 1;
            s_next = BlockInfo.substring(s[i]).indexOf(" ");
            if (s_next == -1 && s_now < BlockInfo.length())
            {
                i++;
                s[i] = BlockInfo.length() + 1;
                break;
            }
            s_now += s_next + 1;
        }
        unsigned int sh = a * 1.25;
        unsigned int h = sh * (i - 1) + a;
        while (h > My)
        {
            sh--;
            h = sh * (i - 1) + a;
        }
        unsigned int y = (My - h) / 2 + display.getAscent();
        for (int j = 0; j < i; j++)
        {
            w = display.getStrWidth(BlockInfo.substring(s[j], s[j + 1] - 1).c_str());
            display.drawStr((Mx - w) / 2, y, BlockInfo.substring(s[j], s[j + 1] - 1).c_str());
            y += sh;
        }
        digitalWrite(CS2, LOW);
        digitalWrite(CS1, LOW);
        display.sendBuffer();
        digitalWrite(CS1, HIGH);
        digitalWrite(CS2, HIGH);
    }
}

void OLED::Update()
{
    if (BlockInfo != "")
    {
        DoBlock();
        BlockInfo = "";
    }
    if (millis() < BlockTime)
        return;

    // Get display direction
    int Rotation = (pIMU->Gravity % 3 == 2) ? pIMU->GravityPrevious : pIMU->Gravity;
    switch (Rotation)
    {
    case 0:
        display.setDisplayRotation(U8G2_R1);
        break;
    case 3:
        display.setDisplayRotation(U8G2_R3);
        break;
    case 4:
        display.setDisplayRotation(U8G2_R2);
        break;
    default:
        display.setDisplayRotation(U8G2_R0);
        break;
    }
    isH = true;

    display.clearBuffer();

    switch (Page)
    {
    case 0:
        One_Sensor_Info();
        break;

    default:
        break;
    }

    digitalWrite(CS1, LOW);
    display.sendBuffer();
    digitalWrite(CS1, HIGH);

    // Small OLED ====================================================================================
    isH = false;
    display.clearBuffer();
    display.setDisplayRotation(U8G2_R1);
    Main();
    digitalWrite(CS2, LOW);
    display.sendBuffer();
    digitalWrite(CS2, HIGH);
}

const unsigned char *OLED::Connect_Icon(bool isSetting)
{
    if (pPOGO->Connect == pPOGO->No_Connect)
    {
        return Connect_all[isSetting];
    }
    else if (pPOGO->Connect == pPOGO->Connect_1)
    {
        return Connect_all[2 + isSetting];
    }
    else if (pPOGO->Connect == pPOGO->Connect_2)
    {
        return Connect_all[4 + isSetting];
    }
    else if (pPOGO->Connect == pPOGO->Bad_Connect)
    {
        return Connect_all[6 + isSetting];
    }
    return Connect_all[7];
}

void OLED::Main()
{
    int px[6];
    int py[6];
    int l = (isH) ? 126 : 62;
    if (isH)
    {
        display.drawXBM(0, 0, 128, 64, Main_H);
        int x[6] = {53, 41, 3, 1, 16, 111};
        int y[6] = {6, 42, 3, 33, 4, 2};
        memcpy(px, x, sizeof(px));
        memcpy(py, y, sizeof(py));
    }
    else
    {
        display.drawXBM(0, 0, 64, 128, Main_V);
        int x[6] = {12, 0, 2, 1, 12, 47};
        int y[6] = {55, 96, 10, 39, 4, 2};
        memcpy(px, x, sizeof(px));
        memcpy(py, y, sizeof(py));
    }
    Timer++;
    // Draw Max Displacement
    /*
    if (pDS->SetZeros)
        DrawUnsignedFloat4_1(px[0], py[0], 100);
    else if (pMeasure->State == pMeasure->Done)
        DrawUnsignedFloat4_1(px[0], py[0], pMeasure->Compute.Max_Displacement);
    else if (pDist->Max_Displacement < 0.35)
        DrawUnsignedFloat4_1(px[0], py[0], 0);
    else
        DrawUnsignedFloat4_1(px[0], py[0], pDist->Max_Displacement);
    */

    // Draw Levelness
    int cal[5] = {180, 90, 0, -90, -180};
    float Angle = (pMeasure->State == pMeasure->Done) ? pMeasure->Result[9] : pIMU->AngleCalShow[2];
    for (int i = 0; i < 6; i++)
    {
        if (i == 5)
        {
            //DrawFloat5_1(px[1], py[1], 100);
            break;
        }
        else if (abs(Angle - cal[i]) < 6)
        {
            // DrawFloat5_1(px[1], py[1], pimu->Angle - cal[i]);
            //DrawFloat5_1(px[1], py[1], tan((Angle - cal[i]) / 180 * PI) * 1000);
            break;
        }
    }

    // Draw Icon
    if (pDS->SetZeroProgress != 0 && pDS->SetZeroProgress != 1)
    {
        Timer = 0;
        display.drawXBM(px[2], py[2], 24, 24, Connect_Icon(true));
        display.drawBox(px[3], py[3], pDS->SetZeroProgress * l, py[4]);
    }
    else if (pDS->SetZeros)
    {
        Timer %= (l - px[4]) * 2;
        display.drawXBM(px[2], py[2], 24, 24, Connect_Icon(true));
        display.drawBox((Timer > (l - px[4])) ? 1 + (l - px[4]) * 2 - Timer : 1 + Timer, py[3], px[4], py[4]);
    }
    else if (pIMU->fWarmUp < 100)
    {
        Timer = 0;
        display.drawXBM(px[2], py[2], 24, 24, Fire);
        display.drawBox(px[3], py[3], pIMU->fWarmUp * l / 100, py[4]);
    }
    else if (pMeasure->State == pMeasure->Sleep)
    {
        Timer = 0;
        display.drawXBM(px[2], py[2], 24, 24, Connect_Icon(false));
    }
    else if (pMeasure->State == pMeasure->Not_Stable)
    {
        Timer %= (l - px[4]) * 2;
        display.drawXBM(px[2], py[2], 24, 24, HG_allArray[Timer / 10 % 2]);
        display.drawBox((Timer > (l - px[4])) ? 1 + (l - px[4]) * 2 - Timer : 1 + Timer, py[3], px[4], py[4]);
    }
    else if (pMeasure->State == pMeasure->Measuring)
    {
        Timer %= (l - px[4]) * 2;
        display.drawXBM(px[2], py[2], 24, 24, HG_allArray[Timer / 10 % 2]);
        display.drawBox(px[3], py[3], pMeasure->MeasurePercent * l, py[4]);
    }
    else if (pMeasure->State == pMeasure->Done)
    {
        Timer = 0;
        display.drawXBM(px[2], py[2], 24, 24, Done);
        display.drawBox(px[3], py[3], l, py[4]);
    }
    else
    {
        Timer %= (l - px[4]) * 2;
        display.drawXBM(px[2], py[2], 24, 24, SD_Saving);
        display.drawBox((Timer > (l - px[4])) ? 1 + (l - px[4]) * 2 - Timer : 1 + Timer, py[3], px[4], py[4]);
    }
    display.setDrawColor(2);
    display.drawBox(px[2], py[2], 24, 24);
    display.setDrawColor(0);
    display.drawTriangle(l - 2, py[3], l + 2, py[3], l + 2, py[3] + py[4]);
    display.setDrawColor(1);
    // Draw Battery
    display.drawBox(px[5], py[5], *pBatt * 13 / 100, 5);
}








// Page For QC ======================================================================================

void OLED::One_Sensor_Info()
{
    display.setFont(u8g2_font_7x13B_tr);
    display.drawStr(0, 12, "Wire:");
    display.drawStr(0, 25, "Addr:");
    display.drawStr(0, 38, "Raw:");
    display.drawStr(0, 51, "Val:");
    display.drawStr(0, 64, "Var:");
    bool haveConnect = false;
    for (int i = 0; i < 8; i++)
    {
        if (pDS->isConnect[i])
        {
            display.drawStr(49, 12, String(i / 4).c_str());
            display.drawStr(49, 25, "0x4");
            display.drawGlyph(70, 25, "89ab"[i % 4]);
            display.drawStr(49, 38, String(pDS->ADCSurgeOut[0][i]).c_str());
            display.drawStr(49, 51, String(pDS->ADCfilt[i]).c_str());
            int MaxVal = pDS->ADCSurgeOut[0][i];
            int MinVal = pDS->ADCSurgeOut[0][i];
            for (int j = 1; j < 20; j++)
            {

                MaxVal = (pDS->ADCSurgeOut[j][i] > MaxVal) ? pDS->ADCSurgeOut[j][i] : MaxVal;
                MinVal = (pDS->ADCSurgeOut[j][i] < MinVal) ? pDS->ADCSurgeOut[j][i] : MinVal;
            }
            display.drawStr(49, 64, String(MaxVal - MinVal).c_str());
        }
    }
}

void OLED::Seven_Sensor_Info()
{
    display.setFont(u8g2_font_5x8_tr);
    for (int i = 0; i < 7; i++)
    {
        display.drawStr(0, 9 + i * 9, String(pDS->ADCfilt[i], 0).c_str());
        display.drawStr(50, 9 + i * 9, String(pMeasure->Result[3 + i], 2).c_str());
        float MaxVal = pDS->ADCSurgeOut[0][i];
        float MinVal = pDS->ADCSurgeOut[0][i];
        for (int j = 1; j < 20; j++)
        {

            MaxVal = (pDS->ADCSurgeOut[j][i] > MaxVal) ? pDS->ADCSurgeOut[j][i] : MaxVal;
            MinVal = (pDS->ADCSurgeOut[j][i] < MinVal) ? pDS->ADCSurgeOut[j][i] : MinVal;
        }
        display.drawStr(100, 9 + i * 9, String(MaxVal - MinVal, 0).c_str());
    }
}

void OLED::ShowAddress()
{
    if (pPOGO->Connect == pPOGO->Connect_1)
        display.drawXBM(0, 0, 128, 64, Addr_CS);
    else if (pPOGO->Connect == pPOGO->Connect_2)
        display.drawXBM(0, 0, 128, 64, Addr_CP);
    else
        display.drawXBM(0, 0, 128, 64, Addr_NC);
    display.setDrawColor(2);
    for (int i = 0; i < 7; i++)
    {
        if (pDS->isConnect[i])
            display.drawBox(102 - 17 * i, 0, 14, 31);
        if (pPOGO->R_Pack.haveSensor[i])
            display.drawBox(102 - 17 * i, 34, 14, 31);
    }
}