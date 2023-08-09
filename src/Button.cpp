/**
 * @file Button.cpp
 * @author Vicky Hung (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-08-01
 *
 * @copyright Wonder Construct (c) 2023
 *
 * @note
 *
 *
 * Main OLED and Button distribution :
 *                 ________________
 *   SLED 0 -> o  |                | O  <- Button 1
 *   SLED 1 -> o  |    SSD 1306    |
 * Button 0 -> O  |________________| O  <- Button 2
 *
 * Side OLED and Button distribution :
 *  _________    ___
 * | CH 1115 |  /   \ <- Long Button ( Button 3 ) with Buttton LED
 * |_________|  \___/
 */

#include "Button.h"

bool Button::CanMeasure()
{
  if (pIMU->fWarmUp != 100)
    return false;
  else if (pMeasure->State == pMeasure->Not_Stable)
    return false;
  else if (pMeasure->State == pMeasure->Measuring)
    return false;
  else if (pDS->SetZeros)
    return false;
  else if (pIMU->CalibrateCheck != -1)
    return false;
  return true;
}

void Button::Update()
{
  if (!Press[0] && !Press[1] && !Press[2] && !Press[3])
    return;
  else if (millis() - LastPress < 500) // Avoide attach interrupt double triggure.
    return;
  else
    LastPress = millis();
  if (millis() < pOLED->BlockTime)
    return;

  if (Press[3] && CanMeasure())
  {
    pMeasure->Switch(true);
    *(pBLEState + 8) = *(pBLEState + 6);
  }

  bool ButtonUp = (pIMU->GravityPrevious == 4) ? Press[2] : Press[1];
  bool ButtonDown = (pIMU->GravityPrevious == 4) ? Press[1] : Press[2];
  bool ButtonAdd, ButtonMin, ButtonOn, ButtonOff;
  if (pIMU->Gravity % 3 == 0)
  {
    ButtonAdd = (pIMU->Gravity == 0) ? Press[2] : Press[1];
    ButtonMin = (pIMU->Gravity == 0) ? Press[1] : Press[2];
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

  switch (pOLED->Page)
  {
  case 0:
    if (Press[0])
    {
      pMeasure->Switch(false);
      *(pBLEState + 8) = false;
      pOLED->Page = 1;
      pOLED->Cursor = 0;
    }
    if (Press[2])
    {
      if (!*(pBLEState + 6))
      {
        Press[1] = true;
      }
      else if (*(pBLEState + 8))
      {
        pMeasure->Switch(false);
        *(pBLEState + 8) = false;
      }
      else if (CanMeasure())
      {
        if (pMeasure->State != pMeasure->Done)
          pMeasure->Switch(true);
        *(pBLEState + 8) = true;
      }
    }
    if (Press[1])
    {
      pMeasure->Switch(CanMeasure() && pMeasure->State != pMeasure->Done);
      *(pBLEState + 8) = false;
    }
    break;
  case 1:
    if (Press[0])
    {
      pOLED->Page = (pOLED->Cursor == 0) ? 0 : pOLED->Cursor + 1;
      pOLED->Cursor = 0;
    }
    else if (ButtonAdd)
      pOLED->Cursor++;
    else if (ButtonMin)
      pOLED->Cursor += 7;
    pOLED->Cursor %= 8;
    break;
  case 2:
    if (Press[0])
    {
      switch (pOLED->Cursor)
      {
      case 1:
        pDS->Reset(true, true);
        pOLED->Page = 0;
        break;
      case 2:
        pDS->Enable_Auto_Reset = !pDS->Enable_Auto_Reset;
        break;
      case 3:
        pOLED->Page = 20;
        pOLED->Cursor = 0;
        break;
      default:
        pOLED->Page = 0;
        break;
      }
    }
    if (ButtonDown && pOLED->Cursor < 2 + *(pBLEState + 9))
      pOLED->Cursor++;
    if (ButtonUp && pOLED->Cursor > 0)
      pOLED->Cursor--;
    break;
  case 3:
    if (ButtonOff)
      *(pBLEState + 7) = false;
    if (ButtonOn)
      *(pBLEState + 7) = true;
    if (Press[0])
      pOLED->Page = 0;
    break;
  case 4:
    if (pIMU->CalibrateCheck == -1 && pIMU->Cursor == 0 && Press[0])
      pOLED->Page = 1;
    else
    {
      if (Press[0])
        pIMU->CalibrateSelect(0);
      if (ButtonUp)
        pIMU->CalibrateSelect(1);
      if (ButtonDown)
        pIMU->CalibrateSelect(2);
      if (pIMU->CalibrateCheck == 0 && ButtonMin)
        pIMU->CalibrateSelect(3);
      if (pIMU->CalibrateCheck == 0 && ButtonAdd)
        pIMU->CalibrateSelect(4);
    }
    break;
  case 5:
  case 6:
    if (Press[0] || Press[1] || Press[2])
      pOLED->Page = 0;
    break;
  case 7:
    if (ButtonOn)
      pSleepTime->SleepTimeAdjust(true);
    if (ButtonOff)
      pSleepTime->SleepTimeAdjust(false);
    if (Press[0])
      pOLED->Page = 0;
    break;
  case 20:
    if (Press[0])
      pOLED->Page = 0;
    else if (Press[1])
      pOLED->Page = 21;
    else if (Press[2])
      pOLED->Page = 22;
    break;
  case 21:
    if (Press[0])
      pOLED->Page = 0;
    else if (Press[1])
      pOLED->Page = 22;
    else if (Press[2])
      pOLED->Page = 20;
    break;
  case 22:
    if (Press[0])
      pOLED->Page = 0;
    else if (Press[1])
      pOLED->Page = 20;
    else if (Press[2])
      pOLED->Page = 21;
    break;
  }
  memset(Press, false, sizeof(Press));
}
