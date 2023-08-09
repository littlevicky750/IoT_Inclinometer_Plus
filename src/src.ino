#include <Arduino.h>

const byte IO_EN = 18;
const byte IO_11V = 37;
const byte IO_Battery = 1;
const byte IO_Long_Button = 2;
const byte IO_Button0 = 6;
const byte IO_Button1 = 7;
const byte IO_Button2 = 0;
const byte IO_SDA1 = 8;
const byte IO_SCL1 = 9;
const byte IO_SDA2 = 5;
const byte IO_SCL2 = 4;
const byte IO_POGO_S_TX = 11;
const byte IO_POGO_S_RX = 12;
const byte IO_POGO_P_TX = 13;
const byte IO_POGO_P_RX = 14;
const byte IO_RFID_RST = 16;
const byte IO_SD_CTRL = 17; // ???????????????
const byte IO_SPI_MOSI = 19;
const byte IO_SPI_MISO = 20;
const byte IO_SPI_CLK = 47;
const byte IO_SD_CS = 48;
const byte IO_Button_LED = 21;
const byte IO_SPI2_DC = 15;  // MISO
const byte IO_SPI2_SDA = 35; // MOSI
const byte IO_SPI2_CLK = 36;
#define CS1 10;
#define CS2 42;
#define IO_OLED_RST 41
#define IO_LED 40;

const byte IO_IMU_RX = 38;
const byte IO_IMU_TX = 39;

#define ExpMode

#ifdef ExpMode
#define Debug_Println(a) Serial.println(a)
#define Debug_Print(b) Serial.print(a)
#else
#define Debug_Println(a)
#define Debug_Print(b)
#endif

#include "RealTimeClock.h"
#include "DistanceSensor.h"
#include "IMU42688.h"
#include "Battery.h"
#include "Measure.h"
#include "OLED.h"
#include "Serial_Communication_Pogo.h"
#include "SLED.h"
#include "OnOff.h"
#include "Button.h"
#include "BLE.h"

DistanceSensor DS;
IMU42688 imu;
Battery Bat;
Measure measure;
RealTimeClock Clock;
Serial_Communication_Pogo Pogo;
SLED led;
OnOff Swich;
Button But;
OLED oled;
BLE ble;

/**
 * @brief Button attachinterrupt callbeack
 * @attention ONLY PUT SIMPLE CALCULATION IN ATTACHINTERRUPT CALLBACK.
 * @attention PUTTING TIME CONSUMING OPERATION (such as Serial.print) IN ATTACHINTERRUPT CALLBACK WILL CAUSE SYSTEM CRASH.
 */

void ButtonPress0()
{
  if (!digitalRead(IO_Button0))
  { // Release
    Swich.Off_Clock_Stop();
    Swich.LastEdit = millis();
    led.Set(0, 0, 0, 4);
    But.Press[0] = true;
  }
  else
  { // Press
    Swich.Off_Clock_Start();
    Swich.LastEdit = millis();
  }
}

void ButtonPress1()
{
  But.Press[1] = true;
  Swich.LastEdit = millis();
}

void ButtonPress2()
{
  But.Press[2] = true;
  Swich.LastEdit = millis();
}

void ButtonPress3()
{
  But.Press[3] = true;
  Swich.LastEdit = millis();
}

/*****************************************************************************************************/
/**
 @brief FreeRTOS task code

 Core 0 : UI
  SPI : OLED (SSD1306, CH1115)
  Calculation : \b Button::Update()
 Core 1 :
  I2C : Wire 0 : 0x48 ~ 0x4b ads1115 (Distance sensor)
        Wire 1 : 0x48 ~ 0x4a ads1115 (Distance sensor) + 0x68 DS3231 (Real Time Clock)
  UART : Serial 1 : Esp32-c3 communication (IMU)
         Serial 2 : Esp32-s3 communication (POGO Pin Communication)
  BLE : Server.
  adc : Battery input.
  Other : LED (LLC2842)
  Calculation : \b Measure::Update()
*/
/*****************************************************************************************************/

TaskHandle_t *T_OLED;
TaskHandle_t *T_I2C0;
TaskHandle_t *T_I2C1;
TaskHandle_t *T_UART;
TaskHandle_t *T_SEND;
TaskHandle_t *T_LOOP;

int OLED_Period = 125;
int Data_Period = 200;
int LOOP_Period = 500;
int SEND_Period = 1000;

static void User_Interface(void *pvParameter)
{
  BaseType_t xWasDelayed;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  bool BLE_Status_Pre = false;
  for (;;)
  {
    xWasDelayed = xTaskDelayUntil(&xLastWakeTime, OLED_Period);
    if (BLE_Status_Pre != ble.State.isConnect)
    {
      BLE_Status_Pre = ble.State.isConnect;
      oled.Block(BLE_Status_Pre ? "Bluetooth Connect" : "Bluetooth Disconnect", 5000);
    }
    else if (!xWasDelayed && millis() > 10000)
      Serial.println("[Warning] Task OLED Time Out.");

    But.Update();
    oled.Update();
  }
}

static void I2C0(void *pvParameter)
{
  BaseType_t xWasDelayed;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  for (;;)
  {
    xWasDelayed = xTaskDelayUntil(&xLastWakeTime, Data_Period);
    if (!xWasDelayed && millis() > 10000)
      Serial.println("[Warning] Task I2C0 Time Out.");
    DS.Update(0);
    Clock.update();
    measure.DataIsUpdte(3, 4);
  }
}

static void I2C1(void *pvParameter)
{
  BaseType_t xWasDelayed;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  for (;;)
  {
    xWasDelayed = xTaskDelayUntil(&xLastWakeTime, Data_Period);
    if (!xWasDelayed && millis() > 10000)
      Serial.println("[Warning] Task I2C1 Time Out.");
    DS.Update(1);
    measure.DataIsUpdte(7, 3);
  }
}

static void UART(void *pvParameter)
{
  BaseType_t xWasDelayed;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  for (;;)
  {
    xWasDelayed = xTaskDelayUntil(&xLastWakeTime, Data_Period);
    if (!xWasDelayed && millis() > 10000)
      Serial.println("[Warning] Task UART Time Out.");
    if (imu.Update() == imu.IMU_Update_Success)
    {
      measure.DataIsUpdte(0, 3);
      if (imu.CalibrateCheck == 1)
        imu.Calibrate();
      if (imu.CalibrateCheck == 2)
      {
        oled.Block((imu.Cursor == 2) ? "Calibration Data Clear" : "Calibrate Complete", 3000);
        imu.CalStop();
      }
    }
    Pogo.Update();
    measure.DataIsUpdte(10, 7);
    measure.Update();
  }
}

static void LOOP(void *pvParameter)
{
  BaseType_t xWasDelayed;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  for (;;)
  {
    xWasDelayed = xTaskDelayUntil(&xLastWakeTime, LOOP_Period);
    if (!xWasDelayed && millis() > 10000)
      Serial.println("[Warning] Task LOOP Time Out.");
    led.Update();
    Swich.Off_Clock_Check();
    Bat.Update_BW();
    digitalWrite(IO_Button_LED, But.CanMeasure());
  }
}

static void SEND(void *pvParameter)
{
  BaseType_t xWasDelayed;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  ble.pRTC = &Clock;
  ble.pLED = &led;
  ble.pIMU = &imu;
  ble.Initialize(Swich.LastEdit);
  for (;;)
  {
    xWasDelayed = xTaskDelayUntil(&xLastWakeTime, SEND_Period);
    if (!xWasDelayed && millis() > 10000)
      Serial.println("[Warning] Task SEND Time Out.");
    if (ble.State.Send_Info && measure.State == measure.Done)
    {
      ble.Send(&measure.Result[0]);
      measure.Switch(false);
    }
    ble.DoSwich();
  }
}

void setup()
{
  // Power On
  Swich.On(IO_Button0, IO_EN, led, oled);
  // OLED display initialize
  oled.TurnOn(IO_11V);
  oled.InitDisplay();
  // I2C device setup
  DS.SetUp(IO_SDA1, IO_SCL1, IO_SDA2, IO_SCL2);
  DS.pReceive = &Pogo.R_Pack;
  Clock.Initialize(OLED_Period);
  // IMU setup
  imu.Initialize(IO_IMU_RX, IO_IMU_TX);
  imu.pLED = &led;
  imu.fWarmUpTime = &Swich.LastEdit;
  imu.ExpertMode = &ble.State.ExpertMode;
  // Pogo Pin Set Up
  Pogo.Setup(IO_POGO_S_TX, IO_POGO_S_RX, 0);
  Pogo.Setup(IO_POGO_P_TX, IO_POGO_P_RX, 1);
  Pogo.pDS = &DS;
  Pogo.pAngle = &imu.AngleCalShow[2];
  // Measurement pointer Setting
  measure.SetInput(imu.AngleCal, 0, 3);
  measure.SetStable(imu.AngleCal_ExG, 0, 3, 0.2, 0.5);
  measure.Set(DS.Distance, 3, 7, 0.5, 1);
  measure.Set(Pogo.R_Pack.Distance, 10, 7, 0.5, 1);
  // Battery Set Up
  Bat.SetPin(IO_Battery);
  Bat.Update();
  // Button Lifht Set Up
  pinMode(IO_Button_LED, OUTPUT);
  digitalWrite(IO_Button_LED, LOW);
  // UI pointer setting
  oled.pRTC = &Clock;
  oled.pDS = &DS;
  oled.pPOGO = &Pogo;
  oled.pMeasure = &measure;
  oled.pIMU = &imu;
  oled.pBatt = &Bat.Percent;
  oled.pBLEState = &ble.State.Address[0];
  oled.pRTC = &Clock;
  oled.pSleepSelect = &Swich.SleepTimeSelect;
  But.pDS = &DS;
  But.pMeasure = &measure;
  But.pIMU = &imu;
  But.pBLEState = &ble.State.Address[0];
  But.pOLED = &oled.flag;
  But.pSleepTime = &Swich;
  // Initialize Long Button LED
  pinMode(IO_Button_LED, OUTPUT);
  digitalWrite(IO_Button_LED, LOW);
  // Wait for button release
  while (digitalRead(IO_Button0))
  {
  }
  led.Set(0, 0, 0, 4);
  led.Update();
  oled.Update();
  // System Start
  xTaskCreatePinnedToCore(I2C0, "Core 1 I2C0", 16384, NULL, 4, T_I2C0, 1);
  xTaskCreatePinnedToCore(I2C1, "Core 1 I2C1", 16384, NULL, 4, T_I2C1, 1);
  xTaskCreatePinnedToCore(UART, "Core 1 UART", 16384, NULL, 4, T_UART, 1);
  xTaskCreatePinnedToCore(LOOP, "Core 1 LOOP", 8192, NULL, 2, T_LOOP, 1);
  xTaskCreatePinnedToCore(SEND, "Core 1 SEND", 8192, NULL, 3, T_SEND, 1);
  xTaskCreatePinnedToCore(User_Interface, "Core 0 Loop", 16384, NULL, 5, T_OLED, 0);
  attachInterrupt(digitalPinToInterrupt(IO_Button0), ButtonPress0, CHANGE);
  attachInterrupt(digitalPinToInterrupt(IO_Button1), ButtonPress1, FALLING);
  attachInterrupt(digitalPinToInterrupt(IO_Button2), ButtonPress2, FALLING);
  attachInterrupt(digitalPinToInterrupt(IO_Long_Button), ButtonPress3, RISING);
}

void loop()
{
}