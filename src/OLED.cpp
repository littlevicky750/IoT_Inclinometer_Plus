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
    flag.BlockTime = millis() + 3000;
}

void OLED::TurnOff()
{
    flag.BlockTime = millis() + 1000000;
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
    flag.BlockTime = millis() + time;
}

void OLED::DoBlock()
{
    if (BlockInfo == "")
    {
        flag.BlockTime = millis();
        return;
    }
    else
    {
        display.clearBuffer();
        int Rotation = (pIMU->Gravity % 3 == 2) ? pIMU->GravityPrevious : pIMU->Gravity;
        switch (Rotation)
        {
        case 0:
            display.setDisplayRotation(U8G2_R3);
            break;
        case 3:
            display.setDisplayRotation(U8G2_R1);
            break;
        case 4:
            display.setDisplayRotation(U8G2_R0);
            break;
        default:
            display.setDisplayRotation(U8G2_R2);
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

/**
 * @brief Rotate OLED display
 *
 * @param isMain True when drawing Main Screen, False when drawing Sub Screen.
 * @return true Display direction is rotate.
 * @return false Display direction not rotate.
 */
bool OLED::DoR(bool isMain)
{
    int R_new;
    if (isMain)
    {
        if (pIMU->Gravity % 3 == 2 || flag.Page == 2 || (flag.Page == 4 && pIMU->CalibrateCheck == -1))
            R_new = pIMU->GravityPrevious;
        else
            R_new = pIMU->Gravity;
    }
    else
        R_new = (pIMU->Gravity % 3 == 2) ? 6 - pIMU->Gravity : pIMU->Gravity;
    isH = R_new % 3;
    if (R_now != R_new)
    {
        switch (R_new)
        {
        case 0:
            display.setDisplayRotation(U8G2_R3);
            break;
        case 1:
            display.setDisplayRotation(U8G2_R2);
            break;
        case 3:
            display.setDisplayRotation(U8G2_R1);
            break;
        case 4:
            display.setDisplayRotation(U8G2_R0);
            break;
        }
        R_now = R_new;
        return true;
    }
    return false;
}

void OLED::Update()
{
    if (BlockInfo != "")
    {
        DoBlock();
        BlockInfo = "";
    }
    if (millis() < flag.BlockTime)
        return;

    FlashCount++;
    // Get display direction
    DoR(true);
    display.clearBuffer();

    switch (flag.Page)
    {
    case 1:
        Menu();
        break;
    case 2:
        Flatness();
        break;
    case 3:
        Bluetooth();
        break;
    case 4:
        if (pIMU->CalibrateCheck == -1)
            Cal_Menu();
        else
            Cal_Check();
        break;
    case 20:
        Show_Sensor_Address();
        break;
    case 21:
        One_Sensor_Info();
        break;
    case 22:
        Seven_Sensor_Info();
        break;
    default:
        Main();
        DrawArrow();
        flag.Page = 0;
        break;
    }

    digitalWrite(CS1, LOW);
    display.sendBuffer();
    digitalWrite(CS1, HIGH);

    // Small OLED ====================================================================================
    if (DoR(false) || flag.Page != 0)
    {
        display.clearBuffer();
        Main();
    }
    display.setDrawColor(0);
    display.drawBox(0, 0, isH ? 5 : 64, isH ? 64 : 5);
    display.drawBox(isH ? 123 : 0, isH ? 0 : 123, isH ? 5 : 64, isH ? 64 : 5);
    display.setDrawColor(1);
    switch (pIMU->Gravity % 3)
    {
    case 0:
        if (abs(pIMU->AngleCalShow[2]) > 90)
            display.drawXBM(0, 0, 64, 5, Main_Up);
        else
            display.drawXBM(0, 123, 64, 5, Main_Down);
        break;
    case 1:
        if (pIMU->AngleCalShow[2] > 0)
            display.drawXBM(123, 0, 5, 64, Main_Right);
        else
            display.drawXBM(0, 0, 5, 64, Main_Left);
        break;
    }
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

uint8_t OLED::BLEShow(bool Animation)
{
    if (*(pBLEState + 7) == false)
        return 0;
    if (*(pBLEState + 6) == true && !Animation)
        return 4;
    else if (Flash(5, 20))
        return 1;
    else if (Flash(10, 20))
        return 2;
    else if (Flash(15, 20))
        return 3;
    else
        return 4;
}

bool OLED::Flash(int Due, int Period)
{
    return (FlashCount % Period < Due);
}

void OLED::DrawFloat_9x15(int x, int y, String f)
{
    if (f.charAt(0) == '-')
        display.drawBox(x, y + 6, 6, 2);
    int p = 7;
    char c;
    for (int i = 1; i < 8; i++)
    {
        c = f.charAt(i);
        if (c == '.')
            display.drawBox(x + p, y + 12, 2, 2);
        else if (c == '-')
            display.drawBox(x + p + 3, y + 6, 6, 2);
        else if (c <= '9' && c >= '0')
            display.drawXBM(x + p, y, 9, 15, Num_9x15_allArray[c - '0']);
        p += (c == '.') ? 4 : 11;
    }
}

void OLED::DrawFloat_12x20(int x, int y, String f)
{
    if (f.charAt(0) == '-')
        display.drawBox(x, y + 10, 9, 3);
    int p = 10;
    char c;
    for (int i = 1; i < 8; i++)
    {
        c = f.charAt(i);
        if (c == '.')
            display.drawBox(x + p, y + 17, 3, 3);
        else if (c == '-')
            display.drawBox(x + p + 2, y + 10, 9, 3);
        else if (c <= '9' && c >= '0')
            display.drawXBM(x + p, y, 12, 20, Num_12x20_allArray[c - '0']);
        p += (c == '.') ? 5 : 14;
    }
}

void OLED::DrawArrow()
{
    if (pIMU->Gravity % 3 == 0)
    {
        int G = (pIMU->Gravity / 3 == 1) ? -90 : 90;
        int length = abs(tan((pIMU->AngleCalShow[2] - G) * PI / 180)) * 1000 + 1;
        length = (length > 13) ? 13 : length;
        bool d = (pIMU->AngleCalShow[2] < G);
        for (int i = 0; i < length; i++)
        {
            display.drawXBM(0 + 5 * i, 0 * d + 123 * !d, 3, 5, Arrow[3]);
            display.drawXBM(61 - 5 * i, 0 * !d + 123 * d, 3, 5, Arrow[2]);
        }
    }
    else if (pIMU->Gravity % 3 == 1)
    {
        float tangent = tan((pIMU->AngleCalShow[2]) * PI / 180) * 1000;
        int length = (abs(tangent) > 12) ? 13 : abs(tangent) + 1;
        bool d = (tangent > 0);
        for (int i = 0; i < length; i++)
        {
            display.drawXBM(0 * d + 123 * !d, 0 + 5 * i, 5, 3, Arrow[1]);
            display.drawXBM(0 * !d + 123 * d, 61 - 5 * i, 5, 3, Arrow[0]);
        }
    }
}

void OLED::YesNo(bool IsH, bool Select)
{
    char S1[4] = "No";
    char S2[4] = "Yes";
    display.setFont(u8g2_font_7x14B_tr);
    int w;
    if (IsH)
    {
        display.drawFrame(41, 26, 46, 18);
        display.drawFrame(41, 46, 46, 18);
        display.drawStr(57, 40, S1);
        display.drawStr(54, 60, S2);
        display.setDrawColor(2);
        display.drawBox(43, 28 + 20 * Select, 42, 14);
        display.setDrawColor(1);
    }
    else
    {
        display.drawFrame(0, 110, 30, 18);
        display.drawFrame(33, 110, 30, 18);
        display.drawStr(8, 124, S1);
        display.drawStr(38, 124, S2);
        display.setDrawColor(2);
        display.drawBox(2 + 33 * Select, 112, 26, 14);
        display.setDrawColor(1);
    }
}

// Each Page =================================================================

void OLED::Main()
{
    int px[6];
    int py[6];
    int l = (isH) ? 111 : 62;
    float Angle = (pMeasure->State == pMeasure->Done) ? pMeasure->Result[9] : pIMU->AngleCalShow[2];
    char c[8];
    dtostrf((pDS->Mm.Diff < 0.3) ? 0 : pDS->Mm.Diff, 7, 1, c);
    if (isH)
    {
        display.drawXBM(0, 0, 128, 64, Main_H);
        int x[6] = {18, 32, 10, 8, 16, 104};
        int y[6] = {9, 44, 3, 33, 4, 2};
        memcpy(px, x, sizeof(px));
        memcpy(py, y, sizeof(py));
        // Draw Max Displacement
        if (pDS->SetZeros || pDS->Mm.Diff >= 100 || pDS->Mm.MaxVal >= 100)
            display.drawXBM(56, py[0] + 9, 45, 3, Main_Dash_45x3);
        else
            DrawFloat_12x20(px[0], py[0], c);
        // Draw Angle
        if (pIMU->Gravity % 3 == 2)
            display.drawXBM(px[1] + 1, py[1] + 9, 81, 3, Main_Dash_81x3);
        else
        {
            if (pIMU->unit == 0)
                DrawFloat_12x20(px[1], py[1], pIMU->String_now_unit(Angle));
            else if (pIMU->unit == 1)
                DrawFloat_12x20(px[0], py[1], pIMU->String_now_unit(Angle));
        }
        // Draw Unit
        if (pIMU->unit == 0)
            display.drawXBM(115, 39, 5, 5, Main_Degree_S);
        else if (pIMU->unit == 1)
            display.drawXBM(104, 48, 19, 16, Main_mmm_S);
    }
    else
    {
        display.drawXBM(0, 0, 64, 128, Main_V);
        int x[6] = {0, 0, 2, 1, 12, 47};
        int y[6] = {59, 93, 9, 38, 4, 9};
        memcpy(px, x, sizeof(px));
        memcpy(py, y, sizeof(py));
        // Draw Max Displacement
        if (pDS->SetZeros || pDS->Mm.Diff >= 100 || pDS->Mm.MaxVal >= 100)
            display.drawXBM(px[0], py[0] + 8, 64, 2, Main_Dash_64x2);
        else
            DrawFloat_9x15(px[0], py[0], c);
        // Draw Angle
        if (pIMU->Gravity % 3 == 2)
            display.drawXBM(px[1], py[1] + 8, 64, 2, Main_Dash_64x2);
        else
            DrawFloat_9x15(px[1], py[1], pIMU->String_now_unit(Angle));
        // Draw Unit
        if (pIMU->unit == 0)
            display.drawXBM(23, 110, 41, 11, Main_Degree_L);
        else if (pIMU->unit == 1)
            display.drawXBM(31, 110, 33, 11, Main_mmm_L);
    }
    Timer++;

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
        display.drawBox((Timer > (l - px[4])) ? px[3] + (l - px[4]) * 2 - Timer : px[3] + Timer, py[3], px[4], py[4]);
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
        display.drawBox((Timer > (l - px[4])) ? px[3] + (l - px[4]) * 2 - Timer : px[3] + Timer, py[3], px[4], py[4]);
    }
    else if (pMeasure->State == pMeasure->Measuring)
    {
        Timer %= (l - px[4]) * 2;
        display.drawXBM(px[2], py[2], 24, 24, HG_allArray[Timer / 10 % 2]);
        display.drawBox(px[3], py[3], pMeasure->MeasurePercent * l, py[4]);
    }
    else if (*(pBLEState + 8))
    {
        Timer %= (l - px[4]) * 2;
        display.drawXBM(px[2], py[2], 24, 24, BLE_allArray[BLEShow(true)]);
        display.drawBox((Timer > (l - px[4])) ? px[3] + (l - px[4]) * 2 - Timer : px[3] + Timer, py[3], px[4], py[4]);
    }
    else if (pMeasure->State == pMeasure->Done)
    {
        Timer = 0;
        display.drawXBM(px[2], py[2], 24, 24, Done);
        display.drawBox(px[3], py[3], l, py[4]);
    }
    display.setDrawColor(2);
    display.drawBox(px[2], py[2], 24, 24);
    display.setDrawColor(0);
    display.drawTriangle(px[3] + l - 1, py[3], px[3] + l + 3, py[3], px[3] + l + 3, py[3] + py[4]);
    display.setDrawColor(1);
    // Draw Battery
    display.drawBox(px[5], py[5], *pBatt * 13 / 100, 5);
}

void OLED::Menu()
{
    int dx[5] = {0, 0, 0, 1, 3};
    int dy[5] = {3, 0, 0, 0, 1};
    int x = dx[R_now];
    int y = dy[R_now];
    display.drawXBM(x, y, isH ? 128 : 64, isH ? 64 : 128, Menu_allArray[isH]);
    int nx, ny;
    for (int i = 0; i < 8; i++)
    {
        nx = 31 * ((isH) ? i / 2 : i % 2);
        ny = 31 * ((isH) ? i % 2 : i / 2);
        switch (i)
        {
        case 1: // Connect
            display.drawXBM(x + nx + 4, y + ny + 4, 24, 24, Connect_Icon(pDS->SetZeros));
            break;
        case 2: // BLE
            display.drawXBM(x + nx + 4, y + ny + 4, 24, 24, BLE_allArray[BLEShow(false)]);
            break;
        case 4: // Clock
        {
            int num[2] = {pRTC->now.minute(), pRTC->now.twelveHour() * 5 + pRTC->now.minute() / 12};
            int l[2] = {8, 5};
            for (int j = 0; j < 2; j++)
            {
                int px = x + nx + 15 + (num[j] > 30);
                int py = y + ny + 16 - (num[j] + 16) / 30 % 2;
                display.drawLine(px, py, px + l[j] * sin(num[j] / 30.0 * PI) + 0.5, py - (l[j] + 1) * cos(num[j] / 30.0 * PI) + 0.5);
            }
            break;
        }
        case 6: // Battery
            display.drawBox(x + nx + 6, y + ny + 12, *pBatt * 17 / 100, 8);
            break;
        }
    }
    display.setDrawColor(2);
    nx = 2 + 31 * ((isH) ? flag.Cursor / 2 : flag.Cursor % 2);
    ny = 2 + 31 * ((isH) ? flag.Cursor % 2 : flag.Cursor / 2);
    display.drawXBM(x + nx, y + ny, 28, 28, Menu_Select);
    display.setDrawColor(1);
}

void OLED::Flatness()
{
    display.drawXBM(0, 0, 128, 64, Open_Page2);
    display.drawXBM(111, 50, 13, 12, Lock_allArray[pDS->Enable_Auto_Reset]);
    if (pDS->ConnectCount != 7)
    {
        display.drawBox(119, 0, 2, 5);
        display.drawBox(119, 7, 2, 2);
        display.drawXBM(123, 0, 7, 10, HEX_5x9_allArray[pDS->ConnectCount]);
    }
    display.setDrawColor(2);
    display.drawBox(0, flag.Cursor * 16 + 16, 128, 16);
}

void OLED::Bluetooth()
{
    if (isH)
    {
        int x = (R_now == 1) ? 9 : 26;
        display.drawXBM(33 + x, 13, 24, 24, BLE_allArray[BLEShow(false)]);
        for (int i = 0; i < 6; i++)
        {
            display.drawXBM(16 * i + x, 43, 5, 9, HEX_5x9_allArray[*(pBLEState + 5 - i) / 0x10]);
            display.drawXBM(16 * i + x + 6, 43, 5, 9, HEX_5x9_allArray[*(pBLEState + 5 - i) % 0x10]);
            if (i != 5)
                display.drawPixel(16 * i + x + 13, 51);
        }
        display.drawXBM((R_now == 1) ? 111 : 0, 0, 17, 64, Switch_allArray[*(pBLEState + 7) + 2]);
    }
    else
    {
        display.drawXBM(19, 33, 24, 24, BLE_allArray[BLEShow(false)]);
        for (int i = 0; i < 6; i++)
        {
            display.drawXBM(11 * i, 63, 4, 7, HEX_4x7_allArray[*(pBLEState + 5 - i) / 0x10]);
            display.drawXBM(11 * i + 5, 63, 4, 7, HEX_4x7_allArray[*(pBLEState + 5 - i) % 0x10]);
            if (i != 5)
                display.drawPixel(11 * i + 9, 70);
        }
        display.drawXBM(0, 104, 64, 24, Switch_allArray[*(pBLEState + 7)]);
    }
}

void OLED::Cal_Menu()
{
    display.setFont(u8g2_font_7x14B_tr);
    display.drawStr(0, 11, "Calibration");
    display.drawBox(0, 13, 128, 2);
    int w;
    char Cal_Mode[5][16] = {"Back", "Calibrate", "Clear", "Exp Cal", "Z Dir Exp Cal"};
    for (int i = 0; i < 3; i++)
    {
        w = display.getStrWidth(Cal_Mode[i + pIMU->CursorStart]);
        display.drawStr(64 - w / 2, 29 + i * 16, Cal_Mode[i + pIMU->CursorStart]);
    }
    display.setDrawColor(2);
    display.drawBox(0, 17 + 16 * (pIMU->Cursor - pIMU->CursorStart), 128, 14);
    display.setDrawColor(1);
}

void OLED::Cal_Check()
{
    display.setFont(u8g2_font_7x14B_tr);
    if (pIMU->CalibrateCheck == 0)
    {
        String Question;
        bool ShowYesNo = true;
        int line3 = 1;
        if (pIMU->Cursor == 1)
            Question = "Ready to Calibrate?";
        else if (pIMU->Cursor == 2)
            Question = "Sure to Clear Data?";
        else if (pIMU->Cursor == 3)
        {
            line3 = 0;
            Question = "g = " + String(pIMU->Gravity);
            ShowYesNo = false;
            if (pIMU->Gravity < 3)
            {
                if (pIMU->FullCalComplete[pIMU->Gravity])
                    Question += ", Complete.";
                else
                {
                    Question += ", Ready?";
                    ShowYesNo = true;
                }
            }
        }
        else if (pIMU->Cursor == 4)
        {
            line3 = 0;
            Question = "g = " + String(pIMU->Gravity);
            ShowYesNo = false;
            if (pIMU->Gravity % 3 == 0)
            {
                if (pIMU->FullCalComplete[pIMU->Gravity])
                    Question += ", Complete.";
                else
                {
                    Question += ", Ready?";
                    Question += "(" + String(pIMU->CalibrateCollectCount[pIMU->Gravity] + 1) + ")";
                    line3 = (pIMU->CalibrateCollectCount[pIMU->Gravity] > 8) ? 4 : 3;
                    ShowYesNo = true;
                }
            }
        }

        if (R_now % 3 != 0)
        {
            int w = 0;
            for (int i = 0; i < Question.length(); i++)
            {
                display.drawGlyph(w, 13, Question.charAt(i));
                w += (Question[i] == ' ') ? 5 : 7;
            }
        }
        else
        {
            int w = 0;
            int l1 = Question.substring(7).indexOf(" ") + 8;
            display.drawStr(0, 15, Question.substring(0, l1 - 1).c_str());
            for (int i = 0; i < Question.substring(l1).length() - line3; i++)
            {
                char a = Question.substring(l1).charAt(i);
                w -= (a == 'l') ? 2 : 0;
                display.drawGlyph(w, 35, a);
                w += (a == ' ' || a == 'l') ? 5 : 7;
            }
            if (pIMU->Cursor != 3)
                display.drawStr(0, 55, Question.substring(Question.length() - line3).c_str());
        }
        if (!ShowYesNo)
        {
            display.drawStr((display.getDisplayWidth() - display.getStrWidth("Back")) / 2, display.getDisplayHeight() - 4, "Back");
            display.drawFrame(display.getDisplayWidth() / 2 - 23, display.getDisplayHeight() - 18, 46, 18);
            display.setDrawColor(2);
            display.drawBox(display.getDisplayWidth() / 2 - 21, display.getDisplayHeight() - 16, 42, 14);
            display.setDrawColor(1);
        }
        else
            YesNo((R_now % 3 != 0), pIMU->YesNo);
    }
    else if (pIMU->CalibrateCheck == 1 && pIMU->Cursor == 2)
    {
        Block("Calibration Data Clear", 3000);
    }
    else if (pIMU->CalibrateCheck == 1)
    {
        for (int i = 1; i < 3; i++)
        {
            display.drawGlyph(0, 17 * i - 7, 120 + ((pIMU->Gravity + i) % 3));
            display.drawGlyph(7, 17 * i - 8, '=');
            char A[8];
            dtostrf(pIMU->Angle[(pIMU->Gravity + i) % 3], 7, 3, A);
            display.drawStr(15, 17 * i - 7, A);
        }
        display.drawGlyph(0, 44, 'T');
        display.drawGlyph(7, 43, '=');
        display.drawStr(22, 44, String(pIMU->SensorTemperature, 1).c_str());
        if (R_now % 3 != 0)
        {
            display.drawFrame(12, 50, 104, 14);
            display.drawBox(14, 52, pIMU->CalibrateCount * 100 / pIMU->CalAvgNum, 10);
        }
        else
        {
            display.drawFrame(5, 100, 54, 14);
            display.drawBox(7, 102, pIMU->CalibrateCount * 50 / pIMU->CalAvgNum, 10);
        }
    }
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
            break;
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

void OLED::Show_Sensor_Address()
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