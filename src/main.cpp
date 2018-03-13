//define used in task scheduler
#define _TASK_TIMECRITICAL
#define _TASK_WDT_IDS
#define _TASK_PRIORITY
#define _TASK_STATUS_REQUEST
#define HTTP_WEBSERVER_PORT 80
#define DATASERVER_PORT 1504
#define DEBUG_ESP_OTA

#include "ModbusIP_ESP8266.h"
#include "Include.h"
#include "Sensitive.h"
#include "TFT_eSPI.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <Modbus.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include <TaskScheduler.h>
#include <EEPROM.h>
#include "Adafruit_MCP23017.h"
#include <Wire.h>
#include "SPI.h"
#include "Free_Fonts.h"
#include "EspSaveCrash.h"
#include "Esp.h"
#include <Time.h>
#include "build_defs.h"
#include "FS.h"
#include <ESP8266FtpServer.h>
#include <RemoteDebug.h>

//classes
ModbusIP mb; //ModbusIP object
DHT dht;
Adafruit_MCP23017 mcp;
TFT_eSPI tft = TFT_eSPI();
Scheduler runner;
ESP8266WebServer webServer(HTTP_WEBSERVER_PORT);
WiFiServer DataServer(DATASERVER_PORT);
IPAddress glb_ipAddress;
RemoteDebug Debug;
File fsUploadFile;

Task taskModbusReadData_1(30, TASK_FOREVER, &Modbus_ReadData, NULL);
Task taskDHT11Temp_2(2000, TASK_FOREVER, &DHT11_TempHumidity, NULL);
Task taskTimeRoutine_3(1000, TASK_FOREVER, &TimeRoutine, NULL);
Task taskLED_Error_4(15000, TASK_FOREVER, &LED_Error, NULL);
Task taskWifiCheckStatus_5(1000, TASK_FOREVER, &Wifi_CheckStatus, NULL);
Task taskThermostatDetect_6(1000, TASK_FOREVER, &Thermostat_Detect, NULL);
Task taskIoControlPins_7(100, TASK_FOREVER, &IO_ControlPins, NULL);
Task taskTelnet_8(20, TASK_FOREVER, &TelnetServer_Process, NULL);
Task taskLED_onEnable_9(4000, TASK_ONCE, NULL, NULL, false, &LED_OnEnable, &LED_OnDisable);
Task taskLED_OnDisable_10(TASK_IMMEDIATE, TASK_FOREVER, NULL, NULL, true, NULL, &LED_Off);
Task taskErrorsCodesProcess_11(500, TASK_FOREVER, &ErrorCodes_Process, NULL);
Task taskModbusProcess_12(50, TASK_FOREVER, &Modbus_Process, NULL);
Task taskEEpromProcess_13(1000, TASK_FOREVER, &EEPROM_Process, NULL);
Task taskMBcoilReg11_14(TASK_IMMEDIATE, TASK_ONCE, NULL, NULL, false, NULL, &ESP_Restart);
Task taskWebServer_Process_15(1000, TASK_FOREVER, &WebServer_Process, NULL);
Task taskDataServer_Process_16(10, TASK_FOREVER, &DataServer_Process, NULL);
Task taskModbusClientSend_17(5000, TASK_FOREVER, &Modbus_Client_Send, NULL);
Task taskOTA_Update_18(100, TASK_FOREVER, &OTA_Update, NULL);
Task taskLogDataSave_19(60000, TASK_FOREVER, &DataLog_Save, NULL);
Task taskLCDUpdate_20(1000, TASK_FOREVER, &LCD_Update, NULL);

//************************************************************************************
void LCD_Update()
{
  int startTimeMicros = micros();
  int endTimeMicros = 0;

  tft.fillScreen(TFT_BLACK);
  String newbuf1 = "Temp:" + String(glb_temperature);
  String newbuf2 = "Humd:" + String(glb_humidity);
  String newbuf3 = "Pkts:" + String(glb_dataServerCounter);
  LCD_DrawText(0, 0, "Temp    :" + String(glb_temperature), TFT_WHITE, TFT_BLACK);
  LCD_DrawText(0, 10, "Humidity:" + String(glb_humidity), TFT_WHITE, TFT_BLACK);
  LCD_DrawText(0, 20, "Pkts    :" + String(glb_dataServerCounter), TFT_WHITE, TFT_BLACK);
  LCD_DrawText(0, 30, "Status  :" + glb_dhtStatusError, TFT_WHITE, TFT_BLACK);
  LCD_DrawText(0, 40, "Time    :" + glb_TimeShort, TFT_WHITE, TFT_BLACK);
  LCD_DrawText(0, 50, "Free mem:" + String(ESP.getFreeHeap()), TFT_WHITE, TFT_BLACK);
  LCD_DrawText(0, 60, "IP Addr :" + glb_ipAddress.toString(), TFT_WHITE, TFT_BLACK);

  endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  glb_TaskTimes[20] = endTimeMicros;

}
//************************************************************************************
void PrintFromFile(String filepath, bool debug)
{
  if (SPIFFS.exists(filepath))
  {
    //Debug.println("File exists...");
    File f = SPIFFS.open(filepath, "r");
    while (f.available())
    {
      //Lets read line by line from the file
      String line = f.readStringUntil('\n');
      if (debug)
      {
        Debug.println(line);
      }
      else
      {
        Debug.print(line);
      }
    }
    f.close();
  }
  else
  {
    Debug.println("File not found error...");
  }
}
//************************************************************************************
void TelnetServer_Process()
{
  int startTimeMicros = micros();
  int endTimeMicros = 0;
  Debug.handle();
  endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  glb_TaskTimes[8] = endTimeMicros;
}
//************************************************************************************
void TelnetServer_ProcessCommand()
{
  char tmpglb_SSID[32];
  char tmpglb_SSIDpassword[32];
  char tmpLastCmd[32];

  String lastCmd = Debug.getLastCommand();

  if (lastCmd == "file format")
  {
    if (Debug.isActive(Debug.ANY))
    {
      SPIFFS.format();
      Debug.print("File system formatted...");
    }
  }
  else if (lastCmd == "file list")
  {
    Debug.println("File list");
    FileSystem_ListDirectory();
  }
  else if (lastCmd == "temperature")
  {
    Debug.println(glb_temperature);
  }
  else if (lastCmd == "humidity")
  {
    Debug.println(glb_humidity);
  }
  else if (lastCmd == "light sensor")
  {
    Debug.println(glb_lightSensor);
  }
  else if (lastCmd == "time")
  {
    Debug.println(glb_TimeLong);
  }
  else if (lastCmd.startsWith("hreg"))
  {
    String tReg = lastCmd.substring(5, lastCmd.length());
    int iReg = tReg.toInt();
    Debug.println(mb.Hreg(iReg));
  }
  else if (lastCmd.startsWith("coil"))
  {
    String tReg = lastCmd.substring(5, lastCmd.length());
    int iReg = tReg.toInt();
    Debug.println(mb.Coil(iReg));
  }
  else if (lastCmd.startsWith("set coil"))
  {
    String tReg = lastCmd.substring(9, lastCmd.length());
    int iReg = tReg.toInt();
    mb.Coil(iReg, 1);
    Debug.println(mb.Coil(iReg));
  }
  else if (lastCmd.startsWith("clr coil"))
  {
    String tReg = lastCmd.substring(9, lastCmd.length());
    int iReg = tReg.toInt();
    mb.Coil(iReg, 0);
    Debug.println(mb.Coil(iReg));
  }
  else if (lastCmd.startsWith("set hreg"))
  {
    String tReg = lastCmd.substring(9, 12);
    int iReg = tReg.toInt();
    int firstSpace = lastCmd.indexOf(" ", 9);
    String tVal = lastCmd.substring(firstSpace + 1, lastCmd.length());
    int iVal = tVal.toInt();
    mb.Hreg(iReg, iVal);
    Debug.println(mb.Hreg(iReg));
  }
  else if (lastCmd.startsWith("set esp pin"))
  {
    String tReg = lastCmd.substring(12, 13);
    int iReg = tReg.toInt();
    //int firstSpace = lastCmd.indexOf(" ", 12);
    //String tVal = lastCmd.substring(firstSpace + 1, lastCmd.length());
    String tVal = lastCmd.substring(15, 15);
    int iVal = tVal.toInt();
    Debug.print(iReg);Debug.print(":");Debug.print(iVal);
    testEspOutputPin(iReg, iVal);
  }
  else if (lastCmd.startsWith("set mcp pin"))
  {
    String tReg = lastCmd.substring(12, 14);
    Debug.println(lastCmd);
    Debug.println(tReg);
    int iReg = tReg.toInt();
    //int firstSpace = lastCmd.indexOf(" ", 1);
    //String tVal = lastCmd.substring(firstSpace + 1, lastCmd.length());
    String tVal = lastCmd.substring(lastCmd.length() - 1, lastCmd.length());
    Debug.println(tVal);
    int iVal = tVal.toInt();
    Debug.print(iReg);Debug.print(":");Debug.print(iVal);
    testMcpOutputPin(iReg, iVal);
  }

  else if (lastCmd.startsWith("task enable"))
  {
    String tReg = lastCmd.substring(12, 2);
    int iReg = tReg.toInt();
    if (iReg == 1)
      taskModbusReadData_1.enable();
    if (iReg == 2)
      taskDHT11Temp_2.enable();
    if (iReg == 3)
      taskTimeRoutine_3.enable();
    if (iReg == 4)
      taskLED_Error_4.enable();
    if (iReg == 5)
      taskWifiCheckStatus_5.enable();
    if (iReg == 6)
      taskThermostatDetect_6.enable();
    if (iReg == 7)
      taskIoControlPins_7.enable();
    if (iReg == 8)
      taskTelnet_8.enable();
    if (iReg == 9)
      taskLED_onEnable_9.enable();
    if (iReg == 10)
      taskLED_OnDisable_10.enable();
    if (iReg == 11)
      taskErrorsCodesProcess_11.enable();
    if (iReg == 12)
      taskModbusProcess_12.enable();
    if (iReg == 13)
      taskEEpromProcess_13.enable();
    if (iReg == 14)
      taskMBcoilReg11_14.enable();
    if (iReg == 15)
      taskWebServer_Process_15.enable();
    if (iReg == 16)
      taskDataServer_Process_16.enable();
    if (iReg == 17)
      taskModbusClientSend_17.enable();
    if (iReg == 18)
      taskOTA_Update_18.enable();
    if (iReg == 19)
      taskLogDataSave_19.enable();
    Debug.println("ok");
  }
  else if (lastCmd.startsWith("task disable"))
  {
    String tReg = lastCmd.substring(13, 2);
    int iReg = tReg.toInt();
    if (iReg == 1)
      taskModbusReadData_1.disable();
    if (iReg == 2)
      taskDHT11Temp_2.disable();
    if (iReg == 3)
      taskTimeRoutine_3.disable();
    if (iReg == 4)
      taskLED_Error_4.disable();
    if (iReg == 5)
      taskWifiCheckStatus_5.disable();
    if (iReg == 6)
      taskThermostatDetect_6.disable();
    if (iReg == 7)
      taskIoControlPins_7.disable();
    if (iReg == 8)
      taskTelnet_8.disable();
    if (iReg == 9)
      taskLED_onEnable_9.disable();
    if (iReg == 10)
      taskLED_OnDisable_10.disable();
    if (iReg == 11)
      taskErrorsCodesProcess_11.disable();
    if (iReg == 12)
      taskModbusProcess_12.disable();
    if (iReg == 13)
      taskEEpromProcess_13.disable();
    if (iReg == 14)
      taskMBcoilReg11_14.disable();
    if (iReg == 15)
      taskWebServer_Process_15.disable();
    if (iReg == 16)
      taskDataServer_Process_16.disable();
    if (iReg == 17)
      taskModbusClientSend_17.disable();
    if (iReg == 18)
      taskOTA_Update_18.disable();
    if (iReg == 19)
      taskLogDataSave_19.disable();
    Debug.println("ok");
  }
  else if (lastCmd.startsWith("wifi ssid"))
  {
    EEPROM.get(glb_eepromSettingsOffset + ES_SSID, tmpglb_SSID);
    Debug.println(tmpglb_SSID);
  }
  else if (lastCmd.startsWith("set wifi ssid"))
  {
    String tReg = lastCmd.substring(14);
    tReg.toCharArray(tmpLastCmd, tReg.length() + 1);
    Debug.print("Old SSID:");
    EEPROM.get(glb_eepromSettingsOffset + ES_SSID, tmpglb_SSID);
    Debug.println(tmpglb_SSID);
    Debug.print("New SSID:");
    Debug.println(tmpLastCmd);
    EEPROM.put(glb_eepromSettingsOffset + ES_SSID, tmpLastCmd);
    EEPROM.commit();
    Debug.println("Will not take affect until restart...");
  }
  else if (lastCmd.startsWith("wifi pass"))
  {
    EEPROM.get(glb_eepromSettingsOffset + ES_SSIDPASSWORD, glb_SSIDpassword);
  }
  else if (lastCmd.startsWith("set wifi pass"))
  {
    String tReg = lastCmd.substring(14);
    tReg.toCharArray(tmpLastCmd, tReg.length() + 1);
    Debug.print("Old SSID pass:");
    Debug.println(EEPROM.get(glb_eepromSettingsOffset + ES_SSIDPASSWORD, tmpglb_SSIDpassword));
    Debug.print("New SSID pass:");
    Debug.println(tmpLastCmd);
    EEPROM.put(glb_eepromSettingsOffset + ES_SSIDPASSWORD, tmpLastCmd);
    EEPROM.commit();
    Debug.println("Will not take affect until restart...");
  }
  else if (lastCmd.startsWith("wifi scan"))
  {
    int numberOfNetworks = WiFi.scanNetworks();
    for (int i = 0; i < numberOfNetworks; i++)
    {
      Debug.print("Network name: ");
      Debug.println(WiFi.SSID(i));
      Debug.print("Signal strength: ");
      Debug.println(WiFi.RSSI(i));
      Debug.println("-----------------------");
    }
  }
  else if (lastCmd.startsWith("wifi rssi"))
  {
    Debug.println(WiFi.RSSI());
  }
  else if (lastCmd.startsWith("savecrash clear"))
  {
    SaveCrash.clear();
    Debug.println("ok");
  }
  else if (lastCmd.startsWith("savecrash print"))
  {
    SaveCrash.print(Debug);
  }
  else if (lastCmd.startsWith("savecrash count"))
  {
    Debug.println(SaveCrash.count());
  }
  else if (lastCmd.startsWith("eeprom erase"))
  {
    EEPROM_Erase();
    Debug.println("ok");
  }
  else if (lastCmd.startsWith("reset reason"))
  {
    Debug.println(ESP.getResetReason());
  }
  else if (lastCmd.startsWith("free sketch size"))
  {
    Debug.println(ESP.getSketchSize());
    Debug.println("ok");
  }
  else if (lastCmd.startsWith("log size"))
  {
    File f = SPIFFS.open(glb_dataLogPath, "r");
    Debug.println(f.size());
    f.close();
  }
  else if (lastCmd.startsWith("tstat status"))
  {
    Debug.println(glb_thermostatStatus);
  }
  else if (lastCmd.startsWith("reset count"))
  {
    glb_resetCounter = 0;
    EEPROM.write(ES_RESETCOUNTER, glb_resetCounter);
    EEPROM.commit();
    Debug.println(glb_resetCounter);
  }
  else if (lastCmd.startsWith("reset clear"))
  {
    glb_resetCounter = 0;
    Debug.println(glb_resetCounter);
  }
  else if (lastCmd.startsWith("wifi resets"))
  {
    Debug.println(glb_wifiNotConnectedCounter);
  }
  else if (lastCmd.startsWith("debug serial on"))
  {
    Debug.setSerialEnabled(true); // All messages too send to serial too, and can be see in serial monitor
    Debug.println("ok");
  }
  else if (lastCmd.startsWith("debug serial off"))
  {
    Debug.setSerialEnabled(false); // All messages too send to serial too, and can be see in serial monitor
    Debug.println("ok");
  }
  else if (lastCmd.startsWith("dht debug on"))
  {
    glb_DHT11debugOn = true;
    Debug.println(glb_DHT11debugOn);
  }
  else if (lastCmd.startsWith("dht debug off"))
  {
    glb_DHT11debugOn = false;
    Debug.println(glb_DHT11debugOn);
  }
  else if (lastCmd.startsWith("datalog debug on"))
  {
    glb_logDataDebug = true;
    Debug.println("ok");
  }
  else if (lastCmd.startsWith("log debug off"))
  {
    glb_logDataDebug = false;
    Debug.println("ok");
  }
  else if (lastCmd.startsWith("datalog data on"))
  {
    taskLogDataSave_19.enable();
    Debug.println("ok");
  }
  else if (lastCmd.startsWith("datalog data off"))
  {
    taskLogDataSave_19.disable();
    Debug.println("ok");
  }
  else if (lastCmd.startsWith("datalog size"))
  {
    File f = SPIFFS.open(glb_dataLogPath, "r");
    Debug.println(f.size());
    f.close();
  }
  else if (lastCmd.startsWith("datalog delete"))
  {
    FileSystem_DeleteFile(glb_dataLogPath);
    Debug.println("ok");
  }
  else if (lastCmd.startsWith("datalog print"))
  {
    PrintFromFile(glb_dataLogPath, true);
  }
  else if (lastCmd.startsWith("errorlog size"))
  {
    File f = SPIFFS.open(glb_errorLogPath, "r");
    Debug.println(f.size());
    f.close();
  }
  else if (lastCmd.startsWith("errorlog delete"))
  {
    FileSystem_DeleteFile(glb_errorLogPath);
    Debug.println("ok");
  }
  else if (lastCmd.startsWith("errorlog print"))
  {
    PrintFromFile(glb_errorLogPath, true);
  }
  else if (lastCmd.startsWith("dataserver count"))
  {
    Debug.println(glb_dataServerCounter);
  }
  else if (lastCmd.startsWith("boot time"))
  {
    Debug.println(glb_BootTime);
  }
  else if (lastCmd.startsWith("task times"))
  {
    for (int i=1; i < 21; i++)
    {
      Debug.print(i);Debug.print(":");
      Debug.println(glb_TaskTimes[i]);
    }
  }
  else if (lastCmd.startsWith("file delete"))
  {
    String tReg = lastCmd.substring(12, lastCmd.length());
    tReg = "/" + tReg;
    Debug.println(tReg);
    bool val = FileSystem_DeleteFile(tReg);
    if (val)
    {
     Debug.println("ok");
    }
    else
    {
      Debug.println("File not found...");
    }
  }
  else if (lastCmd.startsWith("list"))
  {
    Debug.println("file format");
    Debug.println("file list");
    Debug.println("temperature");
    Debug.println("humidity");
    Debug.println("light sensor");
    Debug.println("time");
    Debug.println("hreg xxx");
    Debug.println("set hreg xxx");
    Debug.println("coil xxx");
    Debug.println("set coil xxx");
    Debug.println("clr coil xxx");
    Debug.println("task enable xx");
    Debug.println("task disable xx");
    Debug.println("wifi ssid");
    Debug.println("set wifi ssid");
    Debug.println("wifi pass");
    Debug.println("set wifi pass");
    Debug.println("wifi scan");
    Debug.println("wifi rssi");
    Debug.println("savecrash clear");
    Debug.println("savecrash print");
    Debug.println("savecrash count");
    Debug.println("eeprom erase");
    Debug.println("reset reason");
    Debug.println("reset count");
    Debug.println("reset clear");
    Debug.println("free sketch size");
    Debug.println("tstat status");
    Debug.println("wifi resets");
    Debug.println("debug serial on");
    Debug.println("debug serial off");
    Debug.println("datalog debug on");
    Debug.println("datalog debug off");
    Debug.println("datalog data on");
    Debug.println("datalog data off");
    Debug.println("datalog delete");
    Debug.println("datalog print");
    Debug.println("datalog size");
    Debug.println("dht debug on");
    Debug.println("dht debug off");
    Debug.println("errorlog delete");
    Debug.println("errorlog print");
    Debug.println("errorlog size");
    Debug.println("dataserver count");
    Debug.println("boot time");
    Debug.println("set esp pin xx y");
    Debug.println("set mcp pin xx y");
    Debug.println("task times");
    Debug.println("file delete xxxxx.xxx");
  }
}
//************************************************************************************
void ErrorLogData_Save(String data)
{

  File glb_errorLog = SPIFFS.open(glb_errorLogPath, "a");
  glb_errorLog.print(glb_TimeLong);
  glb_errorLog.print(",");
  glb_errorLog.println(data);
  glb_errorLog.close();
}
//************************************************************************************
void DataLog_Save()
{
  //currently 49 bytes per entry

  int startTimeMicros = micros();
  int endTimeMicros = 0;

  if (glb_logDataDebug)
    Debug.println("Save log data");
  if (glb_logDataDebug)
    Debug.print("Time:");
  if (glb_logDataDebug)
    Debug.println(glb_TimeLong);
  if (glb_logDataDebug)
    Debug.print("Temp:");
  if (glb_logDataDebug)
    Debug.println(glb_temperature);
  if (glb_logDataDebug)
    Debug.print("Humidity:");
  if (glb_logDataDebug)
    Debug.println(glb_humidity);

  File glb_temperatureLog = SPIFFS.open(glb_dataLogPath, "a");
  if (glb_logDataDebug) Debug.println(glb_temperatureLog);

  if (glb_temperatureLog)
  {
    if (glb_logDataDebug) Debug.print("Filelog size...");
    if (glb_logDataDebug) Debug.println(glb_temperatureLog.size());
    glb_dataLogCount++;
    glb_temperatureLog.print(glb_dataLogCount);
    glb_temperatureLog.print(",");
    glb_temperatureLog.print(glb_timeMonth);
    glb_temperatureLog.print(",");
    glb_temperatureLog.print(glb_timeDay);
    glb_temperatureLog.print(",");
    glb_temperatureLog.print(glb_timeYear);
    glb_temperatureLog.print(",");
    glb_temperatureLog.print(glb_timeWeekDay);
    glb_temperatureLog.print(",");
    glb_temperatureLog.print(glb_timeHour);
    glb_temperatureLog.print(",");
    glb_temperatureLog.print(glb_timeMin);
    glb_temperatureLog.print(",");
    glb_temperatureLog.print(glb_timeSec);
    glb_temperatureLog.print(",");
    glb_temperatureLog.print(glb_temperature);
    glb_temperatureLog.print(",");
    glb_temperatureLog.print(glb_humidity);
    glb_temperatureLog.print(",");
    glb_temperatureLog.print(glb_lightSensor);
    glb_temperatureLog.print(",");
    glb_temperatureLog.print(mb.Coil(THERMOSTAT_HEAT_CALL_MB_COIL));
    glb_temperatureLog.print(",");
    glb_temperatureLog.print(mb.Coil(THERMOSTAT_COOL_CALL_MB_COIL));
    glb_temperatureLog.print(",");
    glb_temperatureLog.print(mb.Coil(THERMOSTAT_FAN_CALL_MB_COIL));
    glb_temperatureLog.print(",");
    glb_temperatureLog.print(mb.Coil(HEAT_OVERRIDE_PIN));
    glb_temperatureLog.print(",");
    glb_temperatureLog.print(mb.Coil(COOL_OVERRIDE_PIN));
    glb_temperatureLog.print(",");
    glb_temperatureLog.print(mb.Coil(FAN_OVERRIDE_PIN));
    glb_temperatureLog.print(",");
    glb_temperatureLog.print(mb.Coil(HEAT_CONTROL_PIN));
    glb_temperatureLog.print(",");
    glb_temperatureLog.print(mb.Coil(COOL_CONTROL_PIN));
    glb_temperatureLog.print(",");
    glb_temperatureLog.println(mb.Coil(FAN_CONTROL_PIN));
    glb_temperatureLog.close();
  }
  else
  {
    if (glb_logDataDebug)
      Debug.println("File open failed...");
  }
  endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  glb_TaskTimes[19] = endTimeMicros;
  if (endTimeMicros > 100000)
  {
    Serial.print("save data went long : ");
    Serial.println(endTimeMicros);
  }
}
//************************************************************************************
void OTA_Update()
{
  int startTimeMicros = micros();
  int endTimeMicros = 0;

  ArduinoOTA.handle();

  endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  glb_TaskTimes[18] = endTimeMicros;
}
//************************************************************************************
void Modbus_ReadData()
{
  bool debug = 0;
  if (debug)
    Debug.println("Modbus Routine");

  word returnValue = 0;
  int startTimeMicros = micros();
  int endTimeMicros = 0;

  static word taskMbStartDelayCounter = 0;
  static word taskMbOverrunPosCounter = 0;
  static word taskMbOverrunNegCounter = 0;
  static word noClientCounter = 0;
  static word goodPacketCounter = 0;
  static word badPacketCounter = 0;
  static word notModbusPacketCounter = 0;
  static word largeFrameCounter = 0;
  static word failedWriteCounter = 0;

  returnValue = mb.task();

  if (debug)
  {
    if (returnValue != 1)
      Debug.println(returnValue);
  }

  if (taskModbusReadData_1.getStartDelay() > 0)
  {
    taskMbStartDelayCounter++;
    mb.Hreg(MB_START_DELAY_COUNTER_MB_REG, (word)taskMbStartDelayCounter);
  }

  if (taskModbusReadData_1.getOverrun() < 0)
  {
    taskMbOverrunNegCounter++;
    mb.Hreg(MB_OVERRUN_NEG_COUNTER_MB_REG, (word)taskMbOverrunNegCounter);
  }

  if (taskModbusReadData_1.getOverrun() > 0)
  {
    taskMbOverrunPosCounter++;
    mb.Hreg(MB_OVERRUN_POS_COUNTER_MB_REG, (word)taskMbOverrunPosCounter);
  }

  if (returnValue == 0)
  { //NO_CLIENT
    noClientCounter++;
    mb.Hreg(NO_CLIENT_COUNTER_MB_REG, (word)noClientCounter);
  }
  if (returnValue == 1)
  { //GOOD_PACKET_COUNTER
    goodPacketCounter++;
    mb.Hreg(GOOD_PACKET_COUNTER_MB_REG, (word)goodPacketCounter);
  }
  if (returnValue == 2)
  { //BAD_PACKET_COUNTER
    badPacketCounter++;
    mb.Hreg(BAD_PACKET_COUNTER_MB_REG, (word)badPacketCounter);
  }
  if (returnValue == 3)
  { //NOT_MODBUS_PKT
    notModbusPacketCounter++;
    mb.Hreg(NOT_MODBUS_PACKET_COUNTER_MB_REG, (word)notModbusPacketCounter);
  }
  if (returnValue == 4)
  { //LARGE_FRAME
    largeFrameCounter++;
    mb.Hreg(LARGE_FRAME_COUNTER_MB_REG, (word)largeFrameCounter);
  }
  if (returnValue == 5)
  { //FAILED_WRITE
    failedWriteCounter++;
    mb.Hreg(FAILED_WRITE_COUNTER_MB_REG, (word)failedWriteCounter);
  }

  endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  mb.Hreg(MB_ROUTINE_TIME_MB_HREG, int((word)endTimeMicros));
  glb_TaskTimes[1] = endTimeMicros;
}
//************************************************************************************
void TimeRoutine()
{
  bool debug = 0;
  int startTimeMicros = micros();
  int static secondsCounter = 0;
  unsigned long start = millis();
  unsigned long timeout = 1000;
  static bool firstRun = true;
  struct tm *timeinfo;
  static time_t now;
  time_t tmpNow;

  if (debug) Serial.println("Time Routine");
  if (debug) Serial.println("Waiting for time from time sync");

  configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  secondsCounter++;
  now++;

  if (debug) Serial.print("seconds:");
  if (debug) Serial.println(secondsCounter);
  if (debug) Serial.print("firstRun:");
  if (debug) Serial.println(firstRun);

  if (secondsCounter >= 300 || (firstRun))
  {
    while ( (millis() - start) < timeout)
    {
      tmpNow = time(nullptr);
      if (tmpNow != 0)
        break;
        firstRun = false;
      secondsCounter = 0;
      delay(100);
    }
  }

  if (tmpNow == 0)
  {
    if (debug) Debug.println("No NTP update");
    //firstRun = true;
  }
  else
  {
    now = tmpNow;
    //firstRun = false;
  }

  if (debug) Serial.print("now2:");
  if (debug) Serial.println(now);

  if (debug) Serial.print("&now:");
  if (debug) Serial.print(ctime(&now));

  timeinfo = localtime(&now);
  glb_TimeLong = ctime(&now);
  glb_TimeLong.trim();
  glb_timeHour = String(timeinfo->tm_hour);
  glb_timeMin = String(timeinfo->tm_min);
  glb_timeSec = String(timeinfo->tm_sec);
  glb_timeMonth = String(timeinfo->tm_mon);
  glb_timeDay = String(timeinfo->tm_mday);
  glb_timeYear = String((timeinfo->tm_year) - 100);
  glb_timeWeekDay = String(timeinfo->tm_wday);
  glb_TimeShort = String(glb_timeHour) + ":" + String(glb_timeMin) + ":" + String(glb_timeSec);

  mb.Hreg(TIME_HH_MB_HREG, glb_timeHour.toInt());
  mb.Hreg(TIME_MM_MB_HREG, glb_timeMin.toInt());
  mb.Hreg(TIME_SS_MB_HREG, glb_timeSec.toInt());
  int endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  mb.Hreg(NTP_LOOP_TIME_MB_HREG, (word)endTimeMicros);
  glb_TaskTimes[3] = endTimeMicros;
  if (endTimeMicros > 100000) Serial.println("big time");

}
//************************************************************************************
void ESP_Restart()
{
  bool debug = 1;
  if (debug)
    Serial.println("Restarting ESP");
  ErrorLogData_Save("Restart command received...");
  delay(0);
  ESP.restart();
}
//************************************************************************************
void Modbus_Process()
{
  bool debug = 0;
  int startTimeMicros = micros();
  int endTimeMicros = 0;

  if (debug) Serial.println("Processing Modbus...");

  for (int x = 1; x <= glb_maxCoilSize - 1; x++)
  {
    if (glb_eepromCoilCopy[x] != mb.Coil(x))
    {
      glb_eepromCoilCopy[x] = mb.Coil(x);
      if (debug)
        Serial.print("mb coil:");
      if (debug)
        Serial.print(x);
      if (debug)
        Serial.print(" value:");
      if (debug)
        Serial.print(mb.Coil(x));
      if (debug)
        Serial.print(" eepromcopy:");
      if (debug)
        Serial.println(glb_eepromCoilCopy[x]);
      if (debug)
        Serial.println(x);

      if (x == ESP_RESTART_MB_COIL)
      {
        if (debug)
          Serial.println("case 11");
        //mb.Coil(ESP_RESTART_MB_COIL, COIL_OFF);
        //glb_eepromCoilCopy[ESP_RESTART_MB_COIL] = int(mb.Coil(x));
        Serial.print("Rebooting unit in 3 seconds...");
        taskMBcoilReg11_14.enableDelayed(3000);
      }

      if (x == ESP_CLEAR_SAVECRASH_DATA)
      {
        if (debug)
          Serial.println("case 12");
        //mb.Coil(ESP_CLEAR_SAVECRASH_DATA, COIL_OFF);
        //glb_eepromCoilCopy[ESP_RESTART_MB_COIL] = int(mb.Coil(x));
        Serial.println("Clearing SAVECRASH data...");
        SaveCrash.clear();
      }
    }
    else
    {
      glb_eepromCoilCopy[x] = mb.Coil(x);
    }
  }

  for (int x = 1; x <= glb_maxHregSize - 1; x++)
  {
    glb_eepromHregCopy[x] = mb.Hreg(x);
  }

  glb_freeHeap = ESP.getFreeHeap();
  mb.Hreg(ESP_MEMORY_MB_HREG, (word)glb_freeHeap);

  if (glb_freeHeap < glb_lowMemory)
  {
    glb_lowMemory = glb_freeHeap;
    mb.Hreg(ESP_MEMORY_LOW_POINT, (word)(glb_lowMemory));
  }

  endTimeMicros = micros();
  mb.Hreg(PROCESS_MODBUS_TIME_MB_HREG, (word)(endTimeMicros));
  endTimeMicros = endTimeMicros - startTimeMicros;glb_TaskTimes[12] = endTimeMicros;
}
//************************************************************************************
void EEPROM_Process()
{
  int startTimeMicros = micros();
  int endTimeMicros = 0;

  bool debug = 0;
  if (debug)
    Debug.println("Processing Updated EEPROM...");

  //check foir hreg change
  for (int x = 1; x < glb_maxHregSize; x++)
  {
    if (mb.Hreg(x) != glb_eepromHregCopy[x])
    {
      if (debug)
        Debug.print("mb hreg:");
      if (debug)
        Debug.print(x);
      if (debug)
        Debug.print(" value:");
      if (debug)
        Debug.print(mb.Hreg(x));
      if (debug)
        Debug.print(" eepromcopy:");
      if (debug)
        Debug.println(glb_eepromHregCopy[x]);
      //EEPROM.write( x, int(mb.Hreg(x)) );
      glb_eepromHregCopy[x] = mb.Hreg(x);
    }
  }

  //check for coil change
  for (int y = 1; y <= glb_maxCoilSize; y++)
  {
    if (bool(glb_eepromCoilCopy[y]) != mb.Coil(y))
    {
      if (debug)
        Debug.print("mb coil:");
      if (debug)
        Debug.print(y);
      if (debug)
        Debug.print(" value:");
      if (debug)
        Debug.print(mb.Coil(y));
      if (debug)
        Debug.print(" eepromcopy:");
      if (debug)
        Debug.println(glb_eepromCoilCopy[y]);
      if (debug)
        Debug.print(" y + offset:");
      if (debug)
        Debug.println(y + glb_eepromCoilOffset);
      //EEPROM.write(y + glb_eepromCoilOffset, int( mb.Coil(y) ) );
      glb_eepromCoilCopy[y] = int(mb.Coil(y));
      if (debug)
        Debug.print(" New eepromcopy:");
      if (debug)
        Debug.println(glb_eepromCoilCopy[y]);
    }
  }
  //EEPROM.commit();
  //if (debug) Debug.println("commit");

  endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  glb_TaskTimes[13] = endTimeMicros;

}
//************************************************************************************
void ErrorCodes_Process()
{

  int startTimeMicros = micros();
  int endTimeMicros = 0;

  bool debug = 0;
  if (debug)
    Debug.println("Processing Error Codes...");

  if (debug)
    Serial.print("DHT error code:");
  if (debug)
    Serial.println(glb_errorDHT);
  if (debug)
    Serial.print("Wifi error code:");
  if (debug)
    Serial.println(glb_WiFiStatus);
  if (debug)
    Serial.print("Thermostat error code:");
  if (debug)
    Serial.println(glb_errorThermostat);

  if (glb_errorDHT != 0)
  {
    glb_BlinkErrorCode = glb_errorDHT;
  }
  else if (glb_errorThermostat != 0)
  {
    glb_BlinkErrorCode = glb_errorThermostat;
  }
  else if (glb_WiFiStatus != 0)
  {
    if (debug)
      Serial.println(glb_WiFiStatus);
    glb_BlinkErrorCode = glb_WiFiStatus;
  }

  mb.Hreg(BLINK_ERROR_CODE_MB_HREG, (word)glb_BlinkErrorCode);
  if (debug)
    Serial.print("Blink Error Code:");
  if (debug)
    Serial.println(glb_BlinkErrorCode);

  endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  glb_TaskTimes[11] = endTimeMicros;
}
//************************************************************************************
void LED_Error()
{
  int startTimeMicros = micros();
  int endTimeMicros = 0;
  bool debug = 0;
  if (debug) Serial.println("Wrapper callback");
  taskLED_onEnable_9.restartDelayed();
  endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  glb_TaskTimes[4] = endTimeMicros;

}
//************************************************************************************
// Upon being enabled, taskLED_onEnable_9 will define the parameters
// and enable LED blinking task, which actually controls
// the hardware (LED in this example)
bool LED_OnEnable()
{
  int startTimeMicros = micros();
  int endTimeMicros = 0;
  bool debug = 0;
  taskLED_OnDisable_10.setInterval(500);
  taskLED_OnDisable_10.setCallback(&LED_On);
  taskLED_OnDisable_10.enable();
  taskLED_onEnable_9.setInterval((glb_BlinkErrorCode * 1000) - 100);
  if (debug) Serial.print("Blink Error Code:");
  if (debug) Serial.println(glb_BlinkErrorCode);
  endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  glb_TaskTimes[9] = endTimeMicros;
  return true; // Task should be enabled
}
//************************************************************************************
// taskLED_onEnable_9 does not really need a callback method
// since it just waits for 5 seconds for the first
// and only iteration to occur. Once the iteration
// takes place, taskLED_onEnable_9 is disabled by the Scheduler,
// thus executing its OnDisable method below.
void LED_OnDisable()
{
  bool debug = 0;
  if (debug)
    Serial.println("Blink on disable");
  taskLED_OnDisable_10.disable();
}
//************************************************************************************
void LED_On()
{
  bool debug = 0;
  if (debug) Serial.println("LED on");
  mcp.digitalWrite(LED, HIGH);
  taskLED_OnDisable_10.setCallback(&LED_Off);
}
//************************************************************************************
void LED_Off()
{
  bool debug = 0;
  int startTimeMicros = micros();
  int endTimeMicros = 0;
  if (debug) Serial.println("LED off");
  mcp.digitalWrite(LED, LOW);
  taskLED_OnDisable_10.setCallback(&LED_On);
  endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  glb_TaskTimes[10] = endTimeMicros;
}
//************************************************************************************
void Thermostat_Detect()
{
  bool debug = 0;
  if (debug)
    Serial.println("Processing thermostat detect...");

  int startTimeMicros = micros();
  int endTimeMicros;
  word localval = 0;
  glb_thermostatStatus = "Thermostat no call";

  mb.Hreg(THERMOSTAT_HEAT_CALL_PULSE_VALUE_MB_HREG, (word)glb_heatPulseDuration);
  mb.Hreg(THERMOSTAT_COOL_CALL_PULSE_VALUE_MB_HREG, (word)glb_coolPulseDuration);
  mb.Hreg(THERMOSTAT_FAN_CALL_PULSE_VALUE_MB_HREG, (word)glb_fanPulseDuration);

  if (debug)
    Serial.print("HP:");
  if (debug)
    Serial.print(glb_heatPulseDuration);
  if (debug)
    Serial.print(" CP:");
  if (debug)
    Serial.print(glb_coolPulseDuration);
  if (debug)
    Serial.print(" FP:");
  if (debug)
    Serial.print(glb_fanPulseDuration);

  if (debug)
    Serial.print(" HPC:");
  if (debug)
    Serial.print(glb_heatPulseCounter);
  if (debug)
    Serial.print(" CPC:");
  if (debug)
    Serial.print(glb_coolPulseCounter);
  if (debug)
    Serial.print(" FPC:");
  if (debug)
    Serial.print(glb_fanPulseCounter);
  if (debug)
    Serial.print(" BEC:");
  if (debug)
    Serial.print(glb_BlinkErrorCode);

  if (glb_heatPulseCounter >= 30)
  {
    mb.Coil(THERMOSTAT_HEAT_CALL_MB_COIL, COIL_ON);
  }
  else
  {
    mb.Coil(THERMOSTAT_HEAT_CALL_MB_COIL, COIL_OFF);
  }
  glb_heatPulseCounter = 0;

  if (glb_coolPulseCounter >= 30)
  {
    mb.Coil(THERMOSTAT_COOL_CALL_MB_COIL, COIL_ON);
  }
  else
  {
    mb.Coil(THERMOSTAT_COOL_CALL_MB_COIL, COIL_OFF);
  }
  glb_coolPulseCounter = 0;

  if (glb_fanPulseCounter >= 30)
  {
    mb.Coil(THERMOSTAT_FAN_CALL_MB_COIL, COIL_ON);
  }
  else
  {
    mb.Coil(THERMOSTAT_FAN_CALL_MB_COIL, COIL_OFF);
  }
  glb_fanPulseCounter = 0;

  if (mb.Coil(THERMOSTAT_HEAT_CALL_MB_COIL))
  {
    glb_thermostatStatus = "Heat call...";
    ErrorLogData_Save("Heat call...");
  }
  if (mb.Coil(THERMOSTAT_COOL_CALL_MB_COIL))
  {
    glb_thermostatStatus = "Cool call...";
    ErrorLogData_Save("Cool call...");
  }
  if (mb.Coil(THERMOSTAT_FAN_CALL_MB_COIL))
  {
    glb_thermostatStatus = "Cool call...";
    ErrorLogData_Save("Fan call...");
  }

  //check more than one call. if so then error
  bool pulseCheck = mb.Coil(THERMOSTAT_HEAT_CALL_MB_COIL) && mb.Coil(THERMOSTAT_COOL_CALL_MB_COIL) && mb.Coil(THERMOSTAT_FAN_CALL_MB_COIL);
  if (debug)
    Serial.print(" EC:");
  if (debug)
    Serial.println(pulseCheck);
  if (pulseCheck)
  {
    glb_thermostatStatus = "Error";
    ErrorLogData_Save("Thermostat Error...");
    glb_errorThermostat = 9;
    localval = 1;
  }
  if (debug)
    Serial.println(mb.Coil(THERMOSTAT_HEAT_CALL_MB_COIL));
  if (debug)
    Serial.println(mb.Coil(THERMOSTAT_COOL_CALL_MB_COIL));
  if (debug)
    Serial.println(mb.Coil(THERMOSTAT_FAN_CALL_MB_COIL));
  if (pulseCheck == 1)
  {
    //glb_thermostatStatus = "No error";
    glb_errorThermostat = 0;
    localval = 0;
  }

  mb.Hreg(THERMOSTAT_STATUS_ERR_MB_HREG, (word)localval);
  if (debug)
    Serial.print("TS:");
  if (debug)
    Serial.println(glb_thermostatStatus);
  if (debug)
    Serial.print("T:");

  endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  if (debug) Serial.println(endTimeMicros);
  mb.Hreg(THERM_DETECT_ROUTINE_TIME_MB_HREG, (word)(endTimeMicros));
  glb_TaskTimes[6] =  endTimeMicros;
}
//************************************************************************************
void IO_ControlPins()
{

  int startTimeMicros = micros();
  int endTimeMicros = 0;

  bool debug = 0;
  static int counter;
  counter++;
  if (counter == 20)
  {
    if (debug)
      Serial.println("Processing IO control...");
    counter = 0;
  }
  //read analog to get light sensor value
  glb_lightSensor = analogRead(A0);
  mb.Hreg(ANALOG_SENSOR_MB_HREG, (word)glb_lightSensor);

  //do digital writes
  mcp.digitalWrite(FAN_OVERRIDE_PIN, (bool)(mb.Coil(FAN_OVERRIDE_MB_COIL)));
  mcp.digitalWrite(FAN_CONTROL_PIN, (bool)(mb.Coil(FAN_CONTROL_MB_COIL)));
  mcp.digitalWrite(HEAT_OVERRIDE_PIN, (bool)(mb.Coil(HEAT_OVERRIDE_MB_COIL)));
  mcp.digitalWrite(HEAT_CONTROL_PIN, (bool)(mb.Coil(HEAT_CONTROL_MB_COIL)));
  mcp.digitalWrite(COOL_OVERRIDE_PIN, (bool)(mb.Coil(COOL_OVERRIDE_MB_COIL)));
  mcp.digitalWrite(COOL_CONTROL_PIN, (bool)(mb.Coil(COOL_CONTROL_MB_COIL)));

  endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  glb_TaskTimes[7] = endTimeMicros;
}
//************************************************************************************
void Wifi_CheckStatus()
{
  int startTimeMicros = micros();
  int endTimeMicros = 0;

  bool debug = 0;
  static bool displayed = true;
  static word wifiNotConnected = 0;
  glb_WiFiStatus = WiFi.status();
  if (debug)
    Serial.print("WiFI status:");
  if (debug)
    Serial.print(glb_WiFiStatus);
  if (debug)
    Serial.print(":");
  if (debug)
    Serial.println(errorCodes[glb_WiFiStatus]);
  if (debug)
    Serial.print("displayed:");
  if (debug)
    Serial.print(displayed);
  if (debug)
    Serial.print(":wifiNotConnected Counter:");
  if (debug)
    Serial.println(wifiNotConnected);

  if (glb_WiFiStatus != WL_CONNECTED)
  {
    wifiNotConnected++;
    displayed = false;
    if (debug)
      Serial.print("Disconnect counter:");
    if (debug)
      Serial.println(wifiNotConnected);
    Debug.println("Wifi disconnected. Reconnecting...");
    if (wifiNotConnected >= 1)
    {
      Serial.println("Resetting WiFi...");
      WiFi.begin(glb_SSID, glb_SSIDpassword);
      if (wifiNotConnected == 5)
        ErrorLogData_Save("Resetting WiFi...");
    }
    if (wifiNotConnected >= 10)
    {
      Serial.setDebugOutput(true);
    }

    if (wifiNotConnected >= 20)
    {
      wifiNotConnected = 0;
      Serial.println("Wifi Failed to connect! Rebooting...");
      ErrorLogData_Save("Wifi Failed to connect! Rebooting...");
      delay(1000);
      ESP.restart();
    }
  }

  if (glb_WiFiStatus == 3)
  {
    if (!displayed)
    {
      Serial.println("Wifi Reconnected !****************************************");
      ErrorLogData_Save("Wifi Reconnected");
      glb_wifiRSSI = WiFi.RSSI();
      ErrorLogData_Save("Wifi RSSI:");
      ErrorLogData_Save(String(glb_wifiRSSI));
      glb_ipAddress = WiFi.localIP();
      displayed = true;
      wifiNotConnected = 0;
      glb_wifiNotConnectedCounter++;
      Serial.setDebugOutput(false);
      mb.Hreg(WIFI_NOT_CONNECTED_MB_HREG, (word)glb_wifiNotConnectedCounter);
    }
  }

  if (glb_WiFiStatus == 0)
    glb_WiFiStatus = 10;
  mb.Hreg(WIFI_STATUS_ERR_MB_HREG, (word)glb_WiFiStatus);
  endTimeMicros = micros();
  endTimeMicros = micros() - startTimeMicros;
  glb_TaskTimes[5] = endTimeMicros;
}
//************************************************************************************
void DHT11_TempHumidity()
{
  if (glb_DHT11debugOn)
    Debug.println("Processing temperature and humity sensor...");

  static int err1 = 0;
  static int err2 = 0;
  int startTimeMicros = micros();
  int endTimeMicros = 0;
  int elaspedTimeMicros = 0;
  int tmpTemperature = 0;
  int tmpHumidity = 0;
  String tmpStatus = "";

  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tmpHumidity = dht.getHumidity();
  tmpTemperature = dht.getTemperature();
  tmpTemperature = dht.toFahrenheit(tmpTemperature);
  tmpStatus = dht.getStatusString();

  if (glb_DHT11debugOn)
    Debug.println(tmpStatus);

  if (tmpStatus == "OK")
  {
    glb_temperature = tmpTemperature;
    glb_humidity = tmpHumidity;
    glb_dhtStatusError = tmpStatus;

    mb.Hreg(TEMPERATURE_SENSOR_MB_HREG, (word)(glb_humidity));
    mb.Hreg(HUMIDITY_SENSOR_MB_HREG, (word)(glb_temperature));
    mb.Hreg(DHT_STATUS_ERR_MB_HREG, 0);
    glb_errorDHT = 0;
  }

  if (tmpStatus == "TIMEOUT")
  {
    ErrorLogData_Save("DHT 11 timeout error...");
    glb_errorDHT = 7;
    err1++;
    mb.Hreg(DHT_STATUS_ERR_MB_HREG, 1);
  }
  mb.Hreg(DHT_STATUS_ERR_TIMEOUT_COUNTER_MB_HREG, (word)err1);

  if (tmpStatus == "ERROR")
  {
    ErrorLogData_Save("DHT 11 Checksum Error...");
    glb_errorDHT = 8;
    err2++;
    mb.Hreg(DHT_STATUS_ERR_MB_HREG, 2);
  }

  mb.Hreg(DHT_STATUS_ERR_CHECKSUM_COUNTER_MB_HREG, (word)err2);
  if (glb_DHT11debugOn)
    Debug.println(glb_dhtStatusError);

  endTimeMicros = micros();
  elaspedTimeMicros = endTimeMicros - startTimeMicros;
  mb.Hreg(DHT_ROUTINE_TIME_MB_HREG, (word)(elaspedTimeMicros));
  glb_TaskTimes[2] = elaspedTimeMicros;
}
//************************************************************************************
void LCD_DrawText(int wid, int hei, String text, uint16_t textcolor, uint16_t backcolor)
{

  int startTimeMicros = micros();
  int endTimeMicros;

  bool debug = 0;

  if (debug)
    Debug.print("Text color:");
  if (debug)
    Debug.println(textcolor);
  if (debug)
    Debug.print("Backcolor:");
  if (debug)
    Debug.println(backcolor);

  if (debug)
    Debug.print("Display:");
  if (debug)
    Debug.println(text);
  tft.setCursor(wid, hei);
  tft.setTextColor(textcolor, backcolor);
  tft.setTextWrap(false);
  tft.setCursor(wid, hei);
  tft.print(text);

  endTimeMicros = micros();
  endTimeMicros = micros() - startTimeMicros;
  mb.Hreg(SCREEN_TIME_MB_HREG, (word)(endTimeMicros));
}
//************************************************************************************
void Interrupt_Detect_AC()
{
  bool debug = 0;
  static unsigned long endDetectMicros;
  unsigned long startDetectMicros = micros();
  glb_heatPulseDuration = (startDetectMicros - endDetectMicros);
  if (glb_heatPulseDuration > 10)
  {
    glb_heatPulseCounter++;
    glb_fanPulseCounter++;
    glb_coolPulseCounter++;
    if (debug)
      Serial.println(glb_heatPulseDuration);
  }

  endDetectMicros = startDetectMicros;
}
//************************************************************************************
void loop()
{
  runner.execute();
  yield();
}
//************************************************************************************
int BootDevice_Detect(void)
{
  int bootmode;
  asm(
      "movi %0, 0x60000200\n\t"
      "l32i %0, %0, 0x118\n\t"
      : "+r"(bootmode) /* Output */
      :                /* Inputs (none) */
      : "memory"       /* Clobbered */
  );
  return ((bootmode >> 0x10) & 0x7);
}
//************************************************************************************
void DataServer_Setup()
{
  int startTimeMicros = micros();
  int endTimeMicros = 0;

  Serial.println("Enabling data server...");
  DataServer.begin();

  endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  glb_TaskTimes[16] = endTimeMicros;
}
//************************************************************************************
void DataServer_Process()
{

  bool debug = 0;
  int szHreg = sizeof(glb_eepromHregCopy);
  int szCoil = sizeof(glb_eepromCoilCopy);
  uint8_t rbuf[] = {};
  byte testDataMore[szCoil + szHreg + 1];

  memcpy(testDataMore, glb_eepromHregCopy, szHreg);
  memcpy(testDataMore + szHreg, glb_eepromCoilCopy, szCoil);
  if (debug)
    Serial.println(szHreg);
  if (debug)
    Serial.println(szCoil);

  for (int lc = 0; lc < 100; lc++)
  {
    WiFiClient dataClient = DataServer.available();
    if (dataClient)
    {
      if (dataClient.available())
      {
        dataClient.read(rbuf, dataClient.available());
        if (debug)
          Serial.println(*rbuf);
        if (debug)
          Serial.print("webData Connection from: ");
        if (debug)
          Serial.println(dataClient.remoteIP());
      }
      if (dataClient.connected())
      {
        dataClient.write(testDataMore, sizeof(testDataMore));
        glb_dataServerCounter++;
        dataClient.stop();
        break;
      }
    }
    delayMicroseconds(100);
  }
}
//************************************************************************************
void DeepSleepMode()
{
  Serial.println("ESP8266 in sleep mode");
  ESP.deepSleep(10 * 1000000);
}
//************************************************************************************
void Modbus_Client_Send()
{
  int startTimeMicros = micros();
  int endTimeMicros = 0;

  WiFiClient modbusClient;
  Serial.println("Sending");
  modbusClient.connect("10.0.0.11", 5502);
  modbusClient.print(glb_temperature);
  modbusClient.print(glb_humidity);

  endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  glb_TaskTimes[17] = endTimeMicros;
}
//************************************************************************************
void WebServer_Process()
{
  //int startTimeMicros = micros();
  //int endTimeMicros = 0;

  webServer.handleClient();

  //endTimeMicros = micros();
  //endTimeMicros = endTimeMicros - startTimeMicros;
  //glb_TaskTimes[15] = endTimeMicros;

}
//************************************************************************************
void WebServer_HandleErrorLog()
{

  if (SPIFFS.exists(glb_errorLogPath))
  {
    File f = SPIFFS.open(glb_errorLogPath, "r");
    if (f.size() > 0)
    {
      int siz = f.size();
      webServer.setContentLength(siz);
      webServer.sendHeader("Content-Disposition" , "attachment; filename = errorlog.csv");
      webServer.streamFile(f, "application/octet-stream");
    }
    else
    {
      webServer.send(200, "text/plain", "File Empty");
      Serial.println("File not found error...");
    }
    f.close();
  }
}
//************************************************************************************
void WebServer_HandleDatalogUpload()
{
  webServer.serveStatic("/", SPIFFS, "/index.htm");
}
//************************************************************************************
void WebServer_HandleRoot()
{
  Serial.print("here at root");
  Serial.print("done at root");
  //webServer.send(200, "text/html", "This is a test");
}
//************************************************************************************
void handleFileCreate() {
  if (webServer.args() == 0) {
    return webServer.send(500, "text/plain", "BAD ARGS");
  }
  String path = webServer.arg(0);
  Serial.println("handleFileCreate: " + path);
  if (path == "/") {
    return webServer.send(500, "text/plain", "BAD PATH");
  }
  if (SPIFFS.exists(path)) {
    return webServer.send(500, "text/plain", "FILE EXISTS");
  }
  File file = SPIFFS.open(path, "w");
  if (file) {
    file.close();
  } else {
    return webServer.send(500, "text/plain", "CREATE FAILED");
  }
  webServer.send(200, "text/plain", "");
  path = String();
}
//************************************************************************************
void WebServer_HandleFileUpload()
{
  if (webServer.uri() != "/upload") {
    return;
  }
  HTTPUpload& upload = webServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    //DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
    if (fsUploadFile) {
      fsUploadFile.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {
      fsUploadFile.close();
    }
    Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
  }
}
//************************************************************************************
void WebServer_HandleDataLog()
{
  bool debug = 1;
  if (debug) Serial.println("web data log starting");

  if (SPIFFS.exists(glb_dataLogPath))
  {
    File f = SPIFFS.open(glb_dataLogPath, "r");
    if (debug) Serial.println(f.size());
    if (f.size() > 0)
    {
      int siz = f.size();
      webServer.setContentLength(siz);
      webServer.sendHeader("Content-Disposition", "attachment; filename=datalog.csv");
      webServer.streamFile(f, "application/octet-stream");
    }
    else
    {
      webServer.send(200, "text/plain", "File Empty");
      if (debug) Serial.println("File not found error...");
    }
    if (debug) Serial.println(f.available());
    f.close();
    if (debug) Serial.println("Closing file");
  }
}
//************************************************************************************
void WebServer_HandleText()
{
  Serial.println("TOP");
  String webData = "";
  webData += "LightSensor=";
  webData += String(glb_lightSensor);
  webData += "\n";
  webData += "Temp&Humidity Status=";
  webData += String(glb_dhtStatusError);
  webData += "\n";
  webData += "Humidity=";
  webData += String(glb_humidity);
  webData += "\n";
  webData += "Temperature=";
  webData += String(glb_temperature);
  glb_ipAddress = WiFi.localIP();
  webData += "\n";
  webData += "WiFi Status=";
  webData += String(WiFi.status());
  webData += "\n";
  webData += "IP=";
  webData += String(glb_ipAddress);
  webData += "\n";
  webData += "Heat duration=";
  webData += String(glb_heatPulseDuration);
  webData += "\n";
  webData += ("Cool duration=");
  webData += String(glb_coolPulseDuration);
  webData += "\n";
  webData += ("Fan duration=");
  webData += String(glb_fanPulseDuration);
  webData += "\n";
  webData += ("Thermostat Status=");
  webData += String(glb_thermostatStatus);
  webData += "\n";
  webData += ("Error Code:");
  webData += String(glb_BlinkErrorCode);
  webData += "\n";
  webData += "Coils=";
  for (int x = 23; x > 15; x--)
  {
    webData += String(mb.Coil(x));
  }
  webData += " ";
  for (int x = 15; x > 7; x--)
  {
    webData += String(mb.Coil(x));
  }
  webData += (" ");
  for (int x = 7; x >= 0; x--)
  {
    webData += String(mb.Coil(x));
  }
  webData += ("\n");

  webData += "PINS=";
  for (int x = 23; x > 15; x--)
  {
    webData += String(digitalRead(x));
  }
  webData += " ";
  for (int x = 15; x > 7; x--)
  {
    webData += String(digitalRead(x));
  }
  webData += " ";
  for (int x = 7; x >= 0; x--)
  {
    webData += String(digitalRead(x));
  }
  webData += "\n";

  for (int x = 1; x <= glb_maxCoilSize - 1; x++)
  {
    webData += "Modbus coil ";
    webData += String(x);
    webData += " value:";
    int val = mb.Coil(x);
    webData += String(val);
    webData += "\t\t\t";
    webData += "Modbus hReg ";
    webData += String(x);
    webData += " value:";
    val = mb.Hreg(x);
    webData += String(val);
    webData += "\n";
  }
  Serial.println("Here");
  webServer.send(200, "text/plain", webData);
  //webServer.send_P(200, "text/plain", glb_eepromHregCopy , sizeof(glb_eepromHregCopy));
  //SaveCrash.print();
}
//************************************************************************************
void WebServer_HandleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += webServer.uri();
  message += "\nMethod: ";
  message += (webServer.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += webServer.args();
  message += "\n";
  for (uint8_t i = 0; i < webServer.args(); i++)
  {
    message += " " + webServer.argName(i) + ": " + webServer.arg(i) + "\n";
  }
  webServer.send(404, "text/plain", message);
}
//************************************************************************************
void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting ESP8266");
  ErrorLogData_Save("Booting ESP8266");
  ErrorLog_Create();
  EEPROM_Setup();
  EEPROM_LoadSettings();
  Wifi_Setup();
  TimeSync_Setup();
  TelnetServer_Setup();
  taskTelnet_8.enable();
  LCD_Setup();
  Modbus_Registers_Create();
  DataLog_Create();
  I2C_Setup();
  IO_Pins_Setup();
  selftestMcp();
  DHT11_Sensor_Setup();
  OTA_Setup();
  Modbus_Registers_UpdateOnStart();
  ChipID_Acquire();
  Thermostat_ControlDisable();
  TaskScheduler_Setup();
  DataServer_Setup();
  WebServer_Setup();
  StartupPrinting_Setup();
  Tasks_Enable_Setup();
  Debug.println("Starting...");
}
//************************************************************************************
void EEPROM_LoadSettings()
{

  Serial.println("Loading settings from EEPROM..");

  int i = 0;

  //strcpy(glb_SSID, glb_defaultSSID);
  //strcpy(glb_SSIDpassword, glb_defaultSSIDpassword);
  //EEPROM.put(glb_eepromSettingsOffset + ES_SSID, glb_SSID);
  //EEPROM.put(glb_eepromSettingsOffset + ES_SSIDPASSWORD, glb_SSIDpassword);
  //EEPROM.commit();

  strcpy(glb_SSID, "");
  strcpy(glb_SSIDpassword, "");

  EEPROM.get(glb_eepromSettingsOffset + ES_SSID, glb_SSID);
  EEPROM.get(glb_eepromSettingsOffset + ES_SSIDPASSWORD, glb_SSIDpassword);

  for(i=0;glb_SSID[i] != '\0';i++)
  {
    //Serial.print(i);Serial.print(":");Serial.print(glb_SSID[i]);Serial.print(":");
    if ( (isAlphaNumeric(glb_SSID[i])) || (isPunct(glb_SSID[i])) )
    {
      //Serial.println("ok");
    }
    else
    {
      strcpy(glb_SSID, glb_defaultSSID);
      //Serial.println("Using default SSID");
      break;
    }
  }

  for(i=0;glb_SSIDpassword[i] != '\0';i++)
  {
    //Serial.print(i);Serial.print(":");Serial.print(glb_SSIDpassword[i]);Serial.print(":");
    if ( (isAlphaNumeric(glb_SSIDpassword[i])) || (isPunct(glb_SSIDpassword[i])) )
    {
      //Serial.println("ok");
    }
    else
    {
      strcpy(glb_SSIDpassword, glb_defaultSSIDpassword);
      //Serial.println("Using default PASS");
      break;
    }
  }

  Serial.print("SSID:");
  Serial.println(glb_SSID);
  Serial.print("SSID password:");
  Serial.println(glb_SSIDpassword);
}

//************************************************************************************
void Modbus_Registers_Create()
{
  //create modbus registers and copy contents from eeprom
  Serial.print("Creating Modbus Holding Registers Max size : ");
  Serial.println(glb_maxHregSize);
  word wordTmp;

  for (int x = 1; x <= glb_maxHregSize; x++)
  {
    mb.addHreg(x);
    //wordTmp = EEPROM.read(x);
    wordTmp = x;
    mb.Hreg(x, wordTmp);
    glb_eepromHregCopy[x] = x;
  }

  //create coil registers and copy contents from eepromCopy
  Serial.print("Creating Modbus Coil Registers Max size : ");
  Serial.println(glb_maxCoilSize);
  for (int x = 1; x <= glb_maxCoilSize; x++)
  {
    mb.addCoil(x);
    //tmp = EEPROM.read(x);
    mb.Coil(x, COIL_OFF);
    glb_eepromCoilCopy[x] = COIL_OFF;
  }
}
//************************************************************************************
void LCD_Setup()
{
  Serial.println("Initialize display...");
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(0);
}
//************************************************************************************
void StartupPrinting_Setup()
{

  int getBD = BootDevice_Detect();
  if (getBD == 1)
  {
    Serial.println("This sketch has just been uploaded over the UART.");
  }
  if (BootDevice_Detect() == 3)
  {
    Serial.println("This sketch started from FLASH.");
  }
  mb.Hreg(ESP_BOOT_DEVICE_MB_HREG, (word)getBD);
  Reset_Reason();
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Version:");
  Serial.println(TimestampedVersion);
  Serial.print("Chip ID:");
  Serial.println(ESP.getChipId());
  Serial.print("Sketch size:");
  Serial.println(ESP.getSketchSize());
  Serial.print("Free sketch size:");
  Serial.println(ESP.getFreeSketchSpace());
  Serial.print("Core version:");
  Serial.println(ESP.getCoreVersion());
  Serial.print("Code MD5:");
  Serial.println(ESP.getSketchMD5());
  Serial.print("CPU Frequency:");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println("mhz");
  Serial.print("Max EEPROM size:");
  Serial.println(glb_maxEEpromSize);
  Serial.print("Free RAM ");
  glb_freeHeap = ESP.getFreeHeap();
  Serial.println(glb_freeHeap);
}
//************************************************************************************
void Wifi_Setup()
{
  Serial.println("Trying to connect to WiFI...");
  //Serial.setDebugOutput(true);
  mb.config(glb_SSID, glb_SSIDpassword);
  glb_ipAddress = WiFi.localIP();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  //Serial.setDebugOutput(false);
  //Serial.begin(115200);
}
//************************************************************************************
void FileSystem_CreateHTML()
{
  File f = SPIFFS.open("index.html", "w");
  f.close();

}
//************************************************************************************
void FileSystem_ListDirectory()
{
  Dir dir = SPIFFS.openDir("/");
  while (dir.next())
  {
    Debug.print(dir.fileName());
    Debug.print(" : ");
    File f = dir.openFile("r");
    Debug.println(f.size());
  }
}
//************************************************************************************
bool FileSystem_DeleteFile(String pathname)
{
  bool val = SPIFFS.remove(pathname);
  Serial.println(val);
  if (pathname == glb_dataLogPath)
    glb_dataLogCount = 0;

  return val;
}
//************************************************************************************
void DataLog_Create()
{
  bool debug = 1;

  if (debug)
    Serial.print("Creating Log File:");

  if (SPIFFS.begin())
  {
    Serial.print("File system mounted:");
  }
  else
  {
    SPIFFS.begin();
  }

  if (SPIFFS.exists(glb_dataLogPath))
  {
    Serial.print("File exists:");
    File f = SPIFFS.open(glb_dataLogPath, "r");
    Serial.print("File size is:");
    Serial.println(f.size());
    f.close();
  }
  else
  {
    Serial.println("File not found error...");
  }
}
//************************************************************************************
void I2C_Setup()
{
  Serial.println("Setting up I2C communication");

  // i2c mode
  // used to override clock and data pins
  Wire.begin(I2C_DATA_PIN, I2C_CLOCK_PIN);
  mcp.begin(0);
}
//************************************************************************************
void EEPROM_Setup()
{
  Serial.println("Initializing EEPROM");
  EEPROM.begin(glb_maxEEpromSize);
}
//************************************************************************************
void IO_Pins_Setup()
{
  Serial.println("Initializing IO pins");

  //mcp pin io setup
  mcp.pinMode(HEAT_OVERRIDE_PIN, OUTPUT);
  mcp.pinMode(HEAT_CONTROL_PIN, OUTPUT);
  mcp.pinMode(FAN_OVERRIDE_PIN, OUTPUT);
  mcp.pinMode(FAN_CONTROL_PIN, OUTPUT);
  mcp.pinMode(COOL_OVERRIDE_PIN, OUTPUT);
  mcp.pinMode(COOL_CONTROL_PIN, OUTPUT);
  mcp.pinMode(LED, OUTPUT);

  //esp pin io setup
  pinMode(THERMOSTAT_HEAT_CALL_PIN, INPUT);
  //pinMode(THERMOSTAT_COOL_CALL_PIN, INPUT);
  //pinMode(THERMOSTAT_FAN_CALL_PIN, INPUT);
  pinMode(TEST_PIN, OUTPUT);
  pinMode(I2C_DATA_PIN, OUTPUT);
  pinMode(I2C_CLOCK_PIN, INPUT_PULLUP);

  //INTERRUPTS
  attachInterrupt(digitalPinToInterrupt(THERMOSTAT_HEAT_CALL_PIN), Interrupt_Detect_AC, RISING);
}
//************************************************************************************
void DHT11_Sensor_Setup()
{
  Serial.println("Initializing DHT11 sensor...");
  dht.setup(DHT11_DATA_PIN);
}
//************************************************************************************
void OTA_Setup()
{
  ArduinoOTA.onStart([]() {
    String type;
    glb_OTA_Started = true;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";
    DataServer.stop();
    webServer.stop();
    mb.stop();
    Debug.stop();

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()

    Serial.println("Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA Update complete");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\n", (progress / (total / 100)));
    //Serial.print("\033[0H\033[0J");
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });

  Serial.println("Initializing OTA update routines...");
  ArduinoOTA.begin();
  Serial.println("OTA Update Ready..");
}
//************************************************************************************
void Tasks_Enable_Setup()
{

  Serial.println("Enabling Tasks...");
  taskModbusReadData_1.enable();
  taskDHT11Temp_2.enable();
  taskTimeRoutine_3.enable();
  taskLED_Error_4.enable();
  taskWifiCheckStatus_5.enable();
  taskThermostatDetect_6.enable();
  taskIoControlPins_7.enable();
  taskTelnet_8.enable();
  taskLED_onEnable_9.enable();
  taskLED_OnDisable_10.enable();
  taskErrorsCodesProcess_11.enable();
  taskModbusProcess_12.enable();
  taskEEpromProcess_13.disable();
  taskWebServer_Process_15.enable();
  taskDataServer_Process_16.enable();
  taskModbusClientSend_17.disable();
  taskOTA_Update_18.enable();
  taskLogDataSave_19.enable();
  taskLCDUpdate_20.enable();

  if (taskModbusReadData_1.isEnabled())
  {
    Serial.println("Enabling task taskModbusReadData_1...");
    taskModbusReadData_1.setId(1);
  }

  if (taskDHT11Temp_2.isEnabled())
  {
    Serial.println("Enabling task taskDHT11Temp_2...");
    taskDHT11Temp_2.setId(2);
  }

  if (taskTimeRoutine_3.isEnabled())
  {
    Serial.println("Enabling task taskTimeRoutine_3...");
    taskTimeRoutine_3.setId(3);
  }

  if (taskLED_Error_4.isEnabled())
  {
    Serial.println("Enabling task taskLED_Error_4...");
    taskLED_Error_4.setId(4);
  }

  if (taskWifiCheckStatus_5.isEnabled())
  {
    Serial.println("Enabling task taskWifiCheckStatus_5...");
    taskWifiCheckStatus_5.setId(5);
  }

  if (taskThermostatDetect_6.isEnabled())
  {
    Serial.println("Enabling task taskThermostatDetect_6...");
    taskThermostatDetect_6.setId(6);
  }

  if (taskIoControlPins_7.isEnabled())
  {
    Serial.println("Enabling task taskIoControlPins_7...");
    taskIoControlPins_7.setId(7);
  }

  if (taskTelnet_8.isEnabled())
  {
    Serial.println("Enabling task taskTelnet_8...");
    taskTelnet_8.setId(8);
  }

  if (taskLED_onEnable_9.isEnabled())
  {
    Serial.println("Enabling task taskLED_onEnable_9...");
    taskLED_onEnable_9.setId(9);
  }

  if (taskLED_OnDisable_10.isEnabled())
  {
    Serial.println("Enabling task taskLED_OnDisable_10...");
    taskLED_OnDisable_10.setId(10);
  }

  if (taskErrorsCodesProcess_11.isEnabled())
  {
    Serial.println("Enabling task taskErrorsCodesProcess_11...");
    taskErrorsCodesProcess_11.setId(11);
  }

  if (taskModbusProcess_12.isEnabled())
  {
    Serial.println("Enabling task taskModbusProcess_12...");
    taskModbusProcess_12.setId(12);
  }

  if (taskEEpromProcess_13.isEnabled())
  {
    Serial.println("Enabling task taskEEpromProcess_13...");
    taskEEpromProcess_13.setId(13);
  }

  if (taskMBcoilReg11_14.isEnabled())
  {
    Serial.println("Enabling task taskMBcoilReg11_14...");
    taskMBcoilReg11_14.setId(14);
  }

  if (taskWebServer_Process_15.isEnabled())
  {
    Serial.println("Enabling task taskWebServer_Process_15...");
    taskWebServer_Process_15.setId(15);
  }

  if (taskDataServer_Process_16.isEnabled())
  {
    Serial.println("Enabling task taskDataServer_Process_16...");
    taskDataServer_Process_16.setId(16);
  }

  if (taskModbusClientSend_17.isEnabled())
  {
    Serial.println("Enabling task taskModbusClientSend_17...");
    taskModbusClientSend_17.setId(17);
  }

  if (taskOTA_Update_18.isEnabled())
  {
    Serial.println("Enabling task taskOTA_Update_18...");
    taskOTA_Update_18.setId(18);
  }

  if (taskLogDataSave_19.isEnabled())
  {
    Serial.println("Enabling task taskLogDataSave_19...");
    taskLogDataSave_19.setId(19);
  }
  if (taskLCDUpdate_20.isEnabled())
  {
    Serial.println("Enabling task taskLCDUpdate_20...");
    taskLCDUpdate_20.setId(20);
  }
}
//************************************************************************************
void TaskScheduler_Setup()
{
  Serial.println("Adding Tasks to Scheduler...");

  runner.init();
  runner.addTask(taskModbusReadData_1);
  runner.addTask(taskDHT11Temp_2);
  runner.addTask(taskTimeRoutine_3);
  runner.addTask(taskLED_Error_4);
  runner.addTask(taskWifiCheckStatus_5);
  runner.addTask(taskThermostatDetect_6);
  runner.addTask(taskIoControlPins_7);
  runner.addTask(taskTelnet_8);
  runner.addTask(taskLED_onEnable_9);
  runner.addTask(taskLED_OnDisable_10);
  runner.addTask(taskErrorsCodesProcess_11);
  runner.addTask(taskModbusProcess_12);
  runner.addTask(taskEEpromProcess_13);
  runner.addTask(taskMBcoilReg11_14);
  runner.addTask(taskWebServer_Process_15);
  runner.addTask(taskDataServer_Process_16);
  runner.addTask(taskModbusClientSend_17);
  runner.addTask(taskOTA_Update_18);
  runner.addTask(taskLogDataSave_19);
  runner.addTask(taskLCDUpdate_20);
}

//************************************************************************************
void ChipID_Acquire()
{
  unsigned long chipID = ESP.getChipId();
  word idLowWord = chipID;
  unsigned long chipIDtmp = chipID >> 16;
  word idHighWord = (word)(chipIDtmp);
  mb.Hreg(ESP_CHIP_ID_HIGH_MB_HREG, idHighWord);
  mb.Hreg(ESP_CHIP_ID_LOW_MB_HREG, idLowWord);
}
//************************************************************************************
void EEPROM_Erase()
{
  //zero out eeprom registers
  for (int x = 1; x <= glb_maxEEpromSize; x++)
  {
    EEPROM.write(x, 0);
  }
  EEPROM.commit();
}
//************************************************************************************
void Thermostat_ControlDisable()
{
  //shutdown thermostat override
  Serial.println("Shutting down Thermostat override and control...");
  mb.Coil(HEAT_OVERRIDE_MB_COIL, COIL_OFF); //COIL 1
  mb.Coil(HEAT_CONTROL_MB_COIL, COIL_OFF);  //COIL 2
  mb.Coil(COOL_OVERRIDE_MB_COIL, COIL_OFF); //COIL 3
  mb.Coil(COOL_CONTROL_MB_COIL, COIL_OFF);  //COIL 4
  mb.Coil(FAN_OVERRIDE_MB_COIL, COIL_OFF);  //COIL 5
  mb.Coil(FAN_CONTROL_MB_COIL, COIL_OFF);   //COIL 6
}
//************************************************************************************
void Modbus_Registers_UpdateOnStart()
{
  //update registers with important data
  //mb.Hreg(ESP_RESET_REASON_MB_HREG, ESP.getResetReason());
}
//************************************************************************************
void TimeSync_Setup()
{
  TimeRoutine();
  glb_BootTime = glb_TimeLong;
  Serial.print("Boot time:");
  Serial.println(glb_BootTime);
}
//************************************************************************************
void TelnetServer_Setup()
{
  Serial.println("Setting up Telnet Server...");
  Debug.begin("esp195", 1);                                   // Initiaze the telnet server - HOST_NAME is the used in MDNS.begin and set the initial Serial level
  Debug.setResetCmdEnabled(true);                             // Setup after Serial.begin
  Debug.setSerialEnabled(false);                              // All messages too send to serial too, and can be see in serial monitor
  Debug.setCallBackProjectCmds(&TelnetServer_ProcessCommand); //callback function!!!!!
}
//************************************************************************************
void WebServer_Setup()
{
  Serial.println("HTTP server started");

  webServer.on("/upload", HTTP_POST, []() {
    webServer.send(200, "text/plain", "");
  }, WebServer_HandleFileUpload);
  webServer.on("/info", WebServer_HandleText);
  webServer.on("/errorlog", WebServer_HandleErrorLog);
  webServer.on("/datalog", WebServer_HandleDataLog);
  webServer.on("/edit", HTTP_PUT, handleFileCreate);
  webServer.serveStatic("/", SPIFFS, "/index.htm");

  webServer.on("/inline", []() {
    webServer.send(200, "text/plain", "this works as well");
  });
  webServer.onNotFound(WebServer_HandleNotFound);
  webServer.begin();
}
//************************************************************************************
void ErrorLog_Create()
{
  bool debug = 1;

  if (debug)
    Serial.print("Creating ErrorLog:");

  if (SPIFFS.begin())
  {
    Serial.print("File system mounted:");
  }
  else
  {
    SPIFFS.begin();
  }
  //SPIFFS.format();

  if (SPIFFS.exists(glb_errorLogPath))
  {
    Serial.print("File exists:");
    File f = SPIFFS.open(glb_errorLogPath, "r");
    Serial.print("File size is:");
    Serial.println(f.size());
    f.close();
  }
  else
  {
    Serial.println("File not found error...");
    File f = SPIFFS.open(glb_errorLogPath, "w");
    f.close();
  }
}
//************************************************************************************
void testEspOutputPin(int pin, int value)
{
  pinMode(pin, OUTPUT);
  digitalWrite(pin, value);
}
//************************************************************************************
void testMcpOutputPin(int pin, int value)
{
  mcp.pinMode(pin, OUTPUT);
  mcp.digitalWrite(pin, value);
}
//************************************************************************************
void selftestMcp()
{
  for (int i =0; i < 8; i++){
    mcp.pinMode(i,OUTPUT);
    mcp.digitalWrite(i, HIGH);
    delay(100);
    mcp.digitalWrite(i, LOW);
    delay(100);
  }
}
//************************************************************************************
void Reset_Reason()
{

  word numResetReason = 0;
  Serial.print("Reset Reason:");
  Serial.println(ESP.getResetReason());
  ErrorLogData_Save(ESP.getResetReason());

  if (ESP.getResetReason() == "Power on")
    numResetReason = 0; /* normal startup by power on */
  if (ESP.getResetReason() == "Hardware Watchdog")
    numResetReason = 1; /* hardware watch dog reset */
  if (ESP.getResetReason() == "Exception")
    numResetReason = 2; /* hardware watch dog reset */
  if (ESP.getResetReason() == "Software Watchdog")
    numResetReason = 3; /* exception reset, GPIO status won’t change */
  if (ESP.getResetReason() == "Software/System restart")
    numResetReason = 4; /* software restart ,system_restart */
  if (ESP.getResetReason() == "Deep-Sleep Wake")
    numResetReason = 5; /* wake up from deep-sleep */
  if (ESP.getResetReason() == "External System")
    numResetReason = 6; /* external system reset */
  mb.Hreg(ESP_RESET_REASON_MB_HREG, (word)numResetReason);
  if ((numResetReason == 6) || (numResetReason == 6))
  {
    glb_resetCounter = 0;
    EEPROM.write(ES_RESETCOUNTER, glb_resetCounter);
    EEPROM.commit();
  }
  glb_resetCounter = EEPROM.read(ES_RESETCOUNTER);
  glb_resetCounter++;
  EEPROM.write(ES_RESETCOUNTER, glb_resetCounter);
  EEPROM.commit();
  mb.Hreg(ESP_RESET_COUNTER_MB_HREG, (word)glb_resetCounter);
  String tmp = "Reset counter:";
  ErrorLogData_Save(String(tmp + glb_resetCounter));
  Serial.print("Reset counter:");
  Serial.println(glb_resetCounter);
}
