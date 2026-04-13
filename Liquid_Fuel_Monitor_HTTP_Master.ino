/************************************************************
 * Project       : <Liquid Feul Monitor for Petrol Pump>
 * File Name     : <Liquid_Fuel_Monitor_HTTP_Master.ino>
 * Authorrrrr    : <Saad Ilyas>
 * Organization  : <Neo-Smart>
 *
 * Description   :
 * <Brief technical description of what this file/module does.
 *  Mention key functionality, interfaces, or hardware used.>
  
 * Receives pulses from remote nodes (ESP-NOW)
 * Monitors two local pulse inputs (Fuel1Pin, Fuel2Pin)
 * Aggregates counters, maintains 90-day history all pins
 * Persists state in EEPROM periodically for all fuel pins
 * Sends updates to Blynk (Blynk Edgent provisioning kept) 
 *  
 *
 * Hardware used :
 *   MCU used    : <ESP8266, NodeMCU with PC817 inputs>
 *
 *   Peripherals : <One mini buck convertor for power>
 *
 * Version       : 0.1.0
 * Dateeee       : <10-Apr-2026>
 *
 * Revision History:
 *   ----------------------------------------------------------
 *   Version        Date           Author        Description
 *   ----------------------------------------------------------
 *   v0.1.0    <10-Apr-2026>    <Saad ilyas>    Initial release
 *   v0.1.0    <date>              <name>       <Changes made>
 *   v0.2.0    <date>              <name>       <Changes made>
 *   v0.3.0    <date>              <name>       <Changes made>
 *
 * Dependencies :
 *   - <EEPROM.h>
 *   - <WiFiUdp.h>
 *   - <BlynkEdgent.h>
 *
 * Important Notes :
 *   - <Important implementation notes>
 *   This firmware is specifically meant for Master device
 *   - <Limitations or assumptions>
 *   It can support upto ten fuel dispensors at a time but
 *   the firmware will be changed respectively
 *
 * *********************************************************/

#define BLYNK_TEMPLATE_ID                   "TMPL6EnV5M0ke"
#define BLYNK_TEMPLATE_NAME             "Liquid Fuel Monitor1"
#define BLYNK_FIRMWARE_VERSION                 "0.1.0"
 
// #define BLYNK_PRINT Serial

#include <EEPROM.h>
#include <WiFiUdp.h>
#include "BlynkEdgent.h"
#include <Simpletimer.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WidgetRTC.h>
#include <TimeLib.h>

#define Fuel1Pin  5                        // Input fuel one pin
#define Fuel2Pin  4                        // Input fuel two pin

#define LOCAL_PORT 4210
#define SOFTAP_PASS "12345678"
#define SOFTAP_SSID "FuelMaster"

const char* MASTER_IP = "192.168.1.100";   // change to your master IP

WiFiUDP udp;
WidgetRTC rtcc; 
// WiFiServer credServer(8080);            // Port for sending credentials

String currentTime;
String currentDate;

int newRefill = 0;
bool sentToday = false;             // prevents sending multiple times at same midnight
int lastPulseState = LOW;

bool saveFuel1Flag = false;
bool saveFuel2Flag = false;
bool saveFuel3Flag = false;
bool saveFuel4Flag = false;
bool lastRefilFlag = false;
bool newwRefilFlag = false;

// bool CredentialsBroadcasted = false;
// bool ReconnectAfterBroadcast = false;

float fuelRatee = 0.0;              // price of liquid fuel per liter

float fuelAmountN01 = 0.0;          // total amount for nozzzzzle 01 fuel
float fuelAmountN02 = 0.0;          // total amount for nozzzzzle 02 fuel
float fuelAmountD01 = 0.0;          // total amount for dispenser 01 fuel

float fuelAmountN03 = 0.0;          // total amount for nozzzzzle 03 fuel
float fuelAmountN04 = 0.0;          // total amount for nozzzzzle 04 fuel
float fuelAmountD02 = 0.0;          // total amount for dispenser 02 fuel

// float fuelAmountN05 = 0.0;       // total amount for nozzzzzle 05 fuel
// float fuelAmountN06 = 0.0;       // total amount for nozzzzzle 06 fuel
// float fuelAmountD03 = 0.0;       // total amount for dispenser 03 fuel

// float fuelAmountN07 = 0.0;       // total amount for nozzzzzle 07 fuel
// float fuelAmountN08 = 0.0;       // total amount for nozzzzzle 08 fuel
// float fuelAmountD04 = 0.0;       // total amount for dispenser 04 fuel

// float fuelAmountN09 = 0.0;       // total amount for nozzzzzle 09 fuel
// float fuelAmountN10 = 0.0;       // total amount for nozzzzzle 10 fuel
// float fuelAmountD05 = 0.0;       // total amount for dispenser 05 fuel

// float fuelAmountN11 = 0.0;       // total amount for nozzzzzle 11 fuel
// float fuelAmountN12 = 0.0;       // total amount for nozzzzzle 12 fuel
// float fuelAmountD06 = 0.0;       // total amount for dispenser 06 fuel

// float fuelAmountN13 = 0.0;       // total amount for nozzzzzle 13 fuel
// float fuelAmountN14 = 0.0;       // total amount for nozzzzzle 14 fuel
// float fuelAmountD07 = 0.0;       // total amount for dispenser 07 fuel

// float fuelAmountN15 = 0.0;       // total amount for nozzzzzle 15 fuel
// float fuelAmountN16 = 0.0;       // total amount for nozzzzzle 16 fuel
// float fuelAmountD08 = 0.0;       // total amount for dispenser 08 fuel

// float fuelAmountN17 = 0.0;       // total amount for nozzzzzle 17 fuel
// float fuelAmountN18 = 0.0;       // total amount for nozzzzzle 18 fuel
// float fuelAmountD09 = 0.0;       // total amount for dispenser 09 fuel

// float fuelAmountN19 = 0.0;       // total amount for nozzzzzle 19 fuel
// float fuelAmountN20 = 0.0;       // total amount for nozzzzzle 20 fuel
// float fuelAmountD10 = 0.0;       // total amount for dispenser 10 fuel

float fuelAmountP01 = 0.0;          // total amount for stationnn 01 fuel

// bool sentThisMinute = false;
volatile bool fuel1Flag = false;
volatile bool fuel2Flag = false;

/////////////// ---------------- FUNCTIONING VARIABLES --------------- //////////////
            
unsigned long fuelLevel = 0;           // remaining tank lvl
unsigned long lastRefill = 0;          // last refill record
unsigned long todayLiters = 0;         // daily counter onee

unsigned long fuelLitresD01 = 0;       // for dispensor onee 
unsigned long fuelLitresD02 = 0;       // for dispensor twoo
// unsigned long fuelLitresD03 = 0;    // for dispensor thre
// unsigned long fuelLitresD04 = 0;    // for dispensor four
// unsigned long fuelLitresD05 = 0;    // for dispensor five
// unsigned long fuelLitresD06 = 0;    // for dispensor sixx
// unsigned long fuelLitresD07 = 0;    // for dispensor sevn
// unsigned long fuelLitresD08 = 0;    // for dispensor eigt
// unsigned long fuelLitresD09 = 0;    // for dispensor nine
// unsigned long fuelLitresD10 = 0;    // for dispensor tenn

unsigned long fuelLitresP01 = 0;       // for all dispensors

/////////////// ----------------- VOLATILE COUNTERS ----------------- ///////////////

volatile unsigned long fuelLitresN01 = 0;
volatile unsigned long fuelLitresN02 = 0;

volatile unsigned long fuelLitresN03 = 0;
volatile unsigned long fuelLitresN04 = 0;

// volatile unsigned long fuelLitresN05 = 0;
// volatile unsigned long fuelLitresN06 = 0;

// volatile unsigned long fuelLitresN07 = 0;
// volatile unsigned long fuelLitresN08 = 0;

// volatile unsigned long fuelLitresN09 = 0;
// volatile unsigned long fuelLitresN10 = 0;

// volatile unsigned long fuelLitresN11 = 0;
// volatile unsigned long fuelLitresN12 = 0;

// volatile unsigned long fuelLitresN13 = 0;
// volatile unsigned long fuelLitresN14 = 0;

// volatile unsigned long fuelLitresN15 = 0;
// volatile unsigned long fuelLitresN16 = 0;

// volatile unsigned long fuelLitresN17 = 0;
// volatile unsigned long fuelLitresN18 = 0;

// volatile unsigned long fuelLitresN19 = 0;
// volatile unsigned long fuelLitresN20 = 0;

/////////////// ----------------- DEBOUNCE TRACKING ----------------- ///////////////

volatile unsigned long lastInterrupt1 = 0;
volatile unsigned long lastInterrupt2 = 0;

const unsigned long debounceMs = 500;

/////////////// ----------------- HISTORY (90 DAYS) ----------------- ///////////////

static uint32_t dailyHistory[90] = {0};

struct Persist 
 {uint32_t magic;              // sanity tag
  uint32_t fuelLevel;          // remaining level
  uint32_t fuelLitresN01;      // total1 consumed
  uint32_t fuelLitresN02;      // total2 consumed
  uint32_t fuelLitresN03;      // total3 consumed
  uint32_t fuelLitresN04;      // total4 consumed
  uint32_t todayLiters;        // totalt consumed
  uint32_t lastRefill;         // tank lastRefill
  uint32_t newRefill;          // tank new Refill
  float    fuelRate;           // fule Rate

  uint32_t dayLiters[10];
  uint32_t nodeCount[10];};   
  // persisted;

Persist persisted {0xF00DCAFE, 0, 0, 0, 0, 0, 0, 0, 0, 0, {0}, {0}};

/////////////// ----------------- APPLIED FUNCTIONS ----------------- ///////////////

void SendTimer();
void loadEEPROM();
void saveEEPROM();
void RequestTime();
void UpdateBlynk(uint8_t nodeId);

/////////////////////////////////////////////////////////////////////////////////////
////////////// ----------------- INTERRUPT FUNCTIONS ----------------- //////////////
/////////////////////////////////////////////////////////////////////////////////////

void IRAM_ATTR Fuel1Pulse()
 {unsigned long now = millis();
  if (now - lastInterrupt1 > debounceMs)
     {fuel1Flag = true;
      lastInterrupt1 = now;}}

void IRAM_ATTR Fuel2Pulse() 
 {unsigned long now = millis();
  if (now - lastInterrupt2 > debounceMs)
     {fuel2Flag = true;
      lastInterrupt2 = now;}}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

/*void StartCredentialBroadcast()

 {Serial.println("Starting softAP to share credentials...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP("FuelMaster", "12345678");
  credServer.begin();

  unsigned long startTime = millis();
  
  while (millis() - startTime < 10000)   // 10 sec window
        {WiFiClient client = credServer.available();
         if (client)
            {Serial.println("Client connected! Sending credentials...");
             client.println(WiFi.SSID());
             client.println(WiFi.psk());
             client.println(BLYNK_AUTH_TOKEN);
             delay(500);
             client.stop();
             Serial.println("Credentials sent!");}
         delay(100);}

  credServer.stop();
  WiFi.softAPdisconnect(true);
  Serial.println("Stopped credential broadcast, reconnecting to app...");
  
  delay(500);
  
  ////---- NEW SECTION — return master to normal Wi-Fi & Blynk ----////
  
  Serial.println("Switching back to STA mode and reconnecting...");
  WiFi.mode(WIFI_STA);
  delay(1000);
  Serial.println("Reconnecting Wi-Fi...");
  WiFi.begin(WiFi.SSID().c_str(), WiFi.psk().c_str());

  unsigned long StartAttempt = millis();
  bool wifiConnected = false;
  
  while (WiFi.status() != WL_CONNECTED && millis() - StartAttempt < 15000)
        {delay(100);   // if (WiFi.status() == WL_CONNECTED)
         yield();}     // wifiConnected = true;
                       // break;}

         delay(100);
         yield();

  if (WiFi.status() == WL_CONNECTED)
     {Serial.println("Wi-Fi reconnected. Attempting Blynk...");
      Blynk.config(BLYNK_AUTH_TOKEN);
      Blynk.connect(5000);}

  if (!Blynk.connected())
     {Serial.println("Could not reconnect immediately, Edgent will auto-recover.");}}*/

/////////////////////////////////////////////////////////////////////////////////////
////////////////////--------------- BLYNK HANDLERS ---------------///////////////////
/////////////////////////////////////////////////////////////////////////////////////

BLYNK_WRITE(V4)
 
 {newRefill = param.asInt();                    // safely read integer
  
  // String rawVal = param.asStr();             // read raw string
  // int newRefill = rawVal.toInt();            // convert string → int

  // const char* rawVal = param.asStr();
  // newRefill = atoi(rawVal);                  // Convert to integer safely
  
  persisted.fuelLevel = fuelLevel;              // save to struct
  persisted.lastRefill = persisted.newRefill;   // also store the last refill
  lastRefill = persisted.lastRefill;            // also update the last refill
  persisted.newRefill = newRefill;              // also store new refill level
  fuelLevel += newRefill;                       // set tank to new refill level
  
  EEPROM.put(500, persisted);
  EEPROM.commit();
  
  Serial.print("F level from Blynk: ");
  Serial.println(fuelLevel);
  Serial.print("Last refill value: ");
  Serial.println(persisted.lastRefill);
  Serial.print("New refill value: ");
  Serial.println(persisted.newRefill);}

  // Blynk.virtualWrite(V5, fuelLevel);
  // Blynk.virtualWrite(V3, persisted.lastRefill);}

BLYNK_WRITE(V6)   // Reset Tank button

  {int button = param.asInt();
   
   if(button == 1)
     
     {fuelLevel=0;
      persisted.fuelLevel = fuelLevel;
      EEPROM.put(500, persisted);
      EEPROM.commit();}}

      // Blynk.virtualWrite(V5, fuelLevel);
      // Blynk.virtualWrite(V6, 0);}}

BLYNK_WRITE(V7)

 {String rawRate = param.asStr();
  fuelRatee = rawRate.toFloat();         // store rate per liter
  // fuelRatee = rawRate.toInt();
  persisted.fuelRate = fuelRatee;
  
  EEPROM.put(500, persisted);
  EEPROM.commit();
  
  // Blynk.virtualWrite(V7, fuelRatee);
  
  Serial.print("Fuel rate set to: ");
  Serial.println(fuelRatee);}

BLYNK_WRITE(V8)
  
  {String entered = param.asStr();
   
   if(entered == "0000")
     
     {for (int i = 0; i < 90; i++) dailyHistory[i] = 0;
      for (int i = 0; i < 9; i++) persisted.dayLiters[i] = 0;

      todayLiters = 0;
      persisted.todayLiters = todayLiters;
      
      EEPROM.put(500, persisted);
      EEPROM.put(800, dailyHistory);
      EEPROM.commit();}}

      // Blynk.virtualWrite(V8, 0);}}

BLYNK_WRITE(V9)
 
 {if (param.asInt())
     {Blynk.logEvent("wifi_reset", "Forcing hotspot mode...");
      // Reset WiFi credentials
      Blynk.disconnect();                        // disconnect from server
      WiFi.disconnect(true);                     // drop the previous WiFi
      BlynkState::set(MODE_WAIT_CONFIG);}}       // go to hotspot (provisioning)

BLYNK_WRITE(V10)
 
 {// unsigned long setLiters = param.asLong();   // get value from app

  String rawVal1 = param.asStr();                // always comes as string
  unsigned long set1Liters = rawVal1.toInt();    // convert safely
  
  fuelLitresN01 = set1Liters;                    // overwrite current total
  persisted.fuelLitresN01 = fuelLitresN01;       // save into struct

  EEPROM.put(500, persisted);
  EEPROM.commit();
      
  // mark that we need to save to EEPROM later
  // saveFuel1Flag = true;
  
  // Blynk.virtualWrite(V11, fuelLitresN01);     // always update app
  
  Serial.print("fuelLitresN01 manually set to: ");
  Serial.println(fuelLitresN01);}

BLYNK_WRITE(V12)
 
 {// unsigned long setLiters = param.asLong();   // get value from app

  String rawVal2 = param.asStr();                // always comes as string
  unsigned long set2Liters = rawVal2.toInt();    // convert safely
  
  fuelLitresN02 = set2Liters;                    // overwrite current total
  persisted.fuelLitresN02 = fuelLitresN02;       // save into struct

  EEPROM.put(500, persisted);
  EEPROM.commit();
      
  // mark that we need to save to EEPROM later
  // saveFuel2Flag = true;
  
  // Blynk.virtualWrite(V13, fuelLitresN02);     // always update app
  
  Serial.print("fuelLitresN02 manually set to: ");
  Serial.println(fuelLitresN02);}

BLYNK_WRITE(V15)
 
 {// unsigned long setLiters = param.asLong();   // get value from app

  String rawVal3 = param.asStr();                // always comes as string
  unsigned long set3Liters = rawVal3.toInt();    // convert safely
  
  fuelLitresN03 = set3Liters;                      // overwrite current total
  persisted.fuelLitresN03 = fuelLitresN03;           // save into struct

  EEPROM.put(500, persisted);
  EEPROM.commit();
      
  // mark that we need to save to EEPROM later
  // saveFuel3Flag = true;
  
  // Blynk.virtualWrite(V16, fuelLitresN03);       // always update app
  
  Serial.print("fuelLitresN03 manually set to: ");
  Serial.println(fuelLitresN03);}

BLYNK_WRITE(V17)
 
 {// unsigned long setLiters = param.asLong();   // get value from app

  String rawVal4 = param.asStr();                // always comes as string
  unsigned long set4Liters = rawVal4.toInt();    // convert safely
  
  fuelLitresN04 = set4Liters;                    // overwrite current total
  persisted.fuelLitresN04 = fuelLitresN04;       // save into struct

  EEPROM.put(500, persisted);
  EEPROM.commit();
      
  // mark that we need to save to EEPROM later
  // saveFuel4Flag = true;
  
  // Blynk.virtualWrite(V18, fuelLitresN04);       // always update app
  
  Serial.print("fuelLitresN04 manually set to: ");
  Serial.println(fuelLitresN04);}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

BLYNK_CONNECTED()    //////////// Every time we connect to the cloud... /////////////
  
  {rtcc.begin();
   Blynk.virtualWrite(V5,  fuelLevel);
   Blynk.virtualWrite(V4,  newRefill);
   Blynk.virtualWrite(V3,  lastRefill);
   // Blynk.virtualWrite(V11,  fuelLitresN01);
   // Blynk.virtualWrite(V13,  fuelLitresN02);
   Blynk.virtualWrite(V14, fuelLitresD01);
   // Blynk.virtualWrite(V16,  fuelLitresN03);
   // Blynk.virtualWrite(V18,  fuelLitresN04);
   Blynk.virtualWrite(V19, fuelLitresD02);
   
   Blynk.virtualWrite(V60, fuelLitresP01);
   
   Blynk.virtualWrite(V91, fuelAmountP01);}

/////////////////////////////////////////////////////////////////////////////////////
//////////////////--------------- TIME SYNC TO BLYNK ---------------/////////////////
/////////////////////////////////////////////////////////////////////////////////////

void RequestTime()
 
 {Blynk.sendInternal("rtcc", "sync");
 
  currentTime = String(hour()) + ":" + minute() + ":" + second();
  currentDate = String(day()) + " " + month() + " " + year();

  Blynk.virtualWrite(V0, currentDate);
  Blynk.virtualWrite(V1, currentTime);}

/////////////////////////////////////////////////////////////////////////////////////
////////////////////--------------- SETUP FUNCTION ---------------///////////////////
/////////////////////////////////////////////////////////////////////////////////////

void setup()

  {EEPROM.begin(1024); 
   Serial.begin(115200);

   WiFi.mode(WIFI_AP_STA);                    // AP + STA mode, Start SoftAP (no external router)
   WiFi.softAP(SOFTAP_SSID, SOFTAP_PASS);
   Serial.printf("SoftAP started: %s, IP: %s\n", SOFTAP_SSID, WiFi.softAPIP().toString().c_str());

   udp.begin(LOCAL_PORT);                     // Start UDP server
   Serial.printf("UDP server listening on port %d\n", LOCAL_PORT);
   Serial.printf("UDP Server started at %s:%d\n", WiFi.localIP().toString().c_str(), LOCAL_PORT);
   
   delay(500);

   loadEEPROM(); // load eeprom
  
   pinMode(Fuel1Pin, INPUT_PULLUP);
   pinMode(Fuel2Pin, INPUT_PULLUP);

   BlynkEdgent.begin();
   
   edgentTimer.setInterval(1000L, SendTimer);        // before it was 1000L
   edgentTimer.setInterval(60000L, RequestTime);     // running once a minute
   edgentTimer.setInterval(1800000L, saveEEPROM);    // saving after half hour
   
   attachInterrupt(digitalPinToInterrupt(Fuel1Pin), Fuel1Pulse, RISING);
   attachInterrupt(digitalPinToInterrupt(Fuel2Pin), Fuel2Pulse, RISING);}

/////////////////////////////////////////////////////////////////////////////////////
////////////////////---------------- LOOP FUNCTION ---------------///////////////////
/////////////////////////////////////////////////////////////////////////////////////

void loop()
  
 {BlynkEdgent.run();

  /*if (fuel1Flag)
     {noInterrupts();
      fuelLitresN01++;
      todayLiters++;
      if (fuelLevel>0)
         {fuelLevel--;
          persisted.fuelLitresN01 = fuelLitresN01;
          persisted.todayLiters = todayLiters;
          persisted.fuelLevel = fuelLevel;
          interrupts();}
          
      Serial.printf("Local Fuel1 pulse -> %lu\n", fuelLitresN01);
      Blynk.virtualWrite(V7, fuelLitresN01);
      fuel1Flag = false;}

  if (fuel2Flag)
     {noInterrupts();
      fuelLitresN02++;
      todayLiters++;
      if (fuelLevel>0)
         {fuelLevel--;
          persisted.fuelLitresN02 = fuelLitresN02;
          persisted.todayLiters = todayLiters;
          persisted.fuelLevel = fuelLevel;
          interrupts();}
      Serial.printf("Local Fuel2 pulse -> %lu\n", fuelLitresN02);
      Blynk.virtualWrite(V9, fuelLitresN02);
      fuel2Flag = false;}*/
       
 ESP.wdtFeed();}             // feed hardware WDT occasionally

/////////////////////////////////////////////////////////////////////////////////////
////////////////---------------- SEND TIMER FUNCTION ----------------////////////////
/////////////////////////////////////////////////////////////////////////////////////

void SendTimer()

 //////////// ---------- Edge-detected polling with debounce ---------- /////////////
  
 {uint32_t fuelP3 = 0;
  uint32_t fuelP4 = 0;

  uint32_t deltaP3 = 0;      ////////// for measuring the difference in /////////////
  uint32_t deltaP4 = 0;      //////////// present and previous reading //////////////
  
  int packetSize = udp.parsePacket();
  if (packetSize)
     {char buf[255];
      int len = udp.read(buf, 255);
      if (len > 0)
         {buf[len] = 0;   // null terminate
          sscanf(buf, "%lu,%lu", &fuelP3, &fuelP4);}}   // Parse "123,456"
          // Serial.printf("Parsed -> fuelP3 = %lu | fuelP4 = %lu\n", fuelP3, fuelP4);}}

  if (fuel1Flag)
     {noInterrupts();
      fuelLitresN01++;
      todayLiters++;
      if (fuelLevel>0)
         {fuelLevel--;
          persisted.fuelLitresN01 = fuelLitresN01;
          persisted.todayLiters = todayLiters;
          persisted.fuelLevel = fuelLevel;
          interrupts();}
          
      // Serial.printf("Local Fuel1 pulse -> %lu\n", fuelLitresN01);
      Blynk.virtualWrite(V11, fuelLitresN01);
      fuel1Flag = false;}

  if (fuel2Flag)
     {noInterrupts();
      fuelLitresN02++;
      todayLiters++;
      if (fuelLevel>0)
         {fuelLevel--;
          persisted.fuelLitresN02 = fuelLitresN02;
          persisted.todayLiters = todayLiters;
          persisted.fuelLevel = fuelLevel;
          interrupts();}
      // Serial.printf("Local Fuel2 pulse -> %lu\n", fuelLitresN02);
      Blynk.virtualWrite(V13, fuelLitresN02);
      fuel2Flag = false;}


  noInterrupts();

  /////////// ------- Only add if new value is greater than the previous one ------- //////////////
  
  if (fuelP3 > persisted.fuelLitresN03)             ///////////////////////////////////////////////
      deltaP3 = fuelP3 - persisted.fuelLitresN03;   /////////// only add if new value is //////////         
  if (fuelP4 > persisted.fuelLitresN04)             ///////// greater than the previous one ///////
      deltaP4 = fuelP4 - persisted.fuelLitresN04;   ///////////////////////////////////////////////
  
   fuelLitresN03 += deltaP3;   //////////////// ---- update new readings ---- /////////////////////
   todayLiters += deltaP3;

  if (fuelLevel >= deltaP3)
     {fuelLevel -= deltaP3;}
  else {fuelLevel = 0;}
  
  fuelLitresN04 += deltaP4;   //////////////////---- update new readings ----//////////////////////
  todayLiters += deltaP4;
  
  if (fuelLevel >= deltaP4)
     {fuelLevel -= deltaP4;}
  else {fuelLevel = 0;}

  /////////////// ------- update persisted and runtime counters safely -------- ////////////////
  
  if(persisted.fuelLitresN03 != fuelLitresN03)
    {persisted.todayLiters = todayLiters;
     persisted.fuelLitresN03 = fuelLitresN03;
     persisted.fuelLevel = fuelLevel;
     EEPROM.put(500, persisted);
     EEPROM.commit();
     Serial.print("lo g fuelLitresN03 = ");
     Serial.println(persisted.fuelLitresN03);}

  if(persisted.fuelLitresN04 != fuelLitresN04)
    {persisted.todayLiters = todayLiters;
     persisted.fuelLitresN04 = fuelLitresN04;
     persisted.fuelLevel = fuelLevel;
     EEPROM.put(500, persisted);
     EEPROM.commit();
     Serial.print("lo g fuelLitresN04 = ");
     Serial.println(persisted.fuelLitresN04);}
  
  interrupts();

  /*String packet = String(buf);

  // Extract fuel3
  int f3_start = packet.indexOf("fuel3=") + 6;
  int f3_end   = packet.indexOf(",", f3_start);
  String fuel3_str = packet.substring(f3_start, f3_end);

  // Extract fuel4
  int f4_start = packet.indexOf("fuel4=") + 6;
  String fuel4_str = packet.substring(f4_start);

  // Convert to integer
  int fuel3 = fuel3_str.toInt();
  int fuel4 = fuel4_str.toInt();

  Serial.printf("Fuel3 = %d, Fuel4 = %d\n", fuel3, fuel4);*/
  
  
  /*uint32_t now = millis();
  
  int state = digitalRead(Fuel1Pin);         
  
  if (state == HIGH && lastPulseState == LOW && (now - lastEdgeMs) > DEBOUNCE_MS) 
     {if (fuelLevel > 0) 
         {fuelLevel--;                                       // decrement 1 liter
          fuelLitresN01++;                                     // 1 pulse = 1 liter
          todayLiters++;                                     // daily counter
          Serial.println("---------- Sallo Khupli Pallo : ----------");
          Blynk.virtualWrite(V5, fuelLevel);                 // update to Blynk
          Blynk.virtualWrite(V6, fuelLitresN01);               // update to Blynk
                                               
          lastEdgeMs = now;}}
       
  lastPulseState = state;*/
  
  fuelLitresD01 = fuelLitresN01 + fuelLitresN02;
  fuelLitresD02 = fuelLitresN03 + fuelLitresN04;
  // fuelLitresD03 = fuelLitresN05 + fuelLitresN06;
  // fuelLitresD04 = fuelLitresN07 + fuelLitresN08;
  
  fuelAmountN01 = fuelLitresN01 * fuelRatee;
  fuelAmountN02 = fuelLitresN02 * fuelRatee;
  fuelAmountD01 = fuelLitresD01 * fuelRatee;

  fuelAmountN03 = fuelLitresN03 * fuelRatee;
  fuelAmountN04 = fuelLitresN04 * fuelRatee;
  fuelAmountD02 = fuelLitresD02 * fuelRatee;

  fuelLitresP01 = fuelLitresD01 + fuelLitresD02;

  fuelAmountP01 = fuelAmountD01 + fuelAmountD02;
  
  // Serial.print("fuel1 Liters: "); Serial.println(fuelLitresN01);
  // Serial.print("fuel2 Liters: "); Serial.println(fuelLitresN02);
  // Serial.print("disp1 Liters: "); Serial.println(fuelLitresD01);
  // Serial.print("disp1 Amount: "); Serial.println(fuelAmountD01);
  
  // Serial.print("fuel3 Liters: "); Serial.println(fuelLitresN03);
  // Serial.print("fuel4 Liters: "); Serial.println(fuelLitresN04);
  // Serial.print("disp2 Liters: "); Serial.println(fuelLitresD02);
  // Serial.print("disp2 Amount: "); Serial.println(fuelAmountD02);
  
  // Serial.print("total Liters: "); Serial.println(fuelLitresP01);
  // Serial.print("fuell Rateee: "); Serial.println(fuelRatee);
  // Serial.print("total Amount: "); Serial.println(fuelAmountP01);
  
  Blynk.virtualWrite(V5,  fuelLevel);
  Blynk.virtualWrite(V3,  lastRefill); // keep last refil updated
  
  Blynk.virtualWrite(V11, fuelLitresN01);
  Blynk.virtualWrite(V13, fuelLitresN02);
  Blynk.virtualWrite(V14, fuelLitresD01);
  
  Blynk.virtualWrite(V16, fuelLitresN03);
  Blynk.virtualWrite(V18, fuelLitresN04);
  Blynk.virtualWrite(V19, fuelLitresD02);

  // Blynk.virtualWrite(V21, fuel05Liters);
  // Blynk.virtualWrite(V23, fuel06Liters);
  // Blynk.virtualWrite(V24, disp03Liters);

  // Blynk.virtualWrite(V26, fuel07Liters);
  // Blynk.virtualWrite(V28, fuel08Liters);
  // Blynk.virtualWrite(V29, disp04Liters);

  // Blynk.virtualWrite(V31, fuel09Liters);
  // Blynk.virtualWrite(V33, fuel10Liters);
  // Blynk.virtualWrite(V34, disp05Liters);

  // Blynk.virtualWrite(V36, fuel11Liters);
  // Blynk.virtualWrite(V38, fuel12Liters);
  // Blynk.virtualWrite(V39, disp06Liters);

  // Blynk.virtualWrite(V41, fuel13Liters);
  // Blynk.virtualWrite(V43, fuel14Liters);
  // Blynk.virtualWrite(V44, disp07Liters);

  // Blynk.virtualWrite(V46, fuel15Liters);
  // Blynk.virtualWrite(V48, fuel16Liters);
  // Blynk.virtualWrite(V49, disp08Liters);

  // Blynk.virtualWrite(V51, fuel17Liters);
  // Blynk.virtualWrite(V53, fuel18Liters);
  // Blynk.virtualWrite(V54, disp09Liters);

  // Blynk.virtualWrite(V56, fuel19Liters);
  // Blynk.virtualWrite(V58, fuel20Liters);
  // Blynk.virtualWrite(V59, disp10Liters);

  Blynk.virtualWrite(V60, fuelLitresP01);
  
  Blynk.virtualWrite(V61, fuelAmountN01);
  Blynk.virtualWrite(V62, fuelAmountN02);
  Blynk.virtualWrite(V63, fuelAmountD01);

  Blynk.virtualWrite(V64, fuelAmountN03);
  Blynk.virtualWrite(V65, fuelAmountN04);
  Blynk.virtualWrite(V66, fuelAmountD02);

  // Blynk.virtualWrite(V67, fuelAmountN05);
  // Blynk.virtualWrite(V68, fuelAmountN06);
  // Blynk.virtualWrite(V69, fuelAmountD03);

  // Blynk.virtualWrite(V70, fuelAmountN07);
  // Blynk.virtualWrite(V71, fuelAmountN08);
  // Blynk.virtualWrite(V72, fuelAmountD04);

  // Blynk.virtualWrite(V73, fuelAmountN09);
  // Blynk.virtualWrite(V74, fuelAmountN10);
  // Blynk.virtualWrite(V75, fuelAmountD05);

  // Blynk.virtualWrite(V76, fuelAmountN11);
  // Blynk.virtualWrite(V77, fuelAmountN12);
  // Blynk.virtualWrite(V78, fuelAmountD06);

  // Blynk.virtualWrite(V79, fuelAmountN13);
  // Blynk.virtualWrite(V80, fuelAmountN14);
  // Blynk.virtualWrite(V81, fuelAmountD07);

  // Blynk.virtualWrite(V82, fuelAmountN15);
  // Blynk.virtualWrite(V83, fuelAmountN16);
  // Blynk.virtualWrite(V84, fuelAmountD08);

  // Blynk.virtualWrite(V85, fuelAmountN17);
  // Blynk.virtualWrite(V86, fuelAmountN18);
  // Blynk.virtualWrite(V87, fuelAmountD09);
  
  // Blynk.virtualWrite(V88, fuelAmountN19);
  // Blynk.virtualWrite(V89, fuelAmountN20);
  // Blynk.virtualWrite(V90, fuelAmountD10);
  
  Blynk.virtualWrite(V91, fuelAmountP01);

  // handle EEPROM saving here (safe) //
     
  /* if (saveFuel1Flag)
     {saveFuel1Flag = false;

      // EEPROM.put(500, persisted);
      // EEPROM.commit();
      Serial.println("EEPROM updated safely. fuel1");
      yield();}  // prevent WDT
  
  if (saveFuel2Flag)
     {saveFuel2Flag = false;

      EEPROM.put(500, persisted);
      EEPROM.commit();
      Serial.println("EEPROM updated safely. fuel2");
      yield();}  // prevent WDT

  if (saveFuel3Flag)
     {saveFuel3Flag = false;

      EEPROM.put(500, persisted);
      EEPROM.commit();
      Serial.println("EEPROM updated safely. fuel3");
      yield();}  // prevent WDT

  if (saveFuel4Flag)
     {saveFuel4Flag = false;

      EEPROM.put(500, persisted);
      EEPROM.commit();
      Serial.println("EEPROM updated safely. fuel4");
      yield();}  // prevent WDT*/
  
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
      EEPROM.put(800, dailyHistory);
      EEPROM.commit();}

  if (!(h == 0 && m == 0))   // Reset flag when time is not midnight
     {sentToday = false;}    // and allow the next midnight trigger

  // -------------- DAILY TOTALS (1d, 2d, 3d) -----------------

  uint32_t day1 = 0, day2 = 0, day3 = 0;  
  persisted.dayLiters[0] = dailyHistory[0];   // yesterday
  persisted.dayLiters[1] = dailyHistory[0] + dailyHistory[1];   // 2 days
  persisted.dayLiters[2] = dailyHistory[0] + dailyHistory[1] + dailyHistory[2];   // 3 days

  // ------------- WEEKLY TOTALS (last 3 weeks) ---------------
     
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
     
  Blynk.virtualWrite(V92, persisted.dayLiters[0]); // 1 day
  Blynk.virtualWrite(V93, persisted.dayLiters[1]); // 2 days
  Blynk.virtualWrite(V94, persisted.dayLiters[2]); // 3 days

  Blynk.virtualWrite(V95,  week1);
  Blynk.virtualWrite(V96,  week2);
  Blynk.virtualWrite(V97,  week3);

  Blynk.virtualWrite(V98,  month1);
  Blynk.virtualWrite(V99,  month2);
  Blynk.virtualWrite(V100, month3);}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////    EEPROM SAVE    /////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

void saveEEPROM()

 {noInterrupts();
  persisted.fuelRate = fuelRatee;
  persisted.fuelLevel = fuelLevel;
  persisted.fuelLitresN01 = fuelLitresN01;
  persisted.fuelLitresN02 = fuelLitresN02;
  persisted.fuelLitresN03 = fuelLitresN03;
  persisted.fuelLitresN04 = fuelLitresN04;
  persisted.todayLiters = todayLiters;
  interrupts();
  
  EEPROM.put(500, persisted);
  EEPROM.put(800, dailyHistory);        //// save history separately ////
  EEPROM.commit();

  // lastSavedf3 = persisted.fuelLitresN03;
  // lastSavedf4 = persisted.fuelLitresN04;
  
  Serial.println("---------- 10 Seconds EEPROM saved: ----------");
  
  Serial.print("   FuelLevel  = "); Serial.println(persisted.fuelLevel);
  Serial.print("   NewwRefill = "); Serial.println(persisted.newRefill);
  Serial.print("   LastRefill = "); Serial.println(persisted.lastRefill);
  Serial.print("   TodayLiter = "); Serial.println(persisted.todayLiters);
  Serial.print("   fuelLitresN01 = "); Serial.println(persisted.fuelLitresN01);
  Serial.print("   fuelLitresN02 = "); Serial.println(persisted.fuelLitresN02);
  Serial.print("   fuelLitresN03 = "); Serial.println(persisted.fuelLitresN03);
  Serial.print("   fuelLitresN04 = "); Serial.println(persisted.fuelLitresN04);}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////    EEPROM LOAD    /////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
 
void loadEEPROM()

 {// Persist tmp;
  EEPROM.get(500, persisted);   // EEPROM.get(500, tmp);
  
  if (persisted.magic == 0xF00DCAFE) 
     {noInterrupts();
      fuelRatee = persisted.fuelRate;
      fuelLevel = persisted.fuelLevel;
      newRefill = persisted.newRefill;
      lastRefill = persisted.lastRefill;
      fuelLitresN01 = persisted.fuelLitresN01;
      fuelLitresN02 = persisted.fuelLitresN02;
      fuelLitresN03 = persisted.fuelLitresN03;
      fuelLitresN04 = persisted.fuelLitresN04;
      todayLiters = persisted.todayLiters;
      interrupts();

  /*lastSavedf1 = persisted.fuelLitresN01;
    lastSavedf2 = persisted.fuelLitresN02;
    lastSavedf3 = persisted.fuelLitresN03;
    lastSavedf4 = persisted.fuelLitresN04;*/
      
      Serial.println("---------- EEPROM has been Loaded: ----------");

      Serial.print("Fuell Ratee: ");    Serial.println(fuelRatee);
      Serial.print("Fuell level: ");    Serial.println(fuelLevel);
      Serial.print("Today liters: ");   Serial.println(todayLiters);
      Serial.print("Fuel1 liters: ");   Serial.println(fuelLitresN01);
      Serial.print("Fuel2 liters: ");   Serial.println(fuelLitresN02);
      Serial.print("Fuel3 liters: ");   Serial.println(fuelLitresN03);
      Serial.print("Fuel4 liters: ");   Serial.println(fuelLitresN04);
      Serial.print("Newww refill: ");   Serial.println(persisted.newRefill);
      Serial.print("Lastt refill: ");   Serial.println(persisted.lastRefill);
      
      Blynk.virtualWrite(V5, fuelLevel);               // restore fuel level
      Blynk.virtualWrite(V3, lastRefill);              // restore last refil
      
      Blynk.virtualWrite(V11, fuelLitresN01);             // restore fuel1 litre
      Blynk.virtualWrite(V13, fuelLitresN02);             // restore fuel2 litre
      
      Blynk.virtualWrite(V16, fuelLitresN03);            // restore fuel3 litre
      Blynk.virtualWrite(V18, fuelLitresN04);            // restore fuel4 litre
     
      Blynk.virtualWrite(V92,  persisted.dayLiters[0]);
      Blynk.virtualWrite(V93,  persisted.dayLiters[1]);
      Blynk.virtualWrite(V94,  persisted.dayLiters[2]);

      Blynk.virtualWrite(V95,  persisted.dayLiters[3]);
      Blynk.virtualWrite(V96,  persisted.dayLiters[4]);
      Blynk.virtualWrite(V97,  persisted.dayLiters[5]);

      Blynk.virtualWrite(V98,  persisted.dayLiters[6]);
      Blynk.virtualWrite(V99,  persisted.dayLiters[7]);
      Blynk.virtualWrite(V100, persisted.dayLiters[8]);}
      
  else
     {Serial.println("EEPROM invalid, initializing...");
     
      persisted = {0xF00DCAFE, 0, 0, 0, 0, 0, 0, 0, 0, 0, {0}, {0}};
      
      EEPROM.put(500, persisted);
      EEPROM.commit();}
      
  EEPROM.get(800, dailyHistory);}
