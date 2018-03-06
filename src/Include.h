#include <Arduino.h>
#include <FS.h>

#ifndef __THERMOSTAT_INCLUDE
#define __THERMOSTAT_INCLUDE

//define used in task scheduler
#define _TASK_TIMECRITICAL
#define _TASK_WDT_IDS
#define _TASK_PRIORITY
#define _TASK_STATUS_REQUEST

//dht defines
#define DHTTYPE DHT11   // DHT 11

String errorCodes[12] = {"0", "WIFI NO SSID AVAIL", "WIFI SCAN COMPLETED", "WIFI CONNECTED", \
"WIFI CONNECT FAILED", "WIFI CONNECTION_LOST", "WIFI DISCONNECTED", "DHT TIMEOUT", "DHT CHECKSUM" , \
"TSTAT DETECT ERR", "WIFI IDLE STATUS"};

//eeprom settings memory location
const int ES_SSID = 0;   //size of 32
const int ES_SSIDPASSWORD = 32 ;   //SIZE OF 32
const int ES_RESETCOUNTER = 65 ;   //SIZE OF 32


//Modbus Registers Offsets (0-9999)
const int ANALOG_SENSOR_MB_HREG = 1;
const int TEMPERATURE_SENSOR_MB_HREG = 2;
const int HUMIDITY_SENSOR_MB_HREG = 3;
const int THERMOSTAT_HEAT_CALL_PULSE_VALUE_MB_HREG = 4;
const int THERMOSTAT_COOL_CALL_PULSE_VALUE_MB_HREG = 5;
const int THERMOSTAT_FAN_CALL_PULSE_VALUE_MB_HREG = 6;
const int DHT_STATUS_ERR_TIMEOUT_COUNTER_MB_HREG = 100;
const int DHT_STATUS_ERR_CHECKSUM_COUNTER_MB_HREG = 101;
const int DHT_STATUS_ERR_MB_HREG = 102;
const int BLINK_ERROR_CODE_MB_HREG = 103;
const int WIFI_STATUS_ERR_MB_HREG = 104;
const int THERMOSTAT_STATUS_ERR_MB_HREG = 105;
const int ESP_RESET_REASON_MB_HREG = 106;
const int ESP_CHIP_ID_HIGH_MB_HREG = 107;
const int ESP_CHIP_ID_LOW_MB_HREG = 108;
const int ESP_MEMORY_MB_HREG = 109;
const int WIFI_NOT_CONNECTED_MB_HREG = 110;
const int DHT_ROUTINE_TIME_MB_HREG = 111;
const int TIME_HH_MB_HREG = 112;
const int TIME_MM_MB_HREG = 113;
const int TIME_SS_MB_HREG = 114;
const int GOOD_PACKET_COUNTER_MB_REG = 115;
const int BAD_PACKET_COUNTER_MB_REG = 116;
const int MB_ROUTINE_TIME_MB_HREG = 117;
const int PROCESS_MODBUS_TIME_MB_HREG = 118;
const int THERM_DETECT_ROUTINE_TIME_MB_HREG = 119;
const int SCREEN_TIME_MB_HREG = 120;
const int ESP_BOOT_DEVICE_MB_HREG = 121;
const int ESP_RESET_COUNTER_MB_HREG = 122;
const int MB_START_DELAY_COUNTER_MB_REG = 123;
const int MB_OVERRUN_NEG_COUNTER_MB_REG = 124;
const int MB_OVERRUN_POS_COUNTER_MB_REG = 125;
const int NO_CLIENT_COUNTER_MB_REG = 126;
const int NOT_MODBUS_PACKET_COUNTER_MB_REG = 127;
const int LARGE_FRAME_COUNTER_MB_REG = 128;
const int FAILED_WRITE_COUNTER_MB_REG = 129;
const int NTP_LOOP_TIME_MB_HREG = 130;
const int ESP_MEMORY_LOW_POINT = 131;

//modbus COILS
const int HEAT_OVERRIDE_MB_COIL = 1;
const int HEAT_CONTROL_MB_COIL = 2;
const int COOL_OVERRIDE_MB_COIL = 3;
const int COOL_CONTROL_MB_COIL = 4;
const int FAN_OVERRIDE_MB_COIL = 5;
const int FAN_CONTROL_MB_COIL = 6;
const int THERMOSTAT_HEAT_CALL_MB_COIL = 7;
const int THERMOSTAT_COOL_CALL_MB_COIL = 8;
const int THERMOSTAT_FAN_CALL_MB_COIL = 9;
const int THERMOSTAT_STATUS_MB_COIL = 10;
const int ESP_RESTART_MB_COIL = 11;
const int ESP_CLEAR_SAVECRASH_DATA = 12;

//pin mappping to io expander
const int HEAT_OVERRIDE_PIN = 0;
const int HEAT_CONTROL_PIN = 1;
const int COOL_OVERRIDE_PIN = 2;
const int COOL_CONTROL_PIN = 3;
const int FAN_OVERRIDE_PIN = 4;
const int FAN_CONTROL_PIN = 5;
const int LED = 7;

//pin mappings to esp8266
const int LIGHT_SENSOR_PIN = A0;
const int DHT11_DATA_PIN = 2;
const int I2C_CLOCK_PIN = 4;
const int I2C_DATA_PIN = 5;
const int THERMOSTAT_HEAT_CALL_PIN = 13;
const int THERMOSTAT_COOL_CALL_PIN = 15;
//const int THERMOSTAT_FAN_CALL_PIN = 10;
const int TEST_PIN = 12;

//misc variables

//GLOBAL variables
word glb_BlinkErrorCode = 1;
volatile word glb_heatPulseCounter = 0;
volatile word glb_coolPulseCounter = 0;
volatile word glb_fanPulseCounter = 0;
volatile word glb_heatPulseDuration = 0;
volatile word glb_coolPulseDuration = 0;
volatile word glb_fanPulseDuration = 0;
int glb_temperature = 0;
int glb_lightSensor = 0;
int glb_humidity = 0;
const char* glb_defaultSSID = "WALTERMARCHEWKA";         //Router login
const char* glb_defaultSSIDpassword = "Alignment67581";
char glb_SSID[32];
char glb_SSIDpassword[32];
const word glb_eepromCoilOffset = 127;
const word glb_maxEEpromSize = 2048;
const word glb_maxHregSize = 256;
const word glb_maxCoilSize = 256;
const word glb_eepromSettingsOffset = 0;
const bool COIL_OFF = false;
const bool COIL_ON = true;
word glb_errorDHT = 0;
word glb_errorThermostat = 0;
word glb_WiFiStatus = 0;
word glb_eepromHregCopy[glb_maxHregSize] = { };
bool glb_eepromCoilCopy[glb_maxCoilSize] = { };
word glb_freeHeap = 0;
int glb_lowMemory = 80000;
String glb_dhtStatusError = "";
String glb_thermostatStatus = "";
String glb_dataLogPath = "/templog.csv";
String glb_errorLogPath = "/errorlog.csv";
File glb_temperatureLog;
File glb_errorLog;
uint32_t glb_resetCounter = 0;
word glb_wifiNotConnectedCounter = 0;
bool glb_logDataDebug = false;
bool glb_DHT11debugOn = false;
bool glb_OTA_Started = false;
int glb_dataServerCounter = 0;
String glb_TimeLong = "";
String glb_TimeShort = "";
String glb_timeHour = "";
String glb_timeMin = "";
String glb_timeSec = "";
String glb_timeMonth = "";
String glb_timeDay = "";
String glb_timeYear = "";
String glb_timeWeekDay = "";
String glb_BootTime = "";
int glb_dataLogCount = 0;

//function declarations
void ChipID_Acquire();
void DataServer_Process();
void DeepSleepMode();
void DHT11_Sensor_Setup();
void DHT11_TempHumidity();
void EEPROM_Erase();
void EEPROM_Setup();
void EEPROM_Process();
void ErrorCodes_Process();
void ESP_Restart();
void DataLog_Create();
void IO_ControlPins();
void IO_Pins_Setup();
void Interrupt_Detect_AC();
void I2C_Setup();
void LED_Error();
bool LED_OnEnable();
void LED_OnDisable();
void LED_On();
void LED_Off();
void LCD_DrawText(int, int, String, uint16_t,uint16_t);
void LCD_Setup();
void EEPROM_LoadSettings();
void DataLog_Save();
void Modbus_Process();
void Modbus_ReadData();
void Modbus_Client_Send();
void Modbus_Registers_Create();
void Modbus_Registers_UpdateOnStart();
void OTA_Setup();
void OTA_Update();
void StartupPrinting_Setup();
void TaskScheduler_Setup();
void Tasks_Enable_Setup();
void TelnetServer_ProcessCommand();
void TelnetServer_Setup();
void TelnetServer_Process();
void Thermostat_ControlDisable();
void TimeRoutine();
void TimeSync_Setup();
void Thermostat_Detect();
void WebServer_Process();
void WebServer_Setup();
void WebServer_HandleText();
void WebServer_HandleNotFound();
void WebServer_HandleDataLog();
void WebServer_HandleErrorLog();
void Wifi_CheckStatus();
void Wifi_Setup();
void Reset_Reason();
void DataServer_Setup();
void ErrorLogData_Save(String);
void ErrorLog_Create();
void FileSystem_DeleteFile(String);
void LCD_Update();
void WebServer_HandleFileDialog();
void WebServer_Root();


#endif