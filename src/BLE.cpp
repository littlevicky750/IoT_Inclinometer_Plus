#include "BLE.h"

void MyServerCallbacks::onConnect(BLEServer *pServer)
{
    State->isConnect = true;
    *LastEdit = millis();
    pLED->Set(0, pLED->B, 29, 1);
    Serial.println("Connect");
}

void MyServerCallbacks::onDisconnect(BLEServer *pServer)
{
    State->isConnect = false;
    *LastEdit = millis();
    if (State->OnOff == true)
    {
        BLEDevice::startAdvertising();
        pLED->Set(0, pLED->B, 5, 1);
    }
    Serial.println("Disconnect");
}

void SetTimesCallBacks::onWrite(BLECharacteristic *pCharacteristic)
{
    const uint8_t *T = pCharacteristic->getValue().data();
    uint32_t Time = *(T + 3) << 24 | *(T + 2) << 16 | *(T + 1) << 8 | *(T);
    pRTC->SetTime(Time);
}

void SetUnitCallBacks::onWrite(BLECharacteristic *pCharacteristic)
{
    const char *S = pCharacteristic->getValue().c_str();
    int l = pCharacteristic->getValue().length();
    char C[16];
    memcpy(C, S, sizeof(C[0]) * l);
    Serial.println(C);
    pIMU->SetUnit(C);
}

void InputKeyCallBacks::onWrite(BLECharacteristic *pCharacteristic)
{
    const char *S = pCharacteristic->getValue().c_str();
    State->ExpertMode = (strcmp(S,"123456789") == 0);
}

void BLE::Initialize(int &LastEdit)
{
    // Start BLE Deviec ----------------------------------------
    BLEDevice::init("WoW Construct 數位平整尺");
    memcpy(State.Address, BLEDevice::getAddress().getNative(), sizeof(State.Address));
    AddrStr = BLEDevice::getAddress().toString().c_str();

    // Create Server --------------------------------------------
    pServer = BLEDevice::createServer();

    // Create Address -------------------------------------------
    BLEUUID ServiceUUID("0000abcd-00f1-0123-4567-0123456789ab");
    BLEUUID AngleXUUID("0000a001-00f1-0123-4567-0123456789ab");
    BLEUUID AngleYUUID("0000a002-00f1-0123-4567-0123456789ab");
    BLEUUID AngleZUUID("0000a003-00f1-0123-4567-0123456789ab");
    BLEUUID SetUniUUID("0000ad00-00f1-0123-4567-0123456789ab");
    BLEUUID SetClkUUID("0000c000-00f1-0123-4567-0123456789ab");
    BLEUUID SetKeyUUID("0000eeee-00f1-0123-4567-0123456789ab");

    // Create Service -------------------------------------------
    BLEService *pService = pServer->createService(ServiceUUID);
    // Create Characteristic ------------------------------------
    AngleXChar = pService->createCharacteristic(AngleXUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    AngleYChar = pService->createCharacteristic(AngleYUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    AngleZChar = pService->createCharacteristic(AngleZUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    SetClkChar = pService->createCharacteristic(SetClkUUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::INDICATE);
    SetUniChar = pService->createCharacteristic(SetUniUUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::INDICATE);
    SetKeyChar = pService->createCharacteristic(SetKeyUUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::INDICATE);

    // Add Descriptor (Avoid using UUID 2902 (Already used by NimBLE))
    // Add 2901 Descriptor
    AngleXChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 8)->setValue("Angle X");
    AngleYChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 8)->setValue("Angle Y");
    AngleZChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 8)->setValue("Angle Z");
    SetClkChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 32)->setValue("Set Time (Unix epoch)");
    SetUniChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 32)->setValue("Set Angle Display Unit");
    SetUniChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 32)->setValue("Input expert mode password.");

    // Set input type and unit ------------------------------------------
    BLE2904 *p2904Ang = new BLE2904();
    BLE2904 *p2904Dis = new BLE2904();
    BLE2904 *p2904Tim = new BLE2904();
    BLE2904 *p2904UTF = new BLE2904();

    p2904Ang->setFormat(BLE2904::FORMAT_FLOAT32); // Format float
    p2904Dis->setFormat(BLE2904::FORMAT_FLOAT32); // Format float
    p2904Tim->setFormat(BLE2904::FORMAT_UINT32);  // Format int
    p2904UTF->setFormat(BLE2904::FORMAT_UTF8);    // Format utf8

    p2904Ang->setUnit(0x2763); // plane angle (degree)
    p2904Dis->setUnit(0x2701); // length (meter)
    p2904Tim->setUnit(0x2703); // time (second)
    p2904Ang->setExponent(0);
    p2904Dis->setExponent(-3);
    p2904Tim->setExponent(0);

    AngleXChar->addDescriptor(p2904Ang);
    AngleYChar->addDescriptor(p2904Ang);
    AngleZChar->addDescriptor(p2904Ang);
    SetClkChar->addDescriptor(p2904Tim);
    SetUniChar->addDescriptor(p2904UTF);
    SetKeyChar->addDescriptor(p2904UTF);

    // Set Initial Value
    AngleXChar->setValue(0.0F);
    AngleYChar->setValue(0.0F);
    AngleZChar->setValue(0.0F);
    SetClkChar->setValue(pRTC->NowSec());
    SetUniChar->setValue("deg");

    // Add Displacement relative characteristic
    String DisplacementUUID = "0000d000-00f1-0123-4567-0123456789ab";
    for (int i = 0; i < 15; i++)
    {
        DisplacementUUID.setCharAt(7, "0123456789abcdef"[i]);
        BLEUUID DisUUID(DisplacementUUID.c_str());
        DisChar[i] = pService->createCharacteristic(DisUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
        if (i == 0)
            DisChar[i]->createDescriptor("2901", NIMBLE_PROPERTY::READ, 32)->setValue("Maximum Displacement");
        else
            DisChar[i]->createDescriptor("2901", NIMBLE_PROPERTY::READ, 16)->setValue("Distance " + String(i));
        DisChar[i]->addDescriptor(p2904Dis);
        DisChar[i]->setValue(0.0F);
    }

    // Set Characteristic Callback ------------------------------
    ServerCB.State = &State;
    ServerCB.LastEdit = &LastEdit;
    ServerCB.pLED = pLED;
    SetTimeCB.pRTC = pRTC;
    SetUnitCB.pIMU = pIMU;
    SetKeyCB.State = &State;
    SetClkChar->setCallbacks(&SetTimeCB);
    SetUniChar->setCallbacks(&SetUnitCB);
    SetKeyChar->setCallbacks(&SetKeyCB);
    pServer->setCallbacks(&ServerCB);
    pServer->advertiseOnDisconnect(false);
    // Start the Service ---------------------------------------------
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(ServiceUUID);
    pAdvertising->setScanResponse(true);
    BLEDevice::startAdvertising();
    pLED->Set(0, pLED->B, 5, 1);
}

void BLE::Send(float *SendFloat)
{
    AngleXChar->setValue(*SendFloat);
    AngleXChar->notify(true);
    AngleYChar->setValue(*(SendFloat + 1));
    AngleYChar->notify(true);
    AngleZChar->setValue(*(SendFloat + 2));
    AngleZChar->notify(true);
    for (int i = 0; i < 15; i++)
    {
        DisChar[i]->setValue(*(SendFloat + 3 + i));
        DisChar[i]->notify(true);
    }
    State.Send_Info = false;
}

void BLE::DoSwich()
{
    // Do nothing if OnOff status remain the same.
    if (Pre_OnOff == State.OnOff)
        return;
    // If swich from off to on;
    if (State.OnOff == true)
    {
        if (!BLEDevice::getAdvertising()->isAdvertising())
            BLEDevice::startAdvertising();
        pLED->Set(0, pLED->B, 5, 1);
    }
    // If swich from on to off;
    else
    {
        if (State.isConnect)
        {
            pServer->disconnect(1);
            State.isConnect = false;
        }
        if (BLEDevice::getAdvertising()->isAdvertising())
            BLEDevice::stopAdvertising();
        pLED->Set(0, 0, 0, 1);
    }
    Pre_OnOff = State.OnOff;
};