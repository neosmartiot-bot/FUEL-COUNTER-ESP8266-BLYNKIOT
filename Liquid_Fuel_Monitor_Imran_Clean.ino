/*Basic.pde - example using ModbusMaster library

  Library:: ModbusMaster
  Author:: Doc Walker <4-20ma@wvfans.net>

  Copyright:: 2009-2016 Doc Walker

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.*/

#define BLYNK_TEMPLATE_ID              "TMPL6Lzon0-Bf"
#define BLYNK_TEMPLATE_NAME        "Liquid Fuel Monitor"
#define BLYNK_FIRMWARE_VERSION             "0.1.0"
 
// #define BLYNK_PRINT Serial

#include <time.h>
#include <EEPROM.h>
#include "BlynkEdgent.h"
#include <Simpletimer.h>
#include <WidgetRTC.h>
#include <TimeLib.h>

#define Fuel1Pin  5                          // Input fuel one pin
#define Fuel2Pin  4                          // Input fuel two pin

WidgetRTC rtcc;

String currentTime;
String currentDate;

int newRefill = 0;
bool sentToday = false;           // prevents sending multiple times at same midnight
int lastPulseState = LOW;


float fuelRate = 0.0;             // price per liter
float disp1Amount = 0.0;          // total cost for dispensed fuel

bool sentThisMinute = false;
volatile bool pulseDetected = false;

uint32_t lastEdgeMs = 0;
unsigned long fuelLevel = 0;      // remaining level
unsigned long lastRefill = 0;     // last refill rec
unsigned long todayLiters = 0;    // daily counter one
unsigned long disp1Liters = 0;    // for dispensor ones

// Volatile counters
volatile unsigned long fuel1Liters = 0;
volatile unsigned long fuel2Liters = 0;

// Debounce tracking
volatile unsigned long lastInterrupt1 = 0;
volatile unsigned long lastInterrupt2 = 0;

// debounce time in ms
const unsigned long debounceMs = 200;

static uint32_t dailyHistory[90] = {0};

// uint32_t week1 = 0, week2 = 0, week3 = 0;
// uint32_t month1 = 0, month2 = 0, month3 = 0;

struct Persist 
 {uint32_t magic;         // sanity tag
  uint32_t fuelLevel;     // remaining level
  uint32_t fuel1Liters;   // total1 consumed
  uint32_t fuel2Liters;   // total2 consumed
  uint32_t todayLiters;   // total consumed
  uint32_t lastRefill;    // lastRefill
  uint32_t newRefill;     // newRefill

  uint32_t dayLiters[10];
  };

Persist persisted {0xF00DCAFE, 0, 0, 0, 0, 0, 0, {0,0,0,0,0,0,0,0,0,0}};

void SendTimer();
void eepromLoad();
void eepromSave();
void RequestTime();

// WidgetBridge bridge1(V50);  // Example: Device 1 sending data
// WidgetBridge bridge2(V51);  // Example: Device 2 sending data

void IRAM_ATTR Fuel1Pulse() 
 {// static unsigned long lastInterrupt1 = 0;
  unsigned long now = millis();
  if (now - lastInterrupt1 > debounceMs)                  // 50 ms debounce
     {// pulseDetected = true;
      fuel1Liters++;
      todayLiters++;
      if (fuelLevel > 0) 
         {fuelLevel--;}
      lastInterrupt1 = now;

      // Keep persisted struct in sync
      persisted.fuel1Liters = fuel1Liters;
      persisted.todayLiters = todayLiters;
      persisted.fuelLevel   = fuelLevel;}}

void IRAM_ATTR Fuel2Pulse() 
 {unsigned long now = millis();
  if (now - lastInterrupt2 > debounceMs)
     {fuel2Liters++;
      todayLiters++;
      if (fuelLevel > 0) 
         {fuelLevel--;}
      lastInterrupt2 = now;
      
      // Keep persisted struct in sync
      persisted.fuel2Liters = fuel2Liters;
      persisted.todayLiters = todayLiters;
      persisted.fuelLevel   = fuelLevel;}}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////    BLYNK HANDLERS    //////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

BLYNK_WRITE(V4)
 
 {newRefill = param.asInt();                     // safely read integer
  
  // String rawVal = param.asStr();              // read raw string
  // int newRefill = rawVal.toInt();             // convert string → int

  // const char* rawVal = param.asStr();
  // newRefill = atoi(rawVal);                   // Convert to integer safely
  
  persisted.fuelLevel = fuelLevel;               // save to struct
  persisted.lastRefill = persisted.newRefill;    // also store the last refill
  persisted.newRefill = newRefill;               // also store new refill level
  fuelLevel += newRefill;                        // set tank to new refill level
  
  EEPROM.put(500, persisted);
  EEPROM.commit();
  
  Serial.print("F level from Blynk: ");
  Serial.println(fuelLevel);
  Serial.print("Last refill value: ");
  Serial.println(persisted.lastRefill);
  Serial.print("New refill value: ");
  Serial.println(persisted.newRefill);

  Blynk.virtualWrite(V5, fuelLevel);
  Blynk.virtualWrite(V3, persisted.lastRefill);}

BLYNK_WRITE(V6)
 
 {// unsigned long setLiters = param.asLong();   // get value from app

  String rawVal1 = param.asStr();                // always comes as string
  unsigned long set1Liters = rawVal1.toInt();    // convert safely
  
  fuel1Liters = set1Liters;                      // overwrite current total
  persisted.fuel1Liters = fuel1Liters;           // save into struct
  
  EEPROM.put(500, persisted);
  EEPROM.commit();
  
  Blynk.virtualWrite(V6, fuel1Liters);           // always update app
  
  Serial.print("Fuel1Liters manually set to: ");
  Serial.println(fuel1Liters);}

BLYNK_WRITE(V8)
 
 {// unsigned long setLiters = param.asLong();   // get value from app

  String rawVal2 = param.asStr();                // always comes as string
  unsigned long set2Liters = rawVal2.toInt();    // convert safely
  
  fuel2Liters = set2Liters;                      // overwrite current total
  persisted.fuel2Liters = fuel2Liters;           // save into struct
  
  EEPROM.put(500, persisted);
  EEPROM.commit();
  
  Blynk.virtualWrite(V8, fuel2Liters);           // always update app
  
  Serial.print("Fuel2Liters manually set to: ");
  Serial.println(fuel2Liters);}

/*BLYNK_WRITE(V11)
 
 {// unsigned long setLiters = param.asLong();   // get value from app

  String rawVal3 = param.asStr();                // always comes as string
  unsigned long set3Liters = rawVal3.toInt();    // convert safely
  
  fuel3Liters = set3Liters;                      // overwrite current total
  persisted.fuel3Liters = fue31Liters;           // save into struct
  
  EEPROM.put(500, persisted);
  EEPROM.commit();
  
  Blynk.virtualWrite(V11, fuel3Liters);          // always update app
  
  Serial.print("Fuel3Liters manually set to: ");
  Serial.println(fuel3Liters);}*/

/*BLYNK_WRITE(V13)
 
 {// unsigned long setLiters = param.asLong();   // get value from app

  String rawVal4 = param.asStr();                // always comes as string
  unsigned long set4Liters = rawVal4.toInt();    // convert safely
  
  fuel4Liters = set4Liters;                      // overwrite current total
  persisted.fuel4Liters = fue14Liters;           // save into struct
  
  EEPROM.put(500, persisted);
  EEPROM.commit();
  
  Blynk.virtualWrite(V13, fuel4Liters);          // always update app
  
  Serial.print("Fuel4Liters manually set to: ");
  Serial.println(fuel4Liters);}*/

BLYNK_WRITE(V26)  

  {int button = param.asInt();
   
   if(button == 1)
     
     {fuelLevel=0;
      Blynk.virtualWrite(V5, fuelLevel);
      EEPROM.put(500, persisted);
      EEPROM.commit();
      Blynk.virtualWrite(V26, 0);}}
      
      /*for (int i = 0; i < 90; i++) dailyHistory[i] = 0;
      for (int i = 0; i < 9; i++) persisted.dayLiters[i] = 0;

      todayLiters = 0;

      EEPROM.put(500, persisted);
      EEPROM.put(600, dailyHistory);
      EEPROM.commit();

      Blynk.virtualWrite(V16, 0);}}*/

BLYNK_WRITE(V27)
  
  {String entered = param.asStr();
   
   if(entered == "0000")
     
     {for (int i = 0; i < 90; i++) dailyHistory[i] = 0;
      for (int i = 0; i < 9; i++) persisted.dayLiters[i] = 0;

      todayLiters = 0;

      EEPROM.put(500, persisted);
      EEPROM.put(600, dailyHistory);
      EEPROM.commit();

      Blynk.virtualWrite(V27, 0);}}

BLYNK_WRITE(V28)

 {String rawRate = param.asStr();
  fuelRate = rawRate.toFloat();         // store rate per liter
  Serial.print("Fuel rate set to: ");
  Serial.println(fuelRate);}

BLYNK_WRITE(V30) 
 
 {if (param.asInt())
     {Blynk.logEvent("wifi_reset", "Forcing hotspot mode...");
      // Reset WiFi credentials
      Blynk.disconnect();                        // disconnect from server
      WiFi.disconnect(true);                     // drop the previous WiFi
      BlynkState::set(MODE_WAIT_CONFIG);}}       // go to hotspot (provisioning) mode



BLYNK_CONNECTED()         // Every time we connect to the cloud...
  
  {rtcc.begin();
   Blynk.virtualWrite(V5,  fuelLevel);
   Blynk.virtualWrite(V4,  newRefill);
   Blynk.virtualWrite(V3,  lastRefill);
   // Blynk.virtualWrite(V7,  fuel1Liters);
   // Blynk.virtualWrite(V9,  fuel2Liters);
   Blynk.virtualWrite(V10, disp1Liters);}

//////////////////////////////////////////////////////////////////
////////////////////    TIME SYNC TO BLYNK    ////////////////////
//////////////////////////////////////////////////////////////////

void RequestTime()
 
 {Blynk.sendInternal("rtcc", "sync");
 
  currentTime = String(hour()) + ":" + minute() + ":" + second();
  currentDate = String(day()) + " " + month() + " " + year();

  Blynk.virtualWrite(V0, currentDate);
  Blynk.virtualWrite(V1, currentTime);}

//////////////////////////////////////////////////////////////////
//////////////////////    MAIN FUNCTIONS    //////////////////////
//////////////////////////////////////////////////////////////////

void setup()

  {EEPROM.begin(1024); 
   Serial.begin(9600);
   BlynkEdgent.begin();
   pinMode(Fuel1Pin, INPUT);
   pinMode(Fuel2Pin, INPUT);
   
   eepromLoad(); // load eeprom
   
   // digitalWrite(Fuel1Pin, 0);
   // digitalWrite(Fuel2Pin, 0);
   
   edgentTimer.setInterval(1000L, SendTimer);        // before it was 1000L
   edgentTimer.setInterval(1000L, RequestTime);
   edgentTimer.setInterval(1800000L, eepromSave);    // saving after half hour
   
   attachInterrupt(digitalPinToInterrupt(Fuel1Pin), Fuel1Pulse, RISING);
   attachInterrupt(digitalPinToInterrupt(Fuel2Pin), Fuel2Pulse, RISING);}
   
//////////////////////////////////////////////////////////////////////////////
/////////////////////////    PULSE DETECTION    //////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void loop()
  
  {// timer1.run();

   /*if (pulseDetected) 
      {fuelLevel--;
       fuel1Liters++;
       todayLiters++;
       pulseDetected = false;
       Serial.println("---------- Sallo Khupli Pallo : ----------");
       Blynk.virtualWrite(V5, fuelLevel);
       Blynk.virtualWrite(V6, fuel1Liters);}*/
    
   BlynkEdgent.run();}



void SendTimer()

 ////////// ---------- Edge-detected polling with debounce ---------- //////////
  
 {/*uint32_t now = millis();
 
  int state = digitalRead(Fuel1Pin);         
  
  if (state == HIGH && lastPulseState == LOW && (now - lastEdgeMs) > DEBOUNCE_MS) 
     {// if (fuelLevel > 0) 
         {fuelLevel--;                                       // decrement 1 liter
          fuel1Liters++;                                      // 1 pulse = 1 liter
          todayLiters++;                                     // daily counter
          Serial.println("---------- Sallo Khupli Pallo : ----------");
          Blynk.virtualWrite(V5, fuelLevel);                 // update to Blynk
          Blynk.virtualWrite(V6, fuel1Liters);                // update to Blynk
                                               
          lastEdgeMs = now;}}
       
  lastPulseState = state;*/

  noInterrupts();
  unsigned long f1 = fuel1Liters;
  unsigned long f2 = fuel2Liters;
  interrupts();

  disp1Liters = f1 + f2;
  disp1Amount = disp1Liters * fuelRate;   // multiply liters × rate
  
  Serial.print("Fuel1 Liters: "); Serial.println(f1);
  Serial.print("Fuel2 Liters: "); Serial.println(f2);
  Serial.print("Total Liters: "); Serial.println(disp1Liters);
  Serial.print("Total Amount: "); Serial.println(disp1Amount);
  
  Blynk.virtualWrite(V5,  fuelLevel);
  Blynk.virtualWrite(V7,  fuel1Liters);
  Blynk.virtualWrite(V9,  fuel2Liters);
  Blynk.virtualWrite(V10, disp1Liters);
  Blynk.virtualWrite(V29, disp1Amount);

  
  
  int h = hour();
  int m = minute();
  // int s = second();

  if (h == 0 && m == 0 && !sentToday)   // ------ MIDNIGHT ROLLOVER ------
     
     {for (int i = 89; i > 0; i--)     // shift history (keep last 90 days)
          {dailyHistory[i] = dailyHistory[i-1];}
      
      persisted.todayLiters = todayLiters;     
      dailyHistory[0] = todayLiters;         // store yesterday's total
      todayLiters = 0;                       // reset for new day
      sentToday = true;
     
      EEPROM.put(500, persisted);
      EEPROM.put(600, dailyHistory);
      EEPROM.commit();}

      

  if (!(h == 0 && m == 0))   // Reset flag when time is not midnight anymore
     {sentToday = false;}    // and allow the next midnight trigger

     

  // ------ DAILY TOTALS (1d, 2d, 3d) ------
      
  persisted.dayLiters[0] = dailyHistory[0];   // yesterday
  persisted.dayLiters[1] = dailyHistory[0] + dailyHistory[1];   // 2 days
  persisted.dayLiters[2] = dailyHistory[0] + dailyHistory[1] + dailyHistory[2];   // 3 days

  // ----- WEEKLY TOTALS (last 3 weeks) -----
     
  uint32_t week1 = 0, week2 = 0, week3 = 0;
  for (int i = 0; i < 7; i++)   week1 += dailyHistory[i];
  for (int i = 7; i < 14; i++)  week2 += dailyHistory[i];
  for (int i = 14; i < 21; i++) week3 += dailyHistory[i];

  // ----- MONTHLY TOTALS (last 3 months, 30-day windows) -----
     
  uint32_t month1 = 0, month2 = 0, month3 = 0;
  for (int i = 0; i < 30; i++)   month1 += dailyHistory[i];
  for (int i = 30; i < 60; i++)  month2 += dailyHistory[i];
  for (int i = 60; i < 90; i++)  month3 += dailyHistory[i];

  // --------------------- SEND TO BLYNK ----------------------
     
  Blynk.virtualWrite(V17, persisted.dayLiters[0]); // 1 day
  Blynk.virtualWrite(V18, persisted.dayLiters[1]); // 2 days
  Blynk.virtualWrite(V19, persisted.dayLiters[2]); // 3 days

  Blynk.virtualWrite(V20, week1);
  Blynk.virtualWrite(V21, week2);
  Blynk.virtualWrite(V22, week3);

  Blynk.virtualWrite(V23, month1);
  Blynk.virtualWrite(V24, month2);
  Blynk.virtualWrite(V25, month3);}

  








  /*unsigned long mow = millis() / 1000;   // seconds since boot
  int m = (mow / 60) % 60;                 // minute counter (0–59)
  int s = mow % 60;                        // seconds
  
  if (s == 0 && !sentThisMinute)   // Trigger rollover at every 00s of each minute
     
     {todayLiters++;
      
      sentThisMinute = true;

      // Store today’s total into index 0
      
      dailyHistory[8] = dailyHistory[7];
      dailyHistory[7] = dailyHistory[6];
      dailyHistory[6] = dailyHistory[5];
      dailyHistory[5] = dailyHistory[4];
      dailyHistory[4] = dailyHistory[3];
      dailyHistory[3] = dailyHistory[2];
      dailyHistory[2] = dailyHistory[1];
      dailyHistory[1] = dailyHistory[0];
      dailyHistory[0] = todayLiters;
      
      persisted.dayLiters[0] = dailyHistory[0];
      persisted.dayLiters[1] = dailyHistory[0] + dailyHistory[1];
      persisted.dayLiters[2] = dailyHistory[0] + dailyHistory[1] + dailyHistory[2];
      persisted.dayLiters[3] = dailyHistory[0] + dailyHistory[1] + dailyHistory[2] + dailyHistory[3];
      persisted.dayLiters[4] = dailyHistory[0] + dailyHistory[1] + dailyHistory[2] + dailyHistory[3] + dailyHistory[4];
      persisted.dayLiters[5] = dailyHistory[0] + dailyHistory[1] + dailyHistory[2] + dailyHistory[3] + dailyHistory[4] + dailyHistory[5];
      persisted.dayLiters[6] = dailyHistory[0] + dailyHistory[1] + dailyHistory[2] + dailyHistory[3] + dailyHistory[4] + dailyHistory[5] + dailyHistory[6];
      persisted.dayLiters[7] = dailyHistory[0] + dailyHistory[1] + dailyHistory[2] + dailyHistory[3] + dailyHistory[4] + dailyHistory[5] + dailyHistory[6] + dailyHistory[7];
      persisted.dayLiters[8] = dailyHistory[0] + dailyHistory[1] + dailyHistory[2] + dailyHistory[3] + dailyHistory[4] + dailyHistory[5] + dailyHistory[6] + dailyHistory[7] + dailyHistory[8];
    
      /*todayLiters = 0; // Reset todayLiters for next minute*/

      /*Blynk.virtualWrite(V7, persisted.dayLiters[0]);   // last 24hour
      Blynk.virtualWrite(V8, persisted.dayLiters[1]);   // last 48hour
      Blynk.virtualWrite(V9, persisted.dayLiters[2]);   // last 72hour

      Blynk.virtualWrite(V10, persisted.dayLiters[3]);   // last 1Week week1
      Blynk.virtualWrite(V11, persisted.dayLiters[4]);   // last 2Week week2
      Blynk.virtualWrite(V12, persisted.dayLiters[5]);   // last 3Week week3

      Blynk.virtualWrite(V13, persisted.dayLiters[6]);   // last 1Month month1
      Blynk.virtualWrite(V14, persisted.dayLiters[7]);   // last 2Month month2
      Blynk.virtualWrite(V15, persisted.dayLiters[8]);   // last 3Month month2

      Serial.println("================ Rollover Triggered ================");
      
      Serial.print("Last 24hour = "); Serial.println(persisted.dayLiters[0]);
      Serial.print("Last 48hour = "); Serial.println(persisted.dayLiters[1]);
      Serial.print("Last 72hour = "); Serial.println(persisted.dayLiters[2]);
      
      Serial.print("Last 1Week = "); Serial.println(persisted.dayLiters[3]);
      Serial.print("Last 2Week = "); Serial.println(persisted.dayLiters[4]);
      Serial.print("Last 3Week = "); Serial.println(persisted.dayLiters[5]);
      
      Serial.print("Last 1Month = "); Serial.println(persisted.dayLiters[6]);
      Serial.print("Last 2Month = "); Serial.println(persisted.dayLiters[7]);
      Serial.print("Last 3Month = "); Serial.println(persisted.dayLiters[8]);}*/
      
      /*if (s != 0) // Reset flag when not at second 0
           {sentThisMinute = false;}*/



  


  
  


       
//////////////////////////////////////////////////////////////////
////////////////////    EEPROM SAVE & LOAD    ////////////////////
//////////////////////////////////////////////////////////////////

void eepromSave()

 {persisted.fuelLevel = fuelLevel;
  persisted.fuel1Liters = fuel1Liters;
  persisted.fuel2Liters = fuel2Liters;
  persisted.todayLiters = todayLiters;
  
  EEPROM.put(500, persisted);
  EEPROM.put(600, dailyHistory);        // save history separately
  EEPROM.commit();
  
  Serial.println("---------- 10 Seconds EEPROM saved: ----------");
  
  Serial.print("   FuelLevel  = "); Serial.println(persisted.fuelLevel);
  Serial.print("   NewwRefill = "); Serial.println(persisted.newRefill);
  Serial.print("   LastRefill = "); Serial.println(persisted.lastRefill);
  Serial.print("   TodayLiter = "); Serial.println(persisted.todayLiters);
  Serial.print("   Fuel1Liters = "); Serial.println(persisted.fuel1Liters);
  Serial.print("   Fuel2Liters = "); Serial.println(persisted.fuel2Liters);}
  
  /*Blynk.virtualWrite(V7, persisted.dayLiters[0]);
  Blynk.virtualWrite(V8, persisted.dayLiters[0] + persisted.dayLiters[1]);
  Blynk.virtualWrite(V9, persisted.dayLiters[0] + persisted.dayLiters[1] + persisted.dayLiters[2]);}*/
 
void eepromLoad()

 {Persist tmp;
  EEPROM.get(500, tmp);
  if (tmp.magic == 0xF00DCAFE) 
     {persisted = tmp;

      fuelLevel = persisted.fuelLevel;
      newRefill = persisted.newRefill;
      lastRefill = persisted.lastRefill;
      fuel1Liters = persisted.fuel1Liters;
      fuel2Liters = persisted.fuel2Liters;
      todayLiters = persisted.todayLiters;
      
      
      
      Serial.print("EEPROM Loaded: ");
      Serial.print("Fuel level: ");     Serial.println(fuelLevel);
      Serial.print("Tody liters: ");    Serial.println(todayLiters);
      Serial.print("Fuel1 liters: ");   Serial.println(fuel1Liters);
      Serial.print("Fuel2 liters: ");   Serial.println(fuel2Liters);
      Serial.print("Last refill: ");    Serial.println(persisted.lastRefill);
      Serial.print("Neww refill: ");    Serial.println(persisted.newRefill);
      

      Blynk.virtualWrite(V5, fuelLevel);              // restore fuel level
      Blynk.virtualWrite(V3, lastRefill);             // restore last refil
      Blynk.virtualWrite(V7, fuel1Liters);            // restore fuel litre
      Blynk.virtualWrite(V9, fuel2Liters);            // restore fuel litre
     
      Blynk.virtualWrite(V17, persisted.dayLiters[0]);
      Blynk.virtualWrite(V18, persisted.dayLiters[1]);
      Blynk.virtualWrite(V19, persisted.dayLiters[2]);

      Blynk.virtualWrite(V20, persisted.dayLiters[3]);
      Blynk.virtualWrite(V21, persisted.dayLiters[4]);
      Blynk.virtualWrite(V22, persisted.dayLiters[5]);

      Blynk.virtualWrite(V23, persisted.dayLiters[6]);
      Blynk.virtualWrite(V24, persisted.dayLiters[7]);
      Blynk.virtualWrite(V25, persisted.dayLiters[8]);}
      
  else
     {Serial.println("EEPROM invalid, initializing...");
     
      persisted = {0xF00DCAFE, 0, 0, 0, 0, 0, 0, {0,0,0,0,0,0,0,0,0,0}};
      
      EEPROM.put(500, persisted);
      EEPROM.commit();}
      
  EEPROM.get(600, dailyHistory);}
