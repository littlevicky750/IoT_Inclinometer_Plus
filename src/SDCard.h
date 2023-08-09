/*
 *CreateFile : Create .csv with NewFileName (or NewFileName(n), auto detect) if SD card insert
 *             New file contain headling
 *             Serial print "SD Card Fail" if SD card initialize failed
 *WriteFile  : Write String into New File
 */
#ifndef SDCard_h
#define SDCard_h

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

class SDCard
{
public:
  void SetPin(byte sck, byte miso, byte mosi, byte ss);
  bool AutoOff = true;
  void Swich(bool State);
  void Swich();
  byte Save(String FolderName, String &SaveString);
  String Read(String FileName);
  byte SDOK = 0;
  byte Err_SD_Off = 1;
  byte Err_SD_begin_Failed = 2;
  byte Err_SD_Open_Failed = 3;
  byte Err_File_Open_Failed = 4;
  byte Err_File_Not_Create = 5;
  byte Err_File_Write_Failed = 6;
  String path;
  String DebugString = "";
  String Show = "Loading...";
  int LastCheck = 0;
  int Cursor = 0;

private:
  SPIClass *spi = NULL;
  String DefaltShow = Show;
  byte SSPin;
  File root;
  File rootForCreate;
  File FileName;
  int temp;
  int AvoidWDTimeOut;
  int UsedSpace = 0;
  int CheckCount = -1;
  int AvoidWDTimeOutCount = 0;
  bool SwichMode = false;
  bool Initialize = false;
  void CheckFault(bool add, String DebugPrint);
  void CheckSaveString(String &SaveString);
  void CreateFile(String FolderName);
  bool WriteFile(String SaveString);
  byte CheckState();
  String DebugPath = "/Debug.txt";
};

#endif
