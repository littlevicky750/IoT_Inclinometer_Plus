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

void Button::Update()
{
  switch (Page)
  {
  case 0:
    if (Press[0])
    {
      if (pMeasure->State == pMeasure->Not_Stable || pMeasure->State == pMeasure->Done)
      {
        pMeasure->Switch(0);
      }
      else
      {
        //oled.Page = 1;
        //oled.MenuCursor = 0;
      }
    }
    else if (Press[1] && !pDS->SetZeros)
    {
      pMeasure->Switch((pMeasure->State != pMeasure->Not_Stable && pMeasure->State != pMeasure->Done));
    }
    else if (Press[2])
    {
    }
    break;
    break;

  default:
    break;
  }
  memset(Press, false, sizeof(Press));
}
