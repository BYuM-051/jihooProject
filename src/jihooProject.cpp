#include <Arduino.h>
#include <EEPROM.h>

//#define _DEBUG_

/*
pinMaps

A0 ButtonPin O
A1 SoftwareSerial RX (DF Player)
A2 SoftwareSerial TX (DF Player)
A3
A4 I2C SDA
A5 I2C SCL

0
1
2 ButtonInterruptPin
3 UltraSonicEchoPin
4 UltraSonicClockPin
5 RTCResetPin
6 RTCClockPin
7 RTCDataPin
8 WeightClockPin
9 WeightDataPin
10 MotorDirectionPin
11 MotorStepPin
12 MotorENABLEPin
13 
*/

// EEPROM INDEX constants and variables below here
#define BREAKFAST_AMOUNT_IDX 0
#define LUNCH_AMOUNT_IDX 1
#define DINNER_AMOUNT_IDX 2
static uint8_t breakfastAmount;
static uint8_t lunchAmount;
static uint8_t dinnerAmount;

#define BREAKFAST_HOUR_IDX 3
#define BREAKFAST_MINUTE_IDX 4
#define LUNCH_HOUR_IDX 5
#define LUNCH_MINUTE_IDX 6
#define DINNER_HOUR_IDX 7
#define DINNER_MINUTE_IDX 8
static uint8_t breakfastHour;
static uint8_t breakfastMinute;
static uint8_t lunchHour;
static uint8_t lunchMinute;
static uint8_t dinnerHour;
static uint8_t dinnerMinute;

static uint8_t settingHour;
static uint8_t settingMin;
static uint8_t settingSec;

static bool breakfastFeedFlag = false;
static bool lunchFeedFlag = false;
static bool dinnerFeedFlag = false;

// button constants and variables below here
#define BUTTON_PIN A0
#define BUTTON_INTERRUPT_PIN 2

/*
#define BUTTON_SET 938
#define BUTTON_BACK 826
#define BUTTON_PREV 704
#define BUTTON_NEXT 615
*/

#define BUTTON_SET 980
#define BUTTON_BACK 921
#define BUTTON_PREV 846
#define BUTTON_NEXT 715

#define BUTTON_MARGIN 50

volatile bool buttonCheckFlag = false;
volatile unsigned long lastButtonClickedTime = 0;
const unsigned long DebouncingPeriod = 200;

// motor constants and variables below here
const uint8_t MotorDirPin = 10;
const uint8_t MotorStepPin = 11;
const uint8_t MotorEnPin = 12;

const int CountsPerRevolution = 200; // TODO : modify this with motor spec
const int MotorDelay = 800; // TODO : same
static int motorCount = 0;

// ultrasonic sensor constants and variables below here
#define ULTRASONIC_PERIOD 30000
const uint8_t UltraSonicTrigPin = 3;
const uint8_t UltraSonicEchoPin = 4;

const long UltrasonicNoFoodDistance = 300;
const long UltrasonicWarningDistance = 100;

// weight sensor constants and variables below here
#include "HX711.h"
const uint8_t WeightDataPin = 9;
const uint8_t WeightClockPin = 8;
const float WeightSensorCalibrationData = 2336.76f;

HX711 weightSensor;

// lcd constants and variables below here
#include "LiquidCrystal_I2C.h"
#include <Wire.h>

const unsigned long NoUseScreenChangePeriod = 3000;
static unsigned long lastUsedTime = 0;
static uint8_t lastUpdateMinute = 0;

const uint8_t SDAPin = A4;
const uint8_t SCLPin = A5;

const uint8_t LCDAddress = 0x3F;

LiquidCrystal_I2C lcd(LCDAddress, 16, 2);
/*
SET > ENTER
 CONFIG FEED LOG

SETTINGS
 TIME SCHEDULE

TIME
 HH MM SS

SCHEDULE
 BF LUNCH DINNER

BREAKFAST
TIME AMOUNT

AMOUNT
000 G
*/

bool loopFlag = true;

enum Pages
{
  Page_NOUSE,
  Page_MAIN,
  Page_SETTING,
  Page_DISPENSE,
  Page_TIME,
  Page_ScheduleMain,
  Page_ScheduleBreakfast,
  Page_ScheduleBreakfast_Amount,
  Page_ScheduleBreakfast_TIME,
  Page_ScheduleLunch,
  Page_ScheduleLunch_Amount,
  Page_ScheduleLunch_TIME,
  Page_ScheduleDinner,
  Page_ScheduleDinner_Amount,
  Page_ScheduleDinner_TIME
};
enum actions
{
  PREV,
  NEXT,
  SET,
  BACK
};
enum mainPageButtons
{MainPage_BTN_Setting, MainPage_BTN_Dispense, MainPage_BTN_Log};
enum settingPageButtons
{PAGE1_BTN_TIME, PAGE1_BTN_SCHEDULE};
enum dispensePageButtons
{PAGE2_BTN_AMOUNT};
enum timeSettingPageButtons
{PAGE3_BTN_HH, PAGE3_BTN_MM, PAGE3_BTN_SS};
enum scheduleSettingPageButtons
{PAGE4_BTN_BREAKFAST, PAGE4_BTN_LUNCH, PAGE4_BTN_DINNER};
enum schedulePage_buttons
{SCHEDULEPAGE_BTN_TIME, SCHEDULEPAGE_BTN_AMOUNT};
enum schedule_amountPage_buttons
{SCHEDULE_AMOUNTPAGE_BTN_AMOUNT};
enum schedule_timePage_buttons
{SCHEDULE_TIMEPAGE_BTN_HH, SCHEDULE_TIMEPAGE_BTN_MM};
static uint8_t nowPage = 0;
static uint8_t nowButton = 0;
enum ButtonBlinkFlags
{
  NO_BLINK,
  DISPENSE_WEIGHT,
  CURRENT_TIME_HH, CURRENT_TIME_MM, CURRENT_TIME_SS,
  SETTING_WEIGHT_BREAKFAST, SETTING_TIME_HH_BREAKFAST, SETTING_TIME_MM_BREAKFAST,
  SETTING_WEIGHT_LUNCH, SETTING_TIME_HH_LUNCH, SETTING_TIME_MM_LUNCH,
  SETTING_WEIGHT_DINNER, SETTING_TIME_HH_DINNER, SETTING_TIME_MM_DINNER
};
static uint8_t buttonBlinkFlag = 0;
static bool blinkButtonState = HIGH;
const int ButtonBlinkPeriod = 500; // modify this for blink period
static unsigned long buttonBlinkOldTime = 0;

// RTC constants and variables below here

#include "rtcManager.h"

RTCManager rtcManager;
//static Time *currentTime;

#define RTC_RESET_PIN 5
#define RTC_CLOCK_PIN 6
#define RTC_DATA_PIN 7

// ManualDispense constants and variables below here
//static unsigned int dispenseWeight = 0;
static unsigned int manualDispenseWeight = 0;
const unsigned int DispenseWeightStep = 10; // grams per one action (+/-)

// DFPlayer constants and variables below here
#include <SoftwareSerial.h>
#include "DFRobotDFPlayerMini.h"

#define SOUND_POWER_ON 1
#define SOUND_FEED_COMPLETE 2
#define SOUND_WARNING 3

const uint8_t SoftwareSerialRX = A1;
const uint8_t SoftwareSerialTX = A2;

SoftwareSerial dfPlayerSerial(SoftwareSerialRX, SoftwareSerialTX);
DFRobotDFPlayerMini dfPlayer;

const uint8_t DFPlayerConnectMaxAttempts = 10;
const unsigned long DFPlayerSerialTimeout = 500U;
const uint8_t DFPlayerVolume = 30; // 0~30 sound volume

// Function Declarations
void rotateOneRevolution(bool isClockWise);
int getWeight();
unsigned long ultraSensorCheck();
void fillFood(int weight);
void ISR_ButtonClicked();
void onButtonAction(int page, int button, int action);
void setMotorOff();
void rotateStep(int steps, bool isClockWise);

void setup() 
{  
  // Initialize Stepping Motor
  pinMode(MotorStepPin, OUTPUT);
  pinMode(MotorDirPin, OUTPUT);
  pinMode(MotorEnPin, OUTPUT);
  
  digitalWrite(MotorEnPin, HIGH);
  digitalWrite(MotorDirPin, LOW);

  // LCD Initialize and Loading Screen
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Init...");

  //
  attachInterrupt(digitalPinToInterrupt(BUTTON_INTERRUPT_PIN), ISR_ButtonClicked, RISING); // Attach Inturrupt for buttons
  pinMode(BUTTON_PIN, INPUT); // set button pin as input pin (not needed)
  

  // Initialize weight sensor
  weightSensor.begin(WeightDataPin, WeightClockPin); 
  weightSensor.set_scale(WeightSensorCalibrationData);
  weightSensor.tare();

  // Initialize ultrasonic sensor
  pinMode(UltraSonicTrigPin, OUTPUT);
  pinMode(UltraSonicEchoPin, INPUT);

  // Initialize DFPlayer Mini
  dfPlayerSerial.begin(9600);
  int dfPlayerConnectAttempt = 0;
  while(!dfPlayer.begin(dfPlayerSerial, false))
  {
    delay(10);
    if(dfPlayerConnectAttempt > DFPlayerConnectMaxAttempts)
      {break;}

    dfPlayerConnectAttempt++;
  }
  dfPlayer.setTimeOut(DFPlayerSerialTimeout);
  dfPlayer.volume(DFPlayerVolume);
  // Play PowerOn Sound
  dfPlayer.playMp3Folder(SOUND_POWER_ON);

  // Initialize RTC
  rtcManager.begin(RTC_RESET_PIN, RTC_DATA_PIN, RTC_CLOCK_PIN);

  // Get Data From EEPROM
  breakfastAmount = EEPROM.read(BREAKFAST_AMOUNT_IDX); 
  lunchAmount = EEPROM.read(LUNCH_AMOUNT_IDX);
  dinnerAmount = EEPROM.read(DINNER_AMOUNT_IDX);

  breakfastHour = EEPROM.read(BREAKFAST_HOUR_IDX);
  breakfastMinute = EEPROM.read(BREAKFAST_MINUTE_IDX);
  
  lunchHour = EEPROM.read(LUNCH_HOUR_IDX);
  lunchMinute = EEPROM.read(LUNCH_MINUTE_IDX);
  
  dinnerHour = EEPROM.read(DINNER_HOUR_IDX);
  dinnerMinute = EEPROM.read(DINNER_MINUTE_IDX);

  // Initialize MainScreen
  lcd.clear();
  lcd.print("SET > ENTER");
  lcd.setCursor(0, 1);
  lcd.print(" CONFIG FEED LOG");
  lcd.setCursor(0, 1);
  lcd.blink();
  nowPage = Page_MAIN;
  nowButton = MainPage_BTN_Setting;
  
}

void loop() 
{
  // put your main code here, to run repeatedly:
  if (!loopFlag) return;

  Time currentTime = rtcManager.now();
  unsigned long nowTime = millis();

  // Reset feed flags at midnight (00:00 ~ 00:00:59)
  if (currentTime.hour == 0 && currentTime.min == 0) 
  {
    breakfastFeedFlag = false;
    lunchFeedFlag = false;
    dinnerFeedFlag = false;
  }

  // Fill Food with Schedule
  if (!breakfastFeedFlag && currentTime.hour == breakfastHour && currentTime.min == breakfastMinute) 
  {
    fillFood(breakfastAmount);
    breakfastFeedFlag = true;
  }
  if (!lunchFeedFlag && currentTime.hour == lunchHour && currentTime.min == lunchMinute) 
  {
    fillFood(lunchAmount);
    lunchFeedFlag = true;
  }
  if (!dinnerFeedFlag && currentTime.hour == dinnerHour && currentTime.min == dinnerMinute) 
  {
    fillFood(dinnerAmount);
    dinnerFeedFlag = true;
  }

  if(buttonCheckFlag)
  {
    int buttonValue = analogRead(BUTTON_PIN);
    int action = -1;
    if(BUTTON_SET + BUTTON_MARGIN >= buttonValue && buttonValue >= BUTTON_SET - BUTTON_MARGIN )
      { action = SET; }
    else if(BUTTON_BACK + BUTTON_MARGIN >= buttonValue && buttonValue >= BUTTON_BACK - BUTTON_MARGIN )
      { action = BACK; }
    else if(BUTTON_NEXT + BUTTON_MARGIN >= buttonValue && buttonValue >= BUTTON_NEXT - BUTTON_MARGIN )
      { action = NEXT; }
    else if(BUTTON_PREV + BUTTON_MARGIN >= buttonValue && buttonValue >= BUTTON_PREV - BUTTON_MARGIN )
      { action = PREV; }
    else
    {
      Serial.print(F("ERROR-- Unknown Button Input Detected : ")); 
      Serial.println(buttonValue);
    }
    buttonCheckFlag = false;

    if(action != -1)
      {onButtonAction(nowPage, nowButton, action);}
  }

/*
enum ButtonBlinkFlags
{
  NO_BLINK,
  DISPENSE_WEIGHT,
  CURRENT_TIME_HH, CURRENT_TIME_MM, CURRENT_TIME_SS,
  SETTING_WEIGHT_BREAKFAST, SETTING_TIME_HH_BREAKFAST, SETTING_TIME_MM_BREAKFAST,
  SETTING_WEIGHT_LUNCH, SETTING_TIME_HH_LUNCH, SETTING_TIME_MM_LUNCH,
  SETTING_WEIGHT_DINNER, SETTING_TIME_HH_DINNER, SETTING_TIME_MM_DINNER
};
*/
  if(buttonBlinkFlag)
  {
    lcd.blink_off();
    if(nowTime > buttonBlinkOldTime + ButtonBlinkPeriod)
    {
      buttonBlinkOldTime = nowTime;
      switch(buttonBlinkFlag)
      {
        case DISPENSE_WEIGHT :
          lcd.setCursor(0, 1);
          if(blinkButtonState)
          {
            blinkButtonState = LOW;
            lcd.print("   ");
          }
          else
          {
            blinkButtonState = HIGH;
            if(!(manualDispenseWeight/100)){lcd.print('0');}
            if(!(manualDispenseWeight/10)){lcd.print('0');}
            lcd.print(manualDispenseWeight);
            lcd.print(" G");
          }          
          return;
        case CURRENT_TIME_HH :
          lcd.setCursor(0, 1);
          if(blinkButtonState)
          {
            blinkButtonState = LOW;
            lcd.print("  ");
          }
          else
          {
            blinkButtonState = HIGH;
            if(!(settingHour/10)){lcd.print('0');}
            lcd.print(settingHour);
          }          
          return;
        case CURRENT_TIME_MM :
          lcd.setCursor(0, 4);
          if(blinkButtonState)
          {
            blinkButtonState = LOW;
            lcd.print("  ");
          }
          else
          {
            blinkButtonState = HIGH;
            if(!(settingMin/10)){lcd.print('0');}
            lcd.print(settingMin);
          }          
          return;
        case CURRENT_TIME_SS :
          lcd.setCursor(0, 7);
          if(blinkButtonState)
          {
            blinkButtonState = LOW;
            lcd.print("  ");
          }
          else
          {
            blinkButtonState = HIGH;
            if(!(settingSec/10)){lcd.print('0');}
            lcd.print(settingSec);
          }          
          return;
        case SETTING_WEIGHT_BREAKFAST :
          lcd.setCursor(0, 1);
          if(blinkButtonState)
          {
            blinkButtonState = LOW;
            lcd.print("  ");
          }
          else
          {
            blinkButtonState = HIGH;
            if(!(breakfastAmount/100)){lcd.print('0');}
            if(!(breakfastAmount/10)){lcd.print('0');}
            lcd.print(breakfastAmount);
            lcd.print(" G");
          }     
          return;
        case SETTING_WEIGHT_LUNCH :
          lcd.setCursor(0, 1);
          if(blinkButtonState)
          {
            blinkButtonState = LOW;
            lcd.print("  ");
          }
          else
          {
            blinkButtonState = HIGH;
            if(!(lunchAmount/100)){lcd.print('0');}
            if(!(lunchAmount/10)){lcd.print('0');}
            lcd.print(lunchAmount);
            lcd.print(" G");
          }
          return;
        case SETTING_WEIGHT_DINNER :
          lcd.setCursor(0, 1);
          if(blinkButtonState)
          {
            blinkButtonState = LOW;
            lcd.print("  ");
          }
          else
          {
            blinkButtonState = HIGH;
            if(!(dinnerAmount/100)){lcd.print('0');}
            if(!(dinnerAmount/10)){lcd.print('0');}
            lcd.print(dinnerAmount);
            lcd.print(" G");
          }
          return;
          case SETTING_TIME_HH_BREAKFAST:
          {
              lcd.setCursor(0, 1);
              if (blinkButtonState)
              {
                  blinkButtonState = LOW;
                  lcd.print("  :");
              }
              else
              {
                  blinkButtonState = HIGH;
                  if (breakfastHour < 10) { lcd.print('0'); }
                  lcd.print(breakfastHour);
                  lcd.print(":");
              }
              return;
          }
          
          case SETTING_TIME_MM_BREAKFAST:
          {
              lcd.setCursor(3, 1);
              if (blinkButtonState)
              {
                  blinkButtonState = LOW;
                  lcd.print("  ");
              }
              else
              {
                  blinkButtonState = HIGH;
                  if (breakfastMinute < 10) { lcd.print('0'); }
                  lcd.print(breakfastMinute);
              }
              return;
          }
          
          case SETTING_TIME_HH_LUNCH:
          {
              lcd.setCursor(0, 1);
              if (blinkButtonState)
              {
                  blinkButtonState = LOW;
                  lcd.print("  :");
              }
              else
              {
                  blinkButtonState = HIGH;
                  if (lunchHour < 10) { lcd.print('0'); }
                  lcd.print(lunchHour);
                  lcd.print(":");
              }
              return;
          }
          
          case SETTING_TIME_MM_LUNCH:
          {
              lcd.setCursor(3, 1);
              if (blinkButtonState)
              {
                  blinkButtonState = LOW;
                  lcd.print("  ");
              }
              else
              {
                  blinkButtonState = HIGH;
                  if (lunchMinute < 10) { lcd.print('0'); }
                  lcd.print(lunchMinute);
              }
              return;
          }
          
          case SETTING_TIME_HH_DINNER:
          {
              lcd.setCursor(0, 1);
              if (blinkButtonState)
              {
                  blinkButtonState = LOW;
                  lcd.print("  :");
              }
              else
              {
                  blinkButtonState = HIGH;
                  if (dinnerHour < 10) { lcd.print('0'); }
                  lcd.print(dinnerHour);
                  lcd.print(":");
              }
              return;
          }
          
          case SETTING_TIME_MM_DINNER:
          {
              lcd.setCursor(3, 1);
              if (blinkButtonState)
              {
                  blinkButtonState = LOW;
                  lcd.print("  ");
              }
              else
              {
                  blinkButtonState = HIGH;
                  if (dinnerMinute < 10) { lcd.print('0'); }
                  lcd.print(dinnerMinute);
              }
              return;
          }
      }
    }
  }

  if(nowPage != Page_NOUSE && nowTime > lastUsedTime + NoUseScreenChangePeriod)
  {
    nowPage = Page_NOUSE;
    buttonBlinkFlag=NO_BLINK;
    lcd.clear();
    lcd.noBlink();
    lcd.print("HAVE A NICE DAY!");
    lcd.setCursor(0, 1);
    if(!(currentTime.mon / 10)) {lcd.print('0');}
    lcd.print(currentTime.mon); // MONTH
    lcd.print('/');
    if(!(currentTime.date / 10)) {lcd.print('0');}
    lcd.print(currentTime.date);
    Serial.println(currentTime.date);
    lcd.print(' ');
    switch(currentTime.dow)
    {
      case MONDAY : lcd.print("MON"); break;
      case TUESDAY : lcd.print("TUE"); break;
      case WEDNESDAY : lcd.print("WED"); break;
      case THURSDAY : lcd.print("THU"); break;
      case FRIDAY : lcd.print("FRI"); break;
      case SATURDAY : lcd.print("SAT"); break;
      case SUNDAY : lcd.print("SUN"); break;
      default : lcd.print(F("   ")); break;
    }
    lcd.print(' ');
    if(!(currentTime.hour / 10)) {lcd.print('0');}
    lcd.print(currentTime.hour);
    lcd.print(':');
    if(!(currentTime.min / 10)) {lcd.print('0');}
    lcd.print(currentTime.min);
    Serial.println(currentTime.min);
    lcd.setCursor(12, 1);
    lcd.blink();
  }

  if(nowPage == Page_NOUSE && lastUpdateMinute !=  currentTime.min)
  {
    lcd.setCursor(0, 1);
    if(!(currentTime.mon / 10)) {lcd.print('0');}
    lcd.print(currentTime.mon); // MONTH
    lcd.print('/');
    if(!(currentTime.date / 10)) {lcd.print('0');}
    lcd.print(currentTime.date);
    Serial.println(currentTime.date);
    lcd.print(' ');
    switch(currentTime.dow)
    {
      case MONDAY : lcd.print("MON"); break;
      case TUESDAY : lcd.print("TUE"); break;
      case WEDNESDAY : lcd.print("WED"); break;
      case THURSDAY : lcd.print("THU"); break;
      case FRIDAY : lcd.print("FRI"); break;
      case SATURDAY : lcd.print("SAT"); break;
      case SUNDAY : lcd.print("SUN"); break;
      default : lcd.print(F("   ")); break;
    }
    lcd.print(' ');
    if(!(currentTime.hour / 10)) {lcd.print('0');}
    lcd.print(currentTime.hour);
    lcd.print(':');
    if(!(currentTime.min / 10)) {lcd.print('0');}
    lcd.print(currentTime.min);
    Serial.println(currentTime.min);
    lcd.setCursor(12, 1);
    lcd.blink();
    lastUpdateMinute = currentTime.min;
  }
}

void ISR_ButtonClicked()
{
  volatile unsigned long nowTime = millis();
  if(lastButtonClickedTime + DebouncingPeriod > nowTime) return; // debounce
  buttonCheckFlag = true;
  lastButtonClickedTime = nowTime;
}

/*
  TODO : Button Handle And Update LCD Without Class 0327~
*/
void onButtonAction(int page, int button, int action) // nowPage, nowButton(Cursor), action(Pressed Button)
{
  buttonBlinkFlag = NO_BLINK;
  lcd.blink_off();
  lastUsedTime = millis();
  switch(page)
  {
    case Page_NOUSE :
      lcd.clear();
      lcd.print(F("SET > ENTER"));
      lcd.setCursor(0, 1);
      lcd.print(F(" CONFIG FEED LOG"));
      lcd.setCursor(0, 1);
      lcd.blink();
      nowPage = Page_MAIN;
      nowButton = MainPage_BTN_Setting;
    case Page_MAIN : // Added log button 0412
      switch(button)
      {
        case MainPage_BTN_Setting :
          switch(action)
          {
            case PREV:
              lcd.setCursor(12, 1); // done!
              lcd.blink();
              nowButton = MainPage_BTN_Log;
              return;
            case NEXT :
              lcd.setCursor(7, 1); // done
              lcd.blink();
              nowButton = MainPage_BTN_Dispense;
              return;
            case SET : // done!
              lcd.clear();
              lcd.print(F("SETTINGS"));
              lcd.setCursor(0, 1);
              lcd.print(F(" TIME SCHEDULE"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_SETTING;
              nowButton = PAGE1_BTN_TIME;
              return;
            case BACK : // done!
              lcd.blink();
              return;
          }
        case MainPage_BTN_Dispense :
          switch(action)
          {
            case PREV : // done!
              lcd.setCursor(0, 1);
              lcd.blink();
              nowButton = MainPage_BTN_Setting;
              return;
            case NEXT : // done!
              lcd.setCursor(12, 1);
              lcd.blink();
              nowButton = MainPage_BTN_Log;
              return;
            case SET : // done!
              lcd.clear();
              lcd.print(F("DISPENSE"));
              lcd.setCursor(0, 1);
              lcd.print('0');
              if(!(manualDispenseWeight/100)){lcd.print('0');}
              if(!(manualDispenseWeight/10)){lcd.print('0');}
              lcd.print(manualDispenseWeight);
              //lcd.print(" G");
              nowPage = Page_DISPENSE;
              nowButton = PAGE2_BTN_AMOUNT;
              buttonBlinkFlag = DISPENSE_WEIGHT;
              return;
            case BACK : // done!
              lcd.blink();
              return;
          }
        case MainPage_BTN_Log : 
          switch(action)
          {
            case PREV : 
              lcd.setCursor(7, 1);
              lcd.blink();
              nowButton = MainPage_BTN_Dispense;
              return;
            case NEXT :
              lcd.setCursor(0, 1);
              lcd.blink();
              nowButton = MainPage_BTN_Setting;
              return;
            case SET : // TODO : add log pafe
              return;
            case BACK : // done!
              lcd.blink();
              return;
          }
      }
    case Page_SETTING :
      switch(button)
      { 
        case PAGE1_BTN_TIME :
          switch(action)
          {
            case PREV : // done!
              lcd.setCursor(5, 1);
              lcd.blink();
              nowButton = PAGE1_BTN_SCHEDULE; 
              return;
            case NEXT : // done!
              lcd.setCursor(5, 1);
              lcd.blink();
              nowButton = PAGE1_BTN_SCHEDULE;
              return;
            case SET : // TODO : integrate RTC and go to time setting page
              Time currentTime = rtcManager.now();
              settingHour = currentTime.hour;
              settingMin = currentTime.min;
              settingSec = currentTime.sec;
              lcd.clear();
              lcd.print(F("TIME"));
              lcd.setCursor(0, 1);
              if(!(settingHour/10)){lcd.print('0');}
              lcd.print(settingHour);
              lcd.print(" ");
              if(!(settingMin/10)){lcd.print('0');}
              lcd.print(settingMin);
              lcd.print(" ");
              if(!(settingSec/10)){lcd.print('0');}
              lcd.print(settingSec);
              lcd.setCursor(0, 1);
              nowPage = Page_TIME;
              nowButton = PAGE3_BTN_HH;
              buttonBlinkFlag = CURRENT_TIME_HH;
              return;
            case BACK : // done!
              lcd.clear();
              lcd.print(F("SET > ENTER"));
              lcd.setCursor(0, 1);
              lcd.print(F(" CONFIG FEED LOG"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_MAIN;
              nowButton = MainPage_BTN_Setting;
              return;
          }
        case PAGE1_BTN_SCHEDULE :
          switch(action)
          {
            case PREV : // done!
              lcd.setCursor(0, 1);
              lcd.blink();
              nowButton = PAGE1_BTN_TIME;
              return;
            case NEXT : // done!
              lcd.setCursor(0, 1);
              lcd.blink();
              nowButton = PAGE1_BTN_TIME;
              return;
            case SET : // done!
              lcd.clear();
              lcd.print(F("SCHEDULE"));
              lcd.setCursor(0, 1);
              lcd.print(F(" BF LUNCH DINNER"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_ScheduleMain;
              nowButton = PAGE4_BTN_BREAKFAST;
              return;
            case BACK : // done!
              lcd.clear();
              lcd.print(F("SET > ENTER"));
              lcd.setCursor(0, 1);
              lcd.print(F(" CONFIG FEED LOG"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_MAIN;
              nowButton = MainPage_BTN_Setting;
              return;
          }
      }
    case Page_DISPENSE :
      switch(action)
      {
        case PREV : // done! 
          manualDispenseWeight -= DispenseWeightStep;
          buttonBlinkFlag = DISPENSE_WEIGHT;
          return;
        case NEXT : // done!
          manualDispenseWeight += DispenseWeightStep;
          buttonBlinkFlag = DISPENSE_WEIGHT;
          return;
        case SET : // done!
          lcd.clear();
          lcd.print(F("Dispensing..."));
          lcd.setCursor(0, 1);
          lcd.print(manualDispenseWeight);
          lcd.print(F(" G"));
          fillFood(manualDispenseWeight);
          lcd.clear();
          lcd.print(F("SET > ENTER"));
          lcd.setCursor(0, 1);
          lcd.print(F(" CONFIG FEED LOG"));
          lcd.setCursor(0, 1);
          lcd.blink();
          nowPage = Page_MAIN;
          nowButton = MainPage_BTN_Setting;
          return;
        case BACK : // done!
          lcd.clear();
          lcd.print(F("SET > ENTER"));
          lcd.setCursor(0, 1);
          lcd.print(F(" CONFIG FEED LOG"));
          lcd.setCursor(0, 1);
          lcd.blink();
          nowPage = Page_MAIN;
          nowButton = MainPage_BTN_Setting;
          return;
      }
    case Page_TIME : // TODO : integrate RTC and adjust time function
      switch(button)
      {
        case PAGE3_BTN_HH :
          switch(action)
          {
            case PREV :
              settingHour = settingHour == 0 ? 23 : settingHour - 1;
              return;
            case NEXT :
              settingHour = settingHour == 23 ? 0 : settingHour + 1;
              return;
            case SET : // done!
              buttonBlinkFlag = CURRENT_TIME_MM;
              nowButton = PAGE3_BTN_MM;
              return;
            case BACK : // done!
              //buttonBlinkFlag = NO_BLINK;
              lcd.clear();
              lcd.print(F("SETTINGS"));
              lcd.setCursor(0, 1);
              lcd.print(F(" TIME SCHEDULE"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_SETTING;
              nowButton = PAGE1_BTN_TIME;
              return;
          }
        case PAGE3_BTN_MM :
          switch(action)
          {
            case PREV :
              settingMin = settingMin == 0 ? 59 : settingMin - 1;
              return;
            case NEXT :
              settingMin = settingMin == 59 ? 0 : settingMin + 1;
              return;
            case SET : // done!
              buttonBlinkFlag = CURRENT_TIME_SS;
              nowButton = PAGE3_BTN_SS;
              return;
            case BACK : // done!
              buttonBlinkFlag = CURRENT_TIME_HH;
              nowButton = PAGE3_BTN_HH;
              return;
          }
        case PAGE3_BTN_SS :
          switch(action)
          {
            case PREV :
              settingSec = settingSec == 0 ? 59 : settingSec - 1;
              return;
            case NEXT :
              settingSec = settingSec == 59 ? 0 : settingSec + 1;
              return;
            case SET : // save to RTC
              rtcManager.setTime(settingHour, settingMin, settingSec);
              lcd.clear();
              lcd.print(F("SETTINGS"));
              lcd.setCursor(0, 1);
              lcd.print(F(" TIME SCHEDULE"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_SETTING;
              nowButton = PAGE1_BTN_TIME;
              return;
            case BACK : // done!
              buttonBlinkFlag = CURRENT_TIME_MM;
              nowButton = PAGE3_BTN_MM;
              return;
          }
      }
    case Page_ScheduleMain :
      switch(button)
      {
        case PAGE4_BTN_BREAKFAST :
          switch(action)
          {
            case PREV : // done!
              lcd.setCursor(9, 1); // dinner button position 9, 1
              lcd.blink();
              nowButton = PAGE4_BTN_DINNER;
              return;
            case NEXT : // done!
              lcd.setCursor(3, 1); // lunch button position 3, 1
              lcd.blink();
              nowButton = PAGE4_BTN_LUNCH;
              return;
            case SET : // done!
              lcd.clear();
              lcd.print(F("BREAKFAST"));
              lcd.setCursor(0, 1);
              lcd.print(F(" TIME AMOUNT"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_ScheduleBreakfast;
              nowButton = SCHEDULEPAGE_BTN_TIME;
              return;
            case BACK : // done!
              lcd.clear();
              lcd.print(F("SETTINGS"));
              lcd.setCursor(0, 1);
              lcd.print(F(" TIME SCHEDULE"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_SETTING;
              nowButton = PAGE1_BTN_TIME;
              return;
              
          }
        case PAGE4_BTN_LUNCH :
          switch(action)
          {
            case PREV : // done!
              lcd.setCursor(0, 1);
              lcd.blink();
              nowButton = PAGE4_BTN_BREAKFAST;
              return;
            case NEXT : // done!
              lcd.setCursor(9, 1); // dinner button position 9, 1
              lcd.blink();
              nowButton = PAGE4_BTN_DINNER;
              return;
            case SET : // done!
              lcd.clear();
              lcd.print(F("LUNCH"));
              lcd.setCursor(0, 1);
              lcd.print(F(" TIME AMOUNT"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_ScheduleLunch;
              nowButton = SCHEDULEPAGE_BTN_TIME;
              return;
            case BACK : // done!
              lcd.clear();
              lcd.print(F("SETTINGS"));
              lcd.setCursor(0, 1);
              lcd.print(F(" TIME SCHEDULE"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_SETTING;
              nowButton = PAGE1_BTN_TIME;
          }
        case PAGE4_BTN_DINNER :
          switch(action)
          {
            case PREV : // done!
              lcd.setCursor(3, 1); // lunch button position 3, 1
              lcd.blink();
              nowButton = PAGE4_BTN_LUNCH;
              return;
            case NEXT : // done!
              lcd.setCursor(0, 1);
              lcd.blink();
              nowButton = PAGE4_BTN_BREAKFAST;
              return;
            case SET : // done!
              lcd.clear();
              lcd.print(F("DINNER"));
              lcd.setCursor(0, 1);
              lcd.print(F(" TIME AMOUNT"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_ScheduleDinner;
              nowButton = SCHEDULEPAGE_BTN_TIME;
              return;
            case BACK : // done!
              lcd.clear();
              lcd.print(F("SETTINGS"));
              lcd.setCursor(0, 1);
              lcd.print(F(" TIME SCHEDULE"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_SETTING;
              nowButton = PAGE1_BTN_TIME;
          }
      }
    case Page_ScheduleBreakfast :
      switch(button)
      {
        case SCHEDULEPAGE_BTN_TIME :
          switch(action)
          {
            case PREV : // done!
              lcd.setCursor(5, 1);
              lcd.blink();
              nowButton = SCHEDULEPAGE_BTN_AMOUNT;
              return;
            case NEXT : // done!
              lcd.setCursor(5, 1);
              lcd.blink();
              nowButton = SCHEDULEPAGE_BTN_AMOUNT;
              return;
            case SET :
              lcd.clear();
              lcd.print(F("BREAKFAST TIME"));
              lcd.setCursor(0, 1);
              if(!breakfastHour / 10) {lcd.print('0');}
              lcd.print(breakfastHour);
              lcd.print(F(":"));
              if(!breakfastMinute / 10) {lcd.print('0');}
              lcd.print(breakfastMinute);
              nowPage = Page_ScheduleBreakfast_TIME;
              nowButton = SCHEDULE_TIMEPAGE_BTN_HH;
              buttonBlinkFlag = SETTING_TIME_HH_BREAKFAST;
              return;
            case BACK : // done!
              //buttonBlinkFlag = NO_BLINK;
              lcd.clear();
              lcd.print(F("SCHEDULE"));
              lcd.setCursor(0, 1);
              lcd.print(F(" BF LUNCH DINNER"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_ScheduleMain;
              nowButton = PAGE4_BTN_BREAKFAST;
              return;
          }
        case SCHEDULEPAGE_BTN_AMOUNT :
          switch(action)
          {
            case PREV : // done!
              lcd.setCursor(0, 1);
              lcd.blink();
              nowButton = SCHEDULEPAGE_BTN_TIME;
              return;
            case NEXT : // done!
              lcd.setCursor(0, 1);
              lcd.blink();
              nowButton = SCHEDULEPAGE_BTN_TIME;
              return;
            case SET : //
              lcd.clear();
              lcd.print(F("BREAKFAST AMOUNT"));
              lcd.setCursor(0, 1);
              if(!breakfastAmount / 100) {lcd.print('0');}
              if(!breakfastAmount / 10) {lcd.print('0');}
              lcd.print(breakfastAmount);
              lcd.print(" G");
              nowPage = Page_ScheduleBreakfast_Amount;
              nowButton = SCHEDULE_AMOUNTPAGE_BTN_AMOUNT;
              buttonBlinkFlag = SETTING_WEIGHT_BREAKFAST;
              return;
            case BACK : // done!
              //buttonBlinkFlag = NO_BLINK;
              lcd.clear();
              lcd.print(F("SCHEDULE"));
              lcd.setCursor(0, 1);
              lcd.print(F(" BF LUNCH DINNER"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_ScheduleMain;
              nowButton = PAGE4_BTN_BREAKFAST;
              return;
          }
      }
    case Page_ScheduleBreakfast_Amount :
      switch(button)
      {
        case SCHEDULE_AMOUNTPAGE_BTN_AMOUNT :
          switch(action)
          {
            case PREV : // done!
              breakfastAmount = breakfastAmount <= 10 ? breakfastAmount : breakfastAmount - DispenseWeightStep;
              buttonBlinkFlag = SETTING_WEIGHT_BREAKFAST;
              return;
            case NEXT : // done!
              breakfastAmount = breakfastAmount >= 990 ? breakfastAmount : breakfastAmount + DispenseWeightStep;
              buttonBlinkFlag = SETTING_WEIGHT_BREAKFAST;
              return;
            case SET : // done!
              EEPROM.write(BREAKFAST_AMOUNT_IDX, breakfastAmount);
              lcd.clear();
              lcd.print(F("SCHEDULE"));
              lcd.setCursor(0, 1);
              lcd.print(F(" BF LUNCH DINNER"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_ScheduleMain;
              nowButton = PAGE4_BTN_BREAKFAST;
              buttonBlinkFlag = NO_BLINK;
              return;
            case BACK : // done!
              breakfastAmount = EEPROM.read(BREAKFAST_AMOUNT_IDX);
              lcd.clear();
              lcd.print(F("SCHEDULE"));
              lcd.setCursor(0, 1);
              lcd.print(F(" BF LUNCH DINNER"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_ScheduleMain;
              nowButton = PAGE4_BTN_BREAKFAST;
              buttonBlinkFlag = NO_BLINK;
              return;
          }
          return;
      }

    case Page_ScheduleBreakfast_TIME :
      switch(button)
      {
        case SCHEDULE_TIMEPAGE_BTN_HH :
          switch(action)
          {
            case PREV : // done!
              breakfastHour = breakfastHour == 0 ? 23 : breakfastHour - 1;
              buttonBlinkFlag = SETTING_TIME_HH_BREAKFAST;
              return;
            case NEXT : // done!
              breakfastHour = breakfastHour == 23 ? 0 : breakfastHour + 1;
              buttonBlinkFlag = SETTING_TIME_HH_BREAKFAST;
              return;
            case SET : // done!
              //EEPROM.write(BREAKFAST_HOUR_IDX, breakfastHour); // should save after minute setting
              nowButton = SCHEDULE_TIMEPAGE_BTN_MM;
              buttonBlinkFlag = SETTING_TIME_MM_BREAKFAST;
              return;
            case BACK : // done!
              breakfastHour = EEPROM.read(BREAKFAST_HOUR_IDX);
              breakfastMinute = EEPROM.read(BREAKFAST_MINUTE_IDX);
              lcd.clear();
              lcd.print(F("SCHEDULE"));
              lcd.setCursor(0, 1);
              lcd.print(F(" BF LUNCH DINNER"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_ScheduleMain;
              nowButton = PAGE4_BTN_BREAKFAST;
              buttonBlinkFlag = NO_BLINK;
              return;
          }
        case SCHEDULE_TIMEPAGE_BTN_MM :
          switch(action)
          {
            case PREV : // done!
              breakfastMinute = breakfastMinute == 0 ? 59 : breakfastMinute - 1;
              buttonBlinkFlag = SETTING_TIME_MM_BREAKFAST;
              return;
            case NEXT : // done!
              breakfastMinute = breakfastMinute == 59 ? 0 : breakfastMinute + 1;
              buttonBlinkFlag = SETTING_TIME_MM_BREAKFAST;
              return;
            case SET : // done!
              EEPROM.write(BREAKFAST_HOUR_IDX, breakfastHour);
              EEPROM.write(BREAKFAST_MINUTE_IDX, breakfastMinute);
              lcd.clear();
              lcd.print(F("SET > ENTER"));
              lcd.setCursor(0, 1);
              lcd.print(F(" CONFIG FEED LOG"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_MAIN;
              nowButton = MainPage_BTN_Setting;
              buttonBlinkFlag = NO_BLINK;
              return;
            case BACK : // done!
              nowButton = SCHEDULE_TIMEPAGE_BTN_HH;
              buttonBlinkFlag = SETTING_TIME_HH_BREAKFAST;
              return;
          }
      }

    case Page_ScheduleLunch :
      switch(button)
      {
        case SCHEDULEPAGE_BTN_TIME :
          switch(action)
          {
            case PREV : // done!
              lcd.setCursor(5, 1);
              lcd.blink();
              nowButton = SCHEDULEPAGE_BTN_AMOUNT;
              return;
            case NEXT : // done!
              lcd.setCursor(5, 1);
              lcd.blink();
              nowButton = SCHEDULEPAGE_BTN_AMOUNT;
              return;
            case SET : // done!
              lcd.clear();
              lcd.print(F("LUNCH TIME"));
              lcd.setCursor(0, 1);
              if(!lunchHour / 10) {lcd.print('0');}
              lcd.print(lunchHour);
              lcd.print(F(":"));
              if(!lunchMinute / 10) {lcd.print('0');}
              lcd.print(lunchMinute);
              nowPage = Page_ScheduleLunch_TIME;
              nowButton = SCHEDULE_TIMEPAGE_BTN_HH;
              buttonBlinkFlag = SETTING_TIME_HH_LUNCH;
              return;
            case BACK : // done!
              //buttonBlinkFlag = NO_BLINK;
              lcd.clear();
              lcd.print(F("SCHEDULE"));
              lcd.setCursor(0, 1);
              lcd.print(F(" BF LUNCH DINNER"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_ScheduleMain;
              nowButton = PAGE4_BTN_BREAKFAST;
              return;
          }
        case SCHEDULEPAGE_BTN_AMOUNT :
          switch(action)
          {
            case PREV : // done!
              lcd.setCursor(0, 1);
              lcd.blink();
              nowButton = SCHEDULEPAGE_BTN_TIME;
              return;
            case NEXT : // done!
              lcd.setCursor(0, 1);
              lcd.blink();
              nowButton = SCHEDULEPAGE_BTN_TIME;
              return;
            case SET : // done
              lcd.clear();
              lcd.print(F("LUNCH AMOUNT"));
              lcd.setCursor(0, 1);
              if(!lunchAmount / 100) {lcd.print('0');}
              if(!lunchAmount / 10) {lcd.print('0');}
              lcd.print(lunchAmount);
              lcd.print(F(" G"));
              nowPage = Page_ScheduleLunch_Amount;
              nowButton = SCHEDULE_AMOUNTPAGE_BTN_AMOUNT;
              buttonBlinkFlag = SETTING_WEIGHT_LUNCH;
              return;
            case BACK : // done!
              //buttonBlinkFlag = NO_BLINK;
              lcd.clear();
              lcd.print(F("SCHEDULE"));
              lcd.setCursor(0, 1);
              lcd.print(F(" BF LUNCH DINNER"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_ScheduleMain;
              nowButton = PAGE4_BTN_BREAKFAST;
              return;
          }
      }
    case Page_ScheduleLunch_Amount :
      switch(button)
      {
        case SCHEDULE_AMOUNTPAGE_BTN_AMOUNT :
          switch(action)
          {
            case PREV : // done!
              lunchAmount = lunchAmount <= 10 ? lunchAmount : lunchAmount - DispenseWeightStep;
              buttonBlinkFlag = SETTING_WEIGHT_LUNCH;
              return;
            case NEXT : // done!
              lunchAmount = lunchAmount >= 990 ? lunchAmount : lunchAmount + DispenseWeightStep;
              buttonBlinkFlag = SETTING_WEIGHT_LUNCH;
              return;
            case SET : // done!
              EEPROM.write(LUNCH_AMOUNT_IDX, lunchAmount);
              lcd.clear();
              lcd.print(F("SCHEDULE"));
              lcd.setCursor(0, 1);
              lcd.print(F(" BF LUNCH DINNER"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_ScheduleMain;
              nowButton = PAGE4_BTN_BREAKFAST;
              buttonBlinkFlag = NO_BLINK;
              return;
            case BACK : // done!
              lunchAmount = EEPROM.read(LUNCH_AMOUNT_IDX);
              lcd.clear();
              lcd.print(F("SCHEDULE"));
              lcd.setCursor(0, 1);
              lcd.print(F(" BF LUNCH DINNER"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_ScheduleMain;
              nowButton = PAGE4_BTN_BREAKFAST;
              buttonBlinkFlag = NO_BLINK;
              return;
          }
          return;
      }
    case Page_ScheduleLunch_TIME : 
      switch(button)
      {
        case SCHEDULE_TIMEPAGE_BTN_HH :
          switch(action)
          {
            case PREV : // done!
              lunchHour = lunchHour == 0 ? 23 : lunchHour - 1;
              buttonBlinkFlag = SETTING_TIME_HH_LUNCH;
              return;
            case NEXT : // done!
              lunchHour = lunchHour == 23 ? 0 : lunchHour + 1;
              buttonBlinkFlag = SETTING_TIME_HH_LUNCH;
              return;
            case SET : // done!
              //EEPROM.write(BREAKFAST_HOUR_IDX, breakfastHour); // should save after minute setting
              nowButton = SCHEDULE_TIMEPAGE_BTN_MM;
              buttonBlinkFlag = SETTING_TIME_MM_LUNCH;
              return;
            case BACK : // done!
              lunchHour = EEPROM.read(LUNCH_HOUR_IDX);
              lunchMinute = EEPROM.read(LUNCH_MINUTE_IDX);
              lcd.clear();
              lcd.print(F("SCHEDULE"));
              lcd.setCursor(0, 1);
              lcd.print(F(" BF LUNCH DINNER"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_ScheduleMain;
              nowButton = PAGE4_BTN_BREAKFAST;
              buttonBlinkFlag = NO_BLINK;
              return;
          }
        case SCHEDULE_TIMEPAGE_BTN_MM :
          switch(action)
          {
            case PREV : // done!
              lunchMinute = lunchMinute == 0 ? 59 : lunchMinute - 1;
              buttonBlinkFlag = SETTING_TIME_MM_LUNCH;
              return;
            case NEXT : // done!
              lunchMinute = lunchMinute == 59 ? 0 : lunchMinute + 1;
              buttonBlinkFlag = SETTING_TIME_MM_LUNCH;
              return;
            case SET :
              EEPROM.write(LUNCH_HOUR_IDX, lunchHour);
              EEPROM.write(LUNCH_MINUTE_IDX, lunchMinute);
              lcd.clear();
              lcd.print(F("SET > ENTER"));
              lcd.setCursor(0, 1);
              lcd.print(F(" CONFIG FEED LOG"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_MAIN;
              nowButton = MainPage_BTN_Setting;
              buttonBlinkFlag = NO_BLINK;
              return;
            case BACK : // done!
              nowButton = SCHEDULE_TIMEPAGE_BTN_HH;
              buttonBlinkFlag = SETTING_TIME_HH_LUNCH;
              return;
          }
      }
    
    case Page_ScheduleDinner :
      switch(button)
      {
        case SCHEDULEPAGE_BTN_TIME :
          switch(action)
          {
            case PREV : // done!
              lcd.setCursor(5, 1);
              lcd.blink();
              nowButton = SCHEDULEPAGE_BTN_AMOUNT;
              return;
            case NEXT : // done!
              lcd.setCursor(5, 1);
              lcd.blink();
              nowButton = SCHEDULEPAGE_BTN_AMOUNT;
              return;
            case SET : // done!
              lcd.clear();
              lcd.print(F("DINNER TIME"));
              lcd.setCursor(0, 1);
              if(!dinnerHour / 10) {lcd.print('0');}
              lcd.print(dinnerHour);
              lcd.print(F(":"));
              if(!dinnerMinute / 10) {lcd.print('0');}
              lcd.print(dinnerMinute);
              nowPage = Page_ScheduleDinner_TIME;
              nowButton = SCHEDULE_TIMEPAGE_BTN_HH;
              buttonBlinkFlag = SETTING_TIME_HH_DINNER;
              return;
            case BACK : // done!
              //buttonBlinkFlag = NO_BLINK;
              lcd.clear();
              lcd.print(F("SCHEDULE"));
              lcd.setCursor(0, 1);
              lcd.print(F(" BF LUNCH DINNER"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_ScheduleMain;
              nowButton = PAGE4_BTN_BREAKFAST;
              return;
          }
        case SCHEDULEPAGE_BTN_AMOUNT :
          switch(action)
          {
            case PREV : // done!
              lcd.setCursor(0, 1);
              lcd.blink();
              nowButton = SCHEDULEPAGE_BTN_TIME;
              return;
            case NEXT : // done!
              lcd.setCursor(0, 1);
              lcd.blink();
              nowButton = SCHEDULEPAGE_BTN_TIME;
              return;
            case SET : // done!
              lcd.clear();
              lcd.print(F("DINNER AMOUNT"));
              lcd.setCursor(0, 1);
              if(!dinnerAmount / 100) {lcd.print('0');}
              if(!dinnerAmount / 10) {lcd.print('0');}
              lcd.print(dinnerAmount);
              lcd.print(F(" G"));
              nowPage = Page_ScheduleDinner_Amount;
              nowButton = SCHEDULE_AMOUNTPAGE_BTN_AMOUNT;
              buttonBlinkFlag = SETTING_WEIGHT_DINNER;
              return;
            case BACK : // done!
              //buttonBlinkFlag = NO_BLINK;
              lcd.clear();
              lcd.print(F("SCHEDULE"));
              lcd.setCursor(0, 1);
              lcd.print(F(" BF LUNCH DINNER"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_ScheduleMain;
              nowButton = PAGE4_BTN_BREAKFAST;
              return;
          }
      }
    case Page_ScheduleDinner_Amount :
      switch(button)
      {
        case SCHEDULE_AMOUNTPAGE_BTN_AMOUNT :
          switch(action)
          {
            case PREV : // done!
              dinnerAmount = dinnerAmount <= 10 ? dinnerAmount : dinnerAmount - DispenseWeightStep;
              buttonBlinkFlag = SETTING_WEIGHT_DINNER;
              return;
            case NEXT : // done!
              dinnerAmount = dinnerAmount >= 990 ? dinnerAmount : dinnerAmount + DispenseWeightStep;
              buttonBlinkFlag = SETTING_WEIGHT_DINNER;
              return;
            case SET :
              EEPROM.write(DINNER_AMOUNT_IDX, dinnerAmount);
              lcd.clear();
              lcd.print(F("SCHEDULE"));
              lcd.setCursor(0, 1);
              lcd.print(F(" BF LUNCH DINNER"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_ScheduleMain;
              nowButton = PAGE4_BTN_BREAKFAST;
              buttonBlinkFlag = NO_BLINK;
              return;
            case BACK :
              dinnerAmount = EEPROM.read(DINNER_AMOUNT_IDX);
              lcd.clear();
              lcd.print(F("SCHEDULE"));
              lcd.setCursor(0, 1);
              lcd.print(F(" BF LUNCH DINNER"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_ScheduleMain;
              nowButton = PAGE4_BTN_BREAKFAST;
              buttonBlinkFlag = NO_BLINK;
              return;
          }
          return;
      }
    case Page_ScheduleDinner_TIME :
      switch(button)
      {
        case SCHEDULE_TIMEPAGE_BTN_HH :
          switch(action)
          {
            case PREV : // done!
              dinnerHour = dinnerHour == 0 ? 23 : dinnerHour - 1;
              buttonBlinkFlag = SETTING_TIME_HH_DINNER;
              return;
            case NEXT : // done!
              dinnerHour = dinnerHour == 23 ? 0 : dinnerHour + 1;
              buttonBlinkFlag = SETTING_TIME_HH_DINNER;
              return;
            case SET : // done!
              //EEPROM.write(BREAKFAST_HOUR_IDX, breakfastHour); // should save after minute setting
              nowButton = SCHEDULE_TIMEPAGE_BTN_MM;
              buttonBlinkFlag = SETTING_TIME_MM_DINNER;
              return;
            case BACK : // done!
              dinnerHour = EEPROM.read(DINNER_HOUR_IDX);
              dinnerMinute = EEPROM.read(DINNER_MINUTE_IDX);
              lcd.clear();
              lcd.print(F("SCHEDULE"));
              lcd.setCursor(0, 1);
              lcd.print(F(" BF LUNCH DINNER"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_ScheduleMain;
              nowButton = PAGE4_BTN_BREAKFAST;
              buttonBlinkFlag = NO_BLINK;
              return;
          }
        case SCHEDULE_TIMEPAGE_BTN_MM :
          switch(action)
          {
            case PREV : // done!
              dinnerMinute = dinnerMinute == 0 ? 59 : dinnerMinute - 1;
              buttonBlinkFlag = SETTING_TIME_MM_DINNER;
              return;
            case NEXT : // done!
              dinnerMinute = dinnerMinute == 59 ? 0 : dinnerMinute + 1;
              buttonBlinkFlag = SETTING_TIME_MM_DINNER;
              return;
            case SET :
              EEPROM.write(DINNER_HOUR_IDX, dinnerHour);
              EEPROM.write(DINNER_MINUTE_IDX, dinnerMinute);
              lcd.clear();
              lcd.print(F("SET > ENTER"));
              lcd.setCursor(0, 1);
              lcd.print(F(" CONFIG FEED LOG"));
              lcd.setCursor(0, 1);
              lcd.blink();
              nowPage = Page_MAIN;
              nowButton = MainPage_BTN_Setting;
              buttonBlinkFlag = NO_BLINK;
              return;
            case BACK : // done!
              nowButton = SCHEDULE_TIMEPAGE_BTN_HH;
              buttonBlinkFlag = SETTING_TIME_HH_DINNER;
              return;
          }
      }
      return;
  }
}

/*
void fillFood(int weight, int dishes)
{
  // Rotate stepping motors and check weight sensor
  while(true)
  {
    float w1 = weightSensor.get_units(1);

    if(w1 > weight)
    {
      break;
    }

    rotateOneRevolution(true);
  }
}
*/

//TODO : adjust parameter below here
//TODO : seperate measuring weight code as getWeight
const float FillTolerance = 5.0;
const int FillMaxAttempt = 10;

void fillFood(int targetWeight) 
{
  int attempts = 0;

  weightSensor.tare(); 
  delay(200); 

  while (attempts < FillMaxAttempt) 
  {
    float weight = getWeight(); 
    lcd.clear();
    lcd.print(F("Current weight: "));
    lcd.setCursor(0, 1);
    lcd.print(weight);

    if(weight >= targetWeight - FillTolerance) 
    {
      lcd.clear();
      lcd.print("DONE!");
      delay(2000);
      break;
    }
    /*
    if(ultraSensorCheck() < UltrasonicNoFoodDistance)
    {
      lcd.clear();
      lcd.print("NOT ENOUGH FOOD!");
      lcd.setCursor(0, 1);
      lcd.print(ultraSensorCheck());
      delay(2000);
      break;
    }
    */
    rotateStep(50, true);
    attempts++;
  }

  setMotorOff();

  if(ultraSensorCheck() < UltrasonicWarningDistance)
    {dfPlayer.playMp3Folder(SOUND_WARNING);}
  else
    {dfPlayer.playMp3Folder(SOUND_FEED_COMPLETE);}
  
}

//motor control codes below here
void rotateOneRevolution(bool isClockwise = true)
{
  rotateStep(motorCount, isClockwise);
}

void rotateStep(int steps, bool isClockWise = true)
{
  digitalWrite(MotorDirPin, isClockWise ? HIGH : LOW);
  digitalWrite(MotorEnPin, LOW);
  for(int i = 0 ; i < steps ; i++)
  {
    digitalWrite(MotorStepPin, HIGH);
    delayMicroseconds(MotorDelay);
    digitalWrite(MotorStepPin, LOW);
    delayMicroseconds(MotorDelay);
  }
}

void setMotorOff()
{
  digitalWrite(MotorDirPin, LOW);
  digitalWrite(MotorStepPin, LOW);
  digitalWrite(MotorEnPin, HIGH);
}


int getWeight()
{
  float w1, w2;
  w1 = weightSensor.get_units(10);
  delay(100);
  w2 = weightSensor.get_units();
  while(abs(w1 - w2) > 10)
  {
    w1 = w2;
    w2 = weightSensor.get_units();
    delay(100);
  }

  return round(w1);
}

//TODO : call this method
unsigned long ultraSensorCheck()
{
  unsigned long duration, distance;
  digitalWrite(UltraSonicTrigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(UltraSonicTrigPin, LOW);

  duration = pulseIn(UltraSonicEchoPin, HIGH);
  distance = (((float)(340 * duration) / 1000) / 2);

  //debug code
  #ifdef _DEBUG_
  Serial.print(F("Distance : "));
  Serial.println(distance);
  #endif

  return distance;
}
