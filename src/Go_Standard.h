#ifndef GoStandard_H
#define GoStandard_H
#include <Arduino.h>

class Go_Standard
{
private:
    int Standard_min = 1;
    int Standard_Max = 5;

public:
    int Standard = Standard_min;
    int flag = Standard_min;
    int ShowLine = 3;
    void Add(int A)
    {
        Standard += A;
        Standard = min(max(Standard, Standard_min),Standard_Max);
        if(Standard < flag)
            flag = Standard;
        else if(Standard > flag + ShowLine - 1)
            flag = Standard - ShowLine + 1;
    }
};

#endif