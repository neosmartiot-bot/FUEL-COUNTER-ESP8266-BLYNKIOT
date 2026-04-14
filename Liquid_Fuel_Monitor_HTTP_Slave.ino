/*******************************************************
   SLAVE DEVICE  –  Fuel Pulse Counter (ESP8266)
   -----------------------------------------------------
   • Counts pulses on Fuel3 and Fuel4
   • Sends counts to Master using UDP
   • Debounces pulses inside ISR functions
   • Sends updated counts to Master via ESP-NOW
   • NO BLYNK – This node only sends data to master
********************************************************/

#include <EEPROM.h>
#include <espnow.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>

WiFiUDP udp;
#define MASTER_PORT 4210
const char* MASTER_IP = "192.168.4.1";     // change to your master IP

////////// ------------------ PIN SETTINGS ----------------- //////////

#define Fuel3Pin  5               // Input fuel three pin
#define Fuel4Pin  4               // Input fuel fourr pin

////////// ---------- VOLATILE VALUES (ISR safe) ----------- //////////

uint32_t lastSavedf3 = 0;                 // last value saved in EEPROM
uint32_t lastSavedf4 = 0;                 // last value saved in EEPROM
uint32_t lastSenttf3 = 0;                 // last value saved in EEPROM
uint32_t lastSenttf4 = 0;                 // last value saved in EEPROM

unsigned long lastSendTime = 0;
unsigned long lastSaveTime = 0;

volatile uint32_t fuel3Liters = 0;
volatile uint32_t fuel4Liters = 0;

const unsigned long debounceMs = 1000;
const unsigned long sendInterval = 3000;

volatile unsigned long lastInterrupt3 = 0;
volatile unsigned long lastInterrupt4 = 0;

////////// --------------- MASTER MAC ADDRESS ------------- //////////

uint8_t masterMac[] = {0x34, 0x5F, 0x45, 0x58, 0xFC, 0xEC}; // one of actual 
                   // {0x34, 0x5F, 0x45, 0x56, 0x51, 0x54}; // one of actual
                   // {0x48, 0x55, 0x19, 0xAB, 0xCD, 0xEF}; // a gpt example

////////// ------------ DATA PACKET TO MASTER ------------- //////////

typedef struct struct_message
 {uint8_t nodeId;       // Which node sent this
  uint32_t pulsesf3;    // Pulse count for nozzle 3
  uint32_t pulsesf4;}   // Pulse count for nozzle 4
  struct_message;
  
struct_message outgoing;

////////// ------------ EEPROM PERSIST STRUCT ------------- //////////

struct Persist
 { uint32_t magic;        /////////////// sanity check ///////////////
  uint32_t fuel3Liters;
  uint32_t fuel4Liters;};
  // persisted;
  
Persist persisted {0xF00DCAFE, 0, 0};

void SendTimer();
void loadEEPROM();
void saveEEPROM();
void PrepAndSend();

////////// --------- ISR: FUEL3 PULSE DETECTION ----------- //////////

void IRAM_ATTR Fuel3Pulse()
 {unsigned long now = millis();
  if (now - lastInterrupt3 > debounceMs)
     {fuel3Liters++;
      lastInterrupt3 = now;}}

////////// --------- ISR: FUEL4 PULSE DETECTION ----------- //////////

void IRAM_ATTR Fuel4Pulse() 
 {unsigned long now = millis();
  if (now - lastInterrupt4 > debounceMs)
     {fuel4Liters++;
      lastInterrupt4 = now;}}

////////// ---------- MAC ADDRESS CONFIRMATION ----------- //////////

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus);

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus)
 
               {char macStr[18];
                sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                mac_addr[0], mac_addr[1], mac_addr[2],
                mac_addr[3], mac_addr[4], mac_addr[5]);
                Serial.printf("OnDataSent to %s, status=%u\n", macStr, sendStatus);}
                // status: 0 = success (ESP_OK), non-zero = failed (sendStatus value is informative)

////////// ----------- Master credential fetch ----------- //////////

/*{Serial.println(F("Scanning for FuelMaster AP..."));
  int n = WiFi.scanNetworks();
  bool found = false;
  for (int i = 0; i < n; i++)
      {if (WiFi.SSID(i) == "FuelMaster")
          {found = true;
           break;}}
       if (!found)
          {Serial.println(F("No FuelMaster AP found."));
           return false;}
           
       Serial.println(F("Found FuelMaster - Connecting to it..."));   
       WiFi.begin("FuelMaster", "12345678");
       unsigned long start = millis();
       
       while (WiFi.status() != WL_CONNECTED && millis() - start < 10000)
             {delay(500);}
             
       if (WiFi.status() != WL_CONNECTED)
          {Serial.println(F("Failed to connect to FuelMaster AP."));
           return false;}

       Serial.println(F("Connected to FuelMaster AP - Opening TCP to 192.168.4.1:8080"));
       WiFiClient client;
       
       if (!client.connect(IPAddress(192,168,4,1), 8080))
          {Serial.println(F("Could not connect to master TCP server."));
           WiFi.disconnect(true);
           return false;}
             
       // if (WiFi.status() == WL_CONNECTED)
               
       client.setTimeout(3000);
       // if (client.connect(IPAddress(192, 168, 4, 1), 8080))   // default AP IP
              
              String s1 = client.readStringUntil('\n');
              String s2 = client.readStringUntil('\n');
              String s3 = client.readStringUntil('\n');

              s1.trim(); s2.trim(); s3.trim();

              client.stop();
              WiFi.disconnect(true);

              if (s1.length() == 0 || s3.length() == 0)
                 {Serial.println(F("Invalid credentials received."));
                  return false;}
               
              // copy to c-strings
               
              s1.toCharArray(saved_ssid, sizeof(saved_ssid));
              s2.toCharArray(saved_pass, sizeof(saved_pass));
              s3.toCharArray(saved_auth, sizeof(saved_auth));

              Serial.print(F("Received SSID: ")); Serial.println(saved_ssid);
              Serial.print(F("Received AUTH: ")); Serial.println(saved_auth);

              // save into EEPROM
                   
              EEPROM.put(100, saved_ssid);
              EEPROM.put(200, saved_pass);
              EEPROM.put(300, saved_auth);
              EEPROM.commit();

              Serial.println("Credentials saved! Rebooting...");
              delay(500);
              ESP.restart();
              return true;}*/

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////    BLYNK HANDLERS    //////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

/*BLYNK_WRITE(V4)
 
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

BLYNK_WRITE(V11)
 
 {// unsigned long setLiters = param.asLong();   // get value from app

  String rawVal3 = param.asStr();                // always comes as string
  unsigned long set3Liters = rawVal3.toInt();    // convert safely
  
  fuel3Liters = set3Liters;                      // overwrite current total
  persisted.fuel3Liters = fuel3Liters;           // save into struct
  
  EEPROM.put(500, persisted);
  EEPROM.commit();
  
  Blynk.virtualWrite(V11, fuel3Liters);           // always update app
  
  Serial.print("fuel3Liters manually set to: ");
  Serial.println(fuel3Liters);}

BLYNK_WRITE(V13)
 
 {// unsigned long setLiters = param.asLong();   // get value from app

  String rawVal4 = param.asStr();                // always comes as string
  unsigned long set4Liters = rawVal4.toInt();    // convert safely
  
  fuel4Liters = set4Liters;                      // overwrite current total
  persisted.fuel4Liters = fuel4Liters;           // save into struct
  
  EEPROM.put(500, persisted);
  EEPROM.commit();
  
  Blynk.virtualWrite(V13, fuel4Liters);           // always update app
  
  Serial.print("fuel4Liters manually set to: ");
  Serial.println(fuel4Liters);}*/

/*BLYNK_WRITE(V12)
 
 {// unsigned long setLiters = param.asLong();   // get value from app

  String rawVal3 = param.asStr();                // always comes as string
  unsigned long set3Liters = rawVal3.toInt();    // convert safely
  
  fuel3Liters = set3Liters;                      // overwrite current total
  persisted.fuel3Liters = fue31Liters;           // save into struct
  
  EEPROM.put(500, persisted);
  EEPROM.commit();
  
  Blynk.virtualWrite(V12, fuel3Liters);          // always update app
  
  Serial.print("Fuel3Liters manually set to: ");
  Serial.println(fuel3Liters);}*/

/*BLYNK_WRITE(V14)
 
 {// unsigned long setLiters = param.asLong();   // get value from app

  String rawVal4 = param.asStr();                // always comes as string
  unsigned long set4Liters = rawVal4.toInt();    // convert safely
  
  fuel4Liters = set4Liters;                      // overwrite current total
  persisted.fuel4Liters = fue14Liters;           // save into struct
  
  EEPROM.put(500, persisted);
  EEPROM.commit();
  
  Blynk.virtualWrite(V14, fuel4Liters);          // always update app
  
  Serial.print("Fuel4Liters manually set to: ");
  Serial.println(fuel4Liters);}*/

/*BLYNK_WRITE(V26)  

  {int button = param.asInt();
   
   if(button == 1)
     
     {fuelLevel=0;
      Blynk.virtualWrite(V5, fuelLevel);
      EEPROM.put(500, persisted);
      EEPROM.commit();
      Blynk.virtualWrite(V26, 0);}}*/
      
      /*for (int i = 0; i < 90; i++) dailyHistory[i] = 0;
      for (int i = 0; i < 9; i++) persisted.dayLiters[i] = 0;

      todayLiters = 0;

      EEPROM.put(500, persisted);
      EEPROM.put(600, dailyHistory);
      EEPROM.commit();

      Blynk.virtualWrite(V16, 0);}}*/

/*BLYNK_WRITE(V27)
  
  {String entered = param.asStr();
   
   if(entered == "0000")
     
     {for (int i = 0; i < 90; i++) dailyHistory[i] = 0;
      for (int i = 0; i < 9; i++) persisted.dayLiters[i] = 0;

      todayLiters = 0;

      EEPROM.put(500, persisted);
      EEPROM.put(600, dailyHistory);
      EEPROM.commit();

      Blynk.virtualWrite(V27, 0);}}*/

/*BLYNK_WRITE(V28)

 {String rawRate = param.asStr();
  fuelRate = rawRate.toFloat();         // store rate per liter
  Serial.print("Fuel rate set to: ");
  Serial.println(fuelRate);}*/

/*BLYNK_WRITE(V30) 
 
 {if (param.asInt())
     {Blynk.logEvent("wifi_reset", "Forcing hotspot mode...");
      // Reset WiFi credentials
      Blynk.disconnect();                        // disconnect from server
      WiFi.disconnect(true);}}                   // drop the previous WiFi
      // BlynkState::set(MODE_WAIT_CONFIG);}}    // go to hotspot (provisioning) mode*/

//////////////////////////////////////////////////////////////////
//////////////////////    MAIN FUNCTIONS    //////////////////////
//////////////////////////////////////////////////////////////////

void setup()

  {Serial.begin(115200);
   delay(100);
   
   EEPROM.begin(1024);
   loadEEPROM();
   
   WiFi.mode(WIFI_STA);
   WiFi.begin("FuelMaster", "12345678");

   Serial.print("Connecting");
   while (WiFi.status() != WL_CONNECTED)
         {delay(500);
          Serial.print(".");}
          
   Serial.println("\nConnected to fuelmaster!");

   Serial.print("This STA IP: ");
   Serial.println(WiFi.localIP());  // prints the IP assigned by router
 
   pinMode(Fuel3Pin, INPUT_PULLUP);
   pinMode(Fuel4Pin, INPUT_PULLUP);

   attachInterrupt(digitalPinToInterrupt(Fuel3Pin), Fuel3Pulse, RISING);
   attachInterrupt(digitalPinToInterrupt(Fuel4Pin), Fuel4Pulse, RISING);}
   
   // lastSendTime = millis();}
  
      
   // edgentTimer.setInterval(1000L, SendTimer);        
   // edgentTimer.setInterval(1800000L, eepromSave);}
   
//////////////////////////////////////////////////////////////////////////////
/////////////////////////    PULSE DETECTION    //////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void loop()
  
  {// Periodic sending interval
    
   if (millis() - lastSendTime >= sendInterval)
      {PrepAndSend();
       lastSendTime = millis();}

   yield();}   // keep the loop at minimal idle
    
   /*if (millis() - lastSendTime > sendInterval)
      {uint32_t deltaP3 = 0;
       deltaP3 = fuel3Liters - lastSavedf3;
       uint32_t deltaP4 = 0;
       deltaP4 = fuel4Liters - lastSavedf4;
        
       // noInterrupts();
       outgoing.nodeId = 1;
       outgoing.pulsesf3 = deltaP3;
       outgoing.pulsesf4 = deltaP4;
       // interrupts();

       saveEEPROM();

       Serial.print("lo g delta fuel3Liters = "); Serial.println(deltaP3);
       Serial.print("lo g delta fuel4Liters = "); Serial.println(deltaP4);

       esp_now_send(masterMac, (uint8_t *) &outgoing, sizeof(outgoing));

       // int sendRes = esp_now_send(masterMac, (uint8_t *)&outgoing, sizeof(outgoing));
       // Serial.printf("esp_now_send() returned %d\n", sendRes);

       Serial.printf("Sent pulsesf3=%lu pulsesf4=%lu\n", outgoing.pulsesf3, outgoing.pulsesf4);
       
       /*if (fuel3Liters - lastSavedf3 >= 1)
          {
           lastSavedf3 = fuel3Liters;}

       if (fuel4Liters - lastSavedf4 >= 1)
          {
           lastSavedf4 = fuel4Liters;}

       lastSendTime = millis();}}*/

void PrepAndSend() 

 {uint32_t currentf3;        // Read volatile counters
  uint32_t currentf4;        // Read volatile counters
  
  noInterrupts();
  currentf3 = fuel3Liters;
  currentf4 = fuel4Liters;
  interrupts();

  char payload[40];
  sprintf(payload, "%lu,%lu", currentf3, currentf4);
  
  // String payload = "fuel3=10; - fuel4=20";
  
  udp.beginPacket(MASTER_IP, MASTER_PORT);
  // udp.write(payload.c_str());
  udp.write(payload);
  udp.endPacket();
  Serial.printf("UDP Sent -> f3=%lu f4=%lu\n", currentf3, currentf4);

  lastSenttf3 = currentf3;
  lastSenttf4 = currentf4;

  //////////////////---------- Save to EEPROM only if values changed (reduces flash wear) ----------////////////////////
  
  if (currentf3 != lastSavedf3 || currentf4 != lastSavedf4)
     {saveEEPROM();}}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////----------- EEPROM SAVE & LOAD ---------////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void saveEEPROM()

 {persisted.magic = 0xF00DCAFE;
  
  noInterrupts();
  persisted.fuel3Liters = fuel3Liters;
  persisted.fuel4Liters = fuel4Liters;
  interrupts();

  EEPROM.put(0, persisted);
  EEPROM.commit();

  lastSavedf3 = persisted.fuel3Liters;
  lastSavedf4 = persisted.fuel4Liters;

  Serial.printf("Forced EEPROM Saving: fuel3=%lu fuel4=%lu\n", persisted.fuel3Liters, persisted.fuel4Liters);}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////----------- EEPROM SAVE & LOAD ---------////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loadEEPROM()

 {EEPROM.get(0, persisted);

  if (persisted.magic != 0xF00DCAFE)
     {Serial.println("EEPROM Reset");
      persisted.magic = 0xF00DCAFE;
      persisted.fuel3Liters = 0;
      persisted.fuel4Liters = 0;
      EEPROM.put(0, persisted);
      EEPROM.commit();}

  noInterrupts();
  fuel3Liters = persisted.fuel3Liters;
  fuel4Liters = persisted.fuel4Liters;
  interrupts();

  lastSavedf3 = persisted.fuel3Liters;
  lastSavedf4 = persisted.fuel4Liters;
  lastSenttf3 = lastSavedf3;             // assume we haven't sent anything new yet
  lastSenttf4 = lastSavedf4;

  Serial.printf("EEPROM Restored fuel3=%lu fuel4=%lu\n", fuel3Liters, fuel4Liters);}
