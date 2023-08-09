# 1 "C:\\Users\\littl\\AppData\\Local\\Temp\\tmp6it1cvn3"
#include <Arduino.h>
# 1 "C:/Users/littl/HKUST/CHANG Ching Wei - Wonder Construct Team/Arduino codes/DigitalSpiritLevel_V5.0/src/src.ino"


#include <Arduino.h>
#include "SerialDebug.h"

SerialDebug D;
extern SerialDebug Debug = D;

const gpio_num_t Pin_Button_Wakeup = GPIO_NUM_0;
const byte Pin_SwichEN1 = 1;
const byte Pin_SwichEN2 = 10;
const byte Pin_Button0 = Pin_Button_Wakeup;
const byte Pin_Button1 = 7;
const byte Pin_Button2 = 6;
const byte Pin_I2C_SDA = 8;
const byte Pin_I2C_SCL = 9;
const byte Pin_SD_MOSI = 12;
const byte Pin_SD_MISO = 13;
const byte Pin_SD_SCK = 14;
const byte Pin_SD_CS = 15;
const byte IO_RFID_RST = 16;
const byte Pin_Battery = 17;
const byte IO_Buzzer = 18;
#define IO_OLED_RST 19
const byte Pin_IMU_ISR = 20;
const byte Pin_IMU_RX = 38;
const byte Pin_IMU_TX = 39;
const byte Pin_LED_1_B = 47;
const byte Pin_LED_1_G = 4;
const byte Pin_LED_1_R = 48;
const byte Pin_LED_0_B = 40;
const byte Pin_LED_0_G = 41;
const byte Pin_LED_0_R = 42;
# 61 "C:/Users/littl/HKUST/CHANG Ching Wei - Wonder Construct Team/Arduino codes/DigitalSpiritLevel_V5.0/src/src.ino"
#define RFID_RST IO_RFID_RST
#define TestVersion false
#include "LEDFlash.h"
LEDFlash LED;

#include "SDCard.h"
#include "RealTimeClock.h"
RealTimeClock Clock;


#include "BLE.h"
#include "OLED.h"
#include "LongPressSwich.h"
#include "IMU42688.h"
#include "Battery.h"
#include "RFID.h"
#include "AngleCalculate.h"
#include "Go_Standard.h"

TaskHandle_t *T_FAST;
TaskHandle_t *T_LOOP;
TaskHandle_t *T_MAIN;
TaskHandle_t *T_SLOW;
TaskHandle_t *T_SAVE;

IMU42688 imu;
OLED oled;
LongPressSwich Swich;
SDCard sdCard;

Battery Bat;
RFID rfid;
AngleCalculate Calculate;
Go_Standard Standard;
BLE ble;

int MainLoopDelay = 250;
int FastLoopDelay = 125;
int LastEdit = 0;
bool fHaveSD = false;
bool fSave = false;
bool doRFID = true;
static void FAST(void *pvParameter);
static void MAIN(void *pvParameter);
static void SLOW(void *pvParameter);
static void Save(void *pvParameter);
void ButtonChange0();
void ButtonChange1();
void ButtonChange2();
void ButtonUpdate();
void setup();
void loop();
#line 104 "C:/Users/littl/HKUST/CHANG Ching Wei - Wonder Construct Team/Arduino codes/DigitalSpiritLevel_V5.0/src/src.ino"
static void FAST(void *pvParameter)
{
  TickType_t xLastWakeTime;
  BaseType_t xWasDelayed;
  xLastWakeTime = xTaskGetTickCount();
  for (;;)
  {
    xWasDelayed = xTaskDelayUntil(&xLastWakeTime, FastLoopDelay);
    if (!xWasDelayed && millis() > 10000)
      Debug.println("[Warning] Task Fast Time Out.");
    if (imu.Update() == imu.IMU_Update_Success)
    {
      Calculate.Input();
      if (imu.CalibrateCheck == 1)
        imu.Calibrate();
    }
    ButtonUpdate();
    oled.Update();
  }
}

static void MAIN(void *pvParameter)
{
  TickType_t xLastWakeTime;
  BaseType_t xWasDelayed;
  xLastWakeTime = xTaskGetTickCount();
  byte TimeOutCount = 0;
  for (;;)
  {
    xWasDelayed = xTaskDelayUntil(&xLastWakeTime, MainLoopDelay);
    if (!xWasDelayed && millis() > 10000)
    {
      TimeOutCount++;
      if (fHaveSD || TimeOutCount > 10)
      {
        Debug.println("[Warning] Main Loop Time Out.");
        TimeOutCount = 0;
      }
    }
    else
      TimeOutCount = 0;
    Swich.Off_Clock_Check();
    Clock.update();
    LED.Update();
    if (doRFID)
      rfid.Update();
  }
}

static void SLOW(void *pvParameter)
{
  for (;;)
  {
    Bat.Update();
    if (Bat.Percent < 10)
      LED.Set(0, LED.R, 1, 2);
    else
      LED.Set(0, 0, 0, 2);
    vTaskDelay(10000);
  }
}

static void Save(void *pvParameter)
{
  sdCard.SetPin(Pin_SD_SCK, Pin_SD_MISO, Pin_SD_MOSI, Pin_SD_CS);
  ble.Initialize(LastEdit);
  sdCard.Swich(true);
  TickType_t xLastWakeTime;
  BaseType_t xWasDelayed;
  xLastWakeTime = xTaskGetTickCount();
  bool fHaveSD_pre = fHaveSD;
  for (;;)
  {
    xWasDelayed = xTaskDelayUntil(&xLastWakeTime, 3000);
    if (!xWasDelayed)
      Debug.println("[Warning] Task Save Time Out.");


    String Msg = "";
    byte isSDSave = sdCard.Err_SD_Off;
    if (fSave && Calculate.State == Calculate.Done)
    {

      ble.Send(&Calculate.ResultAngle[0]);

      Msg += Clock.DateTimeStamp() + ",";
      Msg += rfid.ID + ",";
      Msg += String(Calculate.ResultAngle[0], 2) + ",";
      Msg += String(Calculate.ResultAngle[1], 2) + ",";
      Msg += String(Calculate.ResultAngle[2], 2) + ",";
      Msg += String(Standard.Standard) + ",";
      Msg += String(imu.Gravity) + ",";
      Msg += String(imu.SensorTemperature) + "\n";
      isSDSave = sdCard.Save("/" + Clock.DateStamp("", 4), Msg);
      if ((isSDSave == sdCard.SDOK))
        LED.Set(1, LED.G, 2, 5, 3);
      fSave = false;
      if (ble.Status.isConnect == true || isSDSave == sdCard.SDOK)
        Calculate.Switch(false);
    }
    else
    {
      isSDSave = sdCard.Save("", Msg);
    }
    ble.DoSwich();
    fHaveSD = (isSDSave == sdCard.SDOK || isSDSave == sdCard.Err_File_Write_Failed);
    if (fHaveSD != fHaveSD_pre)
    {
      if (fHaveSD)
      {
        String Info = sdCard.Read("/Setting.txt");
        if (Info.indexOf("Full_Cal_Password=123456789") != -1)
        {
          imu.ExpertMode = true;
          if (!TestVersion)
            Debug.Setup(sdCard);
          String SystemInfo = "";
          SystemInfo += "========================Expert_Mode_On========================\n";
          SystemInfo += "Last Upload Time = " + String(__DATE__) + String(__TIME__) + "\n";
          SystemInfo += "Mac Address = " + ble.AddrStr + "\n";
          SystemInfo += imu.Cal_Info_From_Flash;
          SystemInfo += "==============================================================";
          Debug.printOnTop(SystemInfo);
        }
        imu.SetUnit(Info);
      }
      else
      {
        imu.ExpertMode = false;
        imu.Cursor = 0;
        imu.CursorStart = 0;
      }
      fHaveSD_pre = fHaveSD;
    }
    LED.Set(1, LED.Y, (fHaveSD) ? 0 : ((isSDSave == sdCard.Err_SD_Off) ? 30 : 3), 3);
  }
}

bool ButPress[3] = {false};

void ButtonChange0()
{
  if (digitalRead(Pin_Button0))
  {
    Swich.Off_Clock_Stop();
    LED.Set(0, 0, 0, 5);
    ButPress[0] = true;
  }
  else
  {
    Swich.Off_Clock_Start();
  }
}

void ButtonChange1()
{
  ButPress[1] = true;
}

void ButtonChange2()
{
  ButPress[2] = true;
}

void ButtonUpdate()
{
  if (millis() - LastEdit < FastLoopDelay * 2 || millis() < oled.BlockTime)
    memset(ButPress, false, sizeof(ButPress));
  if (ButPress[0] || ButPress[1] || ButPress[2])
    LastEdit = millis();
  else if (oled.Page != 0 && millis() - LastEdit > 10000 && !TestVersion && oled.Page != 8 && oled.Page != 6)
    oled.Page = 0;

  bool ButtonUp = (imu.GravityPrevious == 4) ? ButPress[2] : ButPress[1];
  bool ButtonDown = (imu.GravityPrevious == 4) ? ButPress[1] : ButPress[2];
  bool ButtonAdd, ButtonMin, ButtonOn, ButtonOff;
  if (imu.Gravity % 3 == 0)
  {
    ButtonAdd = (imu.Gravity == 0) ? ButPress[2] : ButPress[1];
    ButtonMin = (imu.Gravity == 0) ? ButPress[1] : ButPress[2];
    ButtonOn = ButtonAdd;
    ButtonOff = ButtonMin;
  }
  else
  {
    ButtonAdd = ButtonDown;
    ButtonMin = ButtonUp;
    ButtonOn = ButtonUp;
    ButtonOff = ButtonDown;
  }
  switch (oled.Page)
  {
  case 0:
    if (ButPress[0])
    {
      if (Calculate.State == Calculate.Not_Stable || Calculate.State == Calculate.Done || fSave)
      {
        Calculate.Switch(0);
        fSave = false;
      }
      else
      {
        oled.Page = 1;
        oled.MenuCursor = 0;
      }
    }
    else if (ButPress[1] && imu.fWarmUp == 100)
    {
      Calculate.Switch((Calculate.State != Calculate.Not_Stable) && (Calculate.State != Calculate.Done));
      fSave = false;

    }
    else if (ButPress[2])
    {
      sdCard.Swich(1);
      if (imu.fWarmUp == 100)
      {
        if (Calculate.State == Calculate.Not_Stable)
        {
          Calculate.Switch(0);
          fSave = false;
        }
        else if (fSave)
        {
          Calculate.Switch(0);
          fSave = false;
        }
        else
        {
          fSave = true;
          if (Calculate.State != Calculate.Done)
            Calculate.Switch(1);
        }
      }
    }
    break;
  case 1:
    if (ButPress[0])
    {
      oled.Page = (oled.MenuCursor == 0) ? 0 : oled.MenuCursor + 1;
      oled.Cursor = 0;
    }
    else
    {
      if (ButtonAdd)
        oled.MenuCursor++;
      if (ButtonMin)
        oled.MenuCursor += (imu.fWarmUp == 100) ? 7 : 6;
      oled.MenuCursor %= (imu.fWarmUp == 100) ? 8 : 7;
    }
    break;
  case 2:
    if (ButtonUp)
      Standard.Add(-1);
    if (ButtonDown)
      Standard.Add(1);
    if (ButPress[0])
      oled.Page = 0;
    break;
  case 3:
    if (ButtonOff)
      ble.Status.OnOff = false;
    if (ButtonOn)
      ble.Status.OnOff = true;
    if (ButPress[0])
      oled.Page = 0;
    break;
  case 4:
    if (ButtonUp && sdCard.Cursor > 0)
      sdCard.Cursor--;
    if (ButtonDown && sdCard.Cursor < sdCard.Show.endsWith(".csv") + 1)
      sdCard.Cursor++;
    if (ButPress[0] && sdCard.Cursor == 0)
      oled.Page = 0;
    if (ButPress[0] && sdCard.Cursor == 1)
      sdCard.Swich();
    if (ButPress[0] && sdCard.Cursor == 2)
      oled.EasyBlock(sdCard.Show);
    break;
  case 5:
    if (Clock.Cursor == -1)
    {
      if (ButPress[0])
        oled.Page = 0;
      else if (ButtonDown)
        Clock.ResetUserSetTimeBuffer();
      else if (ButtonUp)
      {
        Clock.ResetUserSetTimeBuffer();
        Clock.Cursor = 3;
      }
    }
    else
    {
      if (ButPress[0])
        Clock.UserSetTime(1);
      if (ButtonUp)
        Clock.UserSetTime(0);
      if (ButtonDown)
        Clock.UserSetTime(2);
      if (millis() - LastEdit > 3000)
      {
        Clock.Cursor = 6;
        Clock.UserSetTime(1);
        Clock.Cursor = -1;
        LED.Set(0, LED.M, 2, 4, 3);
      }
    }
    break;
  case 6:
    if (ButPress[0] || ButPress[1] || ButPress[2])
      oled.Page = 0;
    break;
  case 7:
    if (ButtonOn)
      Swich.SleepTimeAdjust(true);
    if (ButtonOff)
      Swich.SleepTimeAdjust(false);
    if (ButPress[0])
      oled.Page = 0;
    break;
  case 8:
    if (imu.CalibrateCheck == -1 && imu.Cursor == 0 && ButPress[0])
      oled.Page = 1;
    else
    {
      if (ButPress[0])
        imu.CalibrateSelect(0);
      if (ButtonUp)
        imu.CalibrateSelect(1);
      if (ButtonDown)
        imu.CalibrateSelect(2);
      if (imu.CalibrateCheck == 0 && ButtonMin)
        imu.CalibrateSelect(3);
      if (imu.CalibrateCheck == 0 && ButtonAdd)
        imu.CalibrateSelect(4);
    }
    if (imu.CalibrateCheck == 2)
    {
      oled.Block((imu.Cursor == 2) ? "Calibration Data Clear" : "Calibrate Complete");
      imu.CalStop();
      oled.Page = 0;
    }
    break;
  default:
    oled.Page = 0;
  }
  memset(ButPress, false, sizeof(ButPress));
}

void setup()
{
  LED.SetPin(0, Pin_LED_0_R, Pin_LED_0_G, Pin_LED_0_B);
  Bat.SetPin(Pin_Battery);
  Swich.On(Pin_Button_Wakeup, Pin_SwichEN1, Pin_SwichEN2, LED, Bat, oled);
  Serial.begin(115200);

  pinMode(IO_RFID_RST, OUTPUT);
  pinMode(IO_RFID_RST, HIGH);
  Wire.begin();
  Wire.setClock(400000);
  if (TestVersion)
    Debug.Setup(sdCard);

  Clock.Initialize(MainLoopDelay);
  if (Clock.RtcLostPower)
    LED.Set(1, LED.R, 1, 2);
  Debug.SetRTC(&Clock);
  Debug.printOnTop("-------------------------ESP_Start-------------------------");
  Debug.println("[Battery] Battery " + String(Bat.Percent) + " %");
  Swich.pSD = &sdCard;
  Swich.LastTriggure = &LastEdit;
  rfid.Initialize(IO_Buzzer);
  imu.Initialize(Pin_IMU_RX, Pin_IMU_TX);
  LED.SetPin(1, Pin_LED_1_R, Pin_LED_1_G, Pin_LED_1_B);
  oled.Initialize();
  Calculate.pLED = &LED;
  Calculate.InputAngle = imu.AngleCal;
  Calculate.G = &imu.Gravity;
  rfid.pLED = &LED;
  imu.pLED = &LED;
  imu.fWarmUpTime = &LastEdit;
  oled.ClockShow = &Clock;
  oled.Bat = &Bat.Percent;
  oled.ID = &rfid.ID;
  oled.imu = &imu;
  oled.Measure = &Calculate;
  oled.SDState = &fHaveSD;
  oled.fSave = &fSave;
  oled.pBLEState = &ble.Status.Address[0];
  oled.Standard = &Standard;
  oled.pSD = &sdCard;
  oled.SleepTime = &Swich.SleepTimeSelect;
  ble.pRTC = &Clock;
  ble.pLED = &LED;


  while (!digitalRead(Pin_Button0))
    delay(1);
  LED.Set(0, 0, 0, 4);
  LED.Update();
  xTaskCreatePinnedToCore(FAST, "Core 1 Fast Loop", 10000, NULL, 3, T_FAST, 1);
  xTaskCreatePinnedToCore(MAIN, "Core 1 Main Loop", 16384, NULL, 2, T_MAIN, 1);
  xTaskCreatePinnedToCore(SLOW, "Core 0 Slow Loop", 10000, NULL, 1, T_SLOW, 0);
  xTaskCreatePinnedToCore(Save, "Core 0 Save Data", 16384, NULL, 2, T_SAVE, 0);
  delay(100);
  attachInterrupt(digitalPinToInterrupt(Pin_Button2), ButtonChange2, FALLING);
  attachInterrupt(digitalPinToInterrupt(Pin_Button1), ButtonChange1, FALLING);
  attachInterrupt(digitalPinToInterrupt(Pin_Button0), ButtonChange0, CHANGE);
  Serial.println("Setup Done");
}

void loop()
{

}