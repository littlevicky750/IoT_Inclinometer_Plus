#include "SDCard.h"

#include <Arduino.h>
#include <FS.h>
#include "SerialDebug.h"

byte SDCard::Save(String FolderName, String &SaveString)
{
  if (!SwichMode)
  {
    Show = "Function OFF";
    CheckSaveString(SaveString);
    return Err_SD_Off;
  }
  LastCheck = millis();
  byte SD_Check = CheckState();

  if (SD_Check != SDOK)
  {
    if (SaveString == "")
    {
      Show = "No Card Insert";
      CheckSaveString(SaveString);
    }
    return SD_Check;
  }

  if (SaveString == "" && FolderName == "")
  {
    Show = Initialize ? path : "Ready";
    if (WriteFile(""))
    {
      return SDOK;
    }
    else
    {
      return Err_File_Write_Failed;
    }
  }

  if (!Initialize && FolderName != "")
  {
    CreateFile(FolderName);
  }

  if (Initialize)
  {
    Show = path;
    int line = SaveString.lastIndexOf("\n") + 1;
    String Temp = SaveString.substring(0, line);
    if (WriteFile(Temp))
    {
      SaveString = SaveString.substring(line);
      return SDOK;
    }
    else
    {
      return Err_File_Write_Failed;
    }
  }
  else
  {
    CheckSaveString(SaveString);
    return Err_File_Not_Create;
  }
}

String SDCard::Read(String FileName)
{
  if (SD.exists(FileName))
  {
    File NewFile = SD.open(FileName, FILE_READ);
    String ReadString = NewFile.readString();
    NewFile.close();
    ReadString.replace(" ","");
    return ReadString;
  }
  else
  {
    return "";
  }
}

void SDCard::CheckSaveString(String &SaveString)
{
  if (SaveString.length() > 16384)
  {
    SaveString = SaveString.substring(SaveString.indexOf("\n", 1000) + 1);
  }

  if (DebugString.length() > 16384)
  {
    DebugString = DebugString.substring(SaveString.indexOf("\n", 1000) + 1);
  }
}

void SDCard::SetPin(byte sck, byte miso, byte mosi, byte ss)
{
  SSPin = ss;
  pinMode(SSPin, OUTPUT);
  digitalWrite(SSPin,HIGH);
  spi = new SPIClass();
  spi->begin(sck, miso, mosi, ss);
}

bool SDCard::WriteFile(String SaveString)
{
  size_t bytes_written;
  if (DebugString.length() != 0)
  {
    File DebugFile = SD.open(DebugPath, FILE_APPEND);
    bytes_written = DebugFile.print(DebugString);
    DebugFile.close();
    if (bytes_written != 0)
    {
      DebugString = "";
    }
  }
  if (SaveString.length() != 0)
  {
    File DataFile = SD.open(path, FILE_APPEND);
    bytes_written = DataFile.print(SaveString);
    DataFile.close();
  }
  return (bytes_written != 0);
}

void SDCard::CreateFile(String FolderName)
{
  AvoidWDTimeOut = millis();
  String Headler = "";
  Headler += "Version : 3.1 \n";
  Headler += "Sensor ID / Local File Create Date (YMD) : " + FolderName + "\n";
  Headler += "Time, ID, X, Y, Z, Standard, Gravity, Temperature";
  Headler += "\n";

  if (AvoidWDTimeOutCount == 0)
  {
    rootForCreate = SD.open(FolderName);
    if (!rootForCreate)
    {
      String FN = FolderName + "/";
      int l = 0;
      do
      {
        l = FN.substring(l + 1).indexOf("/") + l + 2;
        File ParentFolder = SD.open(FN.substring(0, l - 1));
        if (!ParentFolder)
        {
          if (!SD.mkdir(FN.substring(0, l - 1)))
          {
            Debug.println("[SD Failed] open dir" + FN.substring(0, l - 1) + "Faild");
            Initialize = false;
            return;
          }
        }
        ParentFolder.close();
      } while (l != FN.lastIndexOf("/") + 1);
      rootForCreate = SD.open(FolderName);
    }
    String pathtemp = "/log.csv";
    if (!SD.exists(FolderName + pathtemp))
    {
      path = FolderName + pathtemp;
      File NewFile = SD.open(path, FILE_APPEND);
      Debug.println("[SD] SD Start. Create New File : " + path);
      NewFile.print(Headler); // file headling
      NewFile.close();
      rootForCreate.close();
      Initialize = true;
      return;
    }
  }
  temp = 0;
  String StartWithPathName = "log(";
  // String StartWithPathName = "/" +NewFilename + "(";
  File file = rootForCreate.openNextFile(FILE_READ);
  while (millis() - AvoidWDTimeOut < 4000 && file)
  {
    String ThisFileName = String(file.name());
    if (ThisFileName.endsWith(").csv") && ThisFileName.startsWith(StartWithPathName))
    {
      int ThisFileNum = ThisFileName.substring(StartWithPathName.length()).toInt();
      temp = max(temp, ThisFileNum);
    }
    file.close();
    AvoidWDTimeOutCount++;
    file = rootForCreate.openNextFile(FILE_READ);
  }

  if (!file)
  {
    path = FolderName + "/log(" + String(temp + 1) + ").csv";
    File NewFile = SD.open(path, FILE_APPEND);
    NewFile.print(Headler); // file headling
    NewFile.close();
    rootForCreate.close();
    Debug.println("[SD] SD Start. Create New File : " + path);
    Initialize = true;
    AvoidWDTimeOutCount++;
  }
  else
  {
    Debug.println("[SD] " + String(AvoidWDTimeOutCount) + " file search.");
    Serial.printf("Free heap (SDCard.h 89) : %d \n", ESP.getFreeHeap());
  }
}

void SDCard::Swich()
{
  SwichMode = !SwichMode;
  CheckCount = SwichMode - 1;
  Debug.print("[SD] Save Function Swich ");
  Debug.println(String(SwichMode));
  Show = SwichMode ? DefaltShow : "Function OFF";
}

void SDCard::Swich(bool State)
{
  if (SwichMode != State)
  {
    SwichMode = State;
    CheckCount = SwichMode - 1;
    Debug.print("[SD] Save Function Swich ");
    Debug.println(String(SwichMode));
  }
}

void SDCard::CheckFault(bool add, String DebugPrint)
{
  if (add)
  {
    CheckCount++;
    if (CheckCount < 5)
    {
      Debug.println(DebugPrint);
    }
    else if (AutoOff)
    {
      Swich(0);
    }
  }
  else
  {
    Debug.println(DebugPrint);
    CheckCount = 0;
  }
}

byte SDCard::CheckState()
{ // Close SD function if the Card Attach after 5 check
  if (!Initialize && !SD.begin(SSPin, *spi, 4000000))
  {
    CheckFault(1, "[SD Failed] SD Begin Failed");
    return Err_SD_begin_Failed;
  }

  if (SD.begin(SSPin, *spi, 4000000) == 0)
  {
    /* Check
    1.Is SD card insert
    2.Is SD card model well connect
    3.Is LoRa well connect (LoRa will return true even if LoRa VCC isn't well connect,
      but will cause the SD card begin failed)
    */
    CheckFault(1, "[SD Failed] SD Begin Failed");
    return Err_SD_begin_Failed;
  }
  root = SD.open("/");
  if (root == false)
  {
    CheckFault(1, "[SD Failed] SD open Failed");
    root.close();
    return Err_SD_Open_Failed;
  }
  root.close();

  if (Initialize)
  {
    FileName = SD.open(path, FILE_READ);
    if (FileName == false)
    {
      int l = 0;
      do
      {
        l = path.substring(l + 1).indexOf("/") + l + 2;
        File ParentFolder = SD.open(path.substring(0, l - 1));
        if (!ParentFolder)
        {
          if (!SD.mkdir(path.substring(0, l - 1)))
          {
            CheckFault(1, "[SD Failed] open dir" + path.substring(0, l - 1) + "Faild");
            return Err_File_Open_Failed;
          }
        }
        ParentFolder.close();
      } while (l != path.lastIndexOf("/") + 1);
      FileName = SD.open(path, FILE_APPEND);
      if (FileName == false)
      {
        CheckFault(1, "[SD Failed] File open Failed");
        FileName.close();
        return Err_File_Open_Failed;
      }
      else
      {
        CheckFault(0, "[SD] File " + path + " didn't exist. Create a new one.");
      }
    }
    FileName.close();
  }

  if (CheckCount != 0)
  {
    CheckFault(0, "[SD] Auto Save Start.");
  }
  return SDOK;
}
