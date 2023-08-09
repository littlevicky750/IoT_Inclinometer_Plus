#include "BLE.h"

void MyServerCallbacks::onConnect(BLEServer *pServer)
{
    Status->isConnect = true;
    *LastEdit = millis();
    pLED->Set(0,pLED->B,29,1);
    Serial.println("Connect");
}

void MyServerCallbacks::onDisconnect(BLEServer *pServer)
{
    Status->isConnect = false;
    *LastEdit = millis();
    if(Status->OnOff == true)
    {
        BLEDevice::startAdvertising();
        pLED->Set(0,pLED->B,5,1);
    }
    Serial.println("Disconnect");
}

void SetTimesCallBacks::onWrite(BLECharacteristic *pCharacteristic)
{
    const uint8_t *T = pCharacteristic->getValue().data();
    uint32_t Time = *(T + 3) << 24 | *(T + 2) << 16 | *(T + 1) << 8 | *(T);
    pRTC->SetTime(Time);
}

void BLE::Initialize(int &LastEdit)
{
    // Start BLE Deviec ----------------------------------------
    BLEDevice::init("WoW Construct 數位水平尺");
    memcpy(Status.Address, BLEDevice::getAddress().getNative(), sizeof(Status.Address));
    AddrStr = BLEDevice::getAddress().toString().c_str();

    // Create Server --------------------------------------------
    pServer = BLEDevice::createServer();

    // Create Address -------------------------------------------
    BLEUUID ServiceUUID("0000abcd-00f1-0123-4567-0123456789ab");
    BLEUUID AngleXUUID("0000a001-00f1-0123-4567-0123456789ab");
    BLEUUID AngleYUUID("0000a002-00f1-0123-4567-0123456789ab");
    BLEUUID AngleZUUID("0000a003-00f1-0123-4567-0123456789ab");
    BLEUUID SetClkUUID("0000c000-00f1-0123-4567-0123456789ab");

    // Create Service -------------------------------------------
    BLEService *pService = pServer->createService(ServiceUUID);
    // Create Characteristic ------------------------------------
    AngleXChar = pService->createCharacteristic(AngleXUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    AngleYChar = pService->createCharacteristic(AngleYUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    AngleZChar = pService->createCharacteristic(AngleZUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    SetClkChar = pService->createCharacteristic(SetClkUUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::INDICATE);

    // Add Descriptor (Avoid using UUID 2902 (Already used by NimBLE))
    // Add 2901 Descriptor
    AngleXChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 8)->setValue("Angle X");
    AngleYChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 8)->setValue("Angle Y");
    AngleZChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 8)->setValue("Angle Z");
    SetClkChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 32)->setValue("Set Time (Unix epoch)");

    // Set input type and unit ------------------------------------------
    BLE2904 *p2904Ang = new BLE2904();
    BLE2904 *p2904Dis = new BLE2904();
    BLE2904 *p2904Tim = new BLE2904();

    p2904Ang->setFormat(BLE2904::FORMAT_FLOAT32); // Format float
    p2904Dis->setFormat(BLE2904::FORMAT_FLOAT32); // Format float
    p2904Tim->setFormat(BLE2904::FORMAT_UINT32);  // Format int

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

    // Set Initial Value
    AngleXChar->setValue(0.0F);
    AngleYChar->setValue(0.0F);
    AngleZChar->setValue(0.0F);
    SetClkChar->setValue(pRTC->NowSec());

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
    ServerCB.Status = &Status;
    ServerCB.LastEdit = &LastEdit;
    ServerCB.pLED = pLED;
    SetTimeCB.pRTC = pRTC;
    SetClkChar->setCallbacks(&SetTimeCB);
    pServer->setCallbacks(&ServerCB);
    pServer->advertiseOnDisconnect(false);
    // Start the Service ---------------------------------------------
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(ServiceUUID);
    pAdvertising->setScanResponse(true);
    BLEDevice::startAdvertising();
    pLED->Set(0,pLED->B,5,1);
}

void BLE::Send(float *SendFloat)
{
    AngleXChar->setValue(*SendFloat);
    AngleYChar->setValue(*(SendFloat + 1));
    AngleZChar->setValue(*(SendFloat + 2));
    AngleXChar->notify(true);
    AngleYChar->notify(true);
    AngleZChar->notify(true);
}

void BLE::DoSwich()
{
    // Do nothing if OnOff status remain the same.
    if (Pre_OnOff == Status.OnOff)
        return;
    // If swich from off to on;
    if (Status.OnOff == true)
    {
        if(! BLEDevice::getAdvertising()->isAdvertising())
            BLEDevice::startAdvertising();
        pLED->Set(0,pLED->B,5,1);
    }
    // If swich from on to off;
    else
    {
        if (Status.isConnect)
        {
            pServer->disconnect(1);
            Status.isConnect = false;
        }
        if(BLEDevice::getAdvertising()->isAdvertising())
            BLEDevice::stopAdvertising();
        pLED->Set(0,0,0,1);
    }
    Pre_OnOff = Status.OnOff;
};