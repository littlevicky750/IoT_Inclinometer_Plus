/**
 * @file MaxMin.h
 * @author VickyHung
 * @brief Calculate Maximum Value, minimum value, and difference of a set of float.
 * @version 0.1
 * @date 2023-07-31
 * 
 * @copyright Wonder Construct (c) 2023
 * 
 */
#ifndef MaxMin_H
#define MaxMin_H
#include <Arduino.h>

class MaxMin
{
public:
    float MaxVal = 0; // Maximum value
    float minVal = 0; // minimum value
    float Diff = 0;   // Difference
    
    /**
     * @brief Add float array to calculation
     * 
     * @param pArray Pointer to the float array
     * @param Length Number of float in the array
     */
    void Add(float *pArray, uint8_t Length)
    {
        if (MaxVal == 0)
            MaxVal = *pArray;
        if (minVal == 0)
            minVal = *pArray;
        for (int i = 0; i < Length; i++)
        {
            if (*(pArray + i) != 0)
            {
                MaxVal = (*(pArray + i) > MaxVal) ? *(pArray + i) : MaxVal;
                minVal = (*(pArray + i) < minVal) ? *(pArray + i) : minVal;
            }
        }
        Diff = MaxVal - minVal;
    }

    void Clear()
    {
        MaxVal = 0;
        minVal = 0;
        Diff = 0;
    }
};

#endif