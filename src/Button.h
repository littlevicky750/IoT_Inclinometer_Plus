/**
 * @file Button.h
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
#ifndef Button_H
#define Button_H

#include "DistanceSensor.h"
#include "Measure.h"

class Button
{
private:
public:
    uint8_t Page = 0;
    uint8_t Cursor = 0;
    bool Press[4];
    void Update();
    DistanceSensor *pDS;
    Measure *pMeasure;
};
#endif

