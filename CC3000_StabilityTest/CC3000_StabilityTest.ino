// Libraries
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"
#include<stdlib.h>

// Define CC3000 chip pins
#define ADAFRUIT_CC3000_IRQ   3
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10

// Create CC3000 instances
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
SPI_CLOCK_DIV2); // you can change this clock speed

// WLAN parameters
#define WLAN_SSID       "smdkgljirmksmnbsndblksnlb"
#define WLAN_PASS       "72F70&0CFa3AiafVyXp%ZoFIB$eDF3%"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2            

#define IDLE_TIMEOUT_MS  3000      // Amount of time to wait (in milliseconds) with no data 
// received before closing the connection.  If you know the server
// you're accessing is quick to respond, you can reduce this value.
uint32_t ip;

#define USE_SERIAL 0 // disables all serial output besides response data
unsigned long tries = 0;
unsigned long successes = 0;
void setup(void)
{
  // Initialize
  //Serial.begin(115200);

  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  if(USE_SERIAL) Serial.println(F("\nInitializing..."));
   if(USE_SERIAL == 0) cc3000.setPrinter(0);
  if (!cc3000.begin())
  {
    if(USE_SERIAL) Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
  
  

}
//http://events2.vsshs.com/api/Test/TestMethod
#define WEBSITE      "events2.vsshs.com"
#define WEBPAGE      "/api/Test/TestMethod"


char fail_count;
void loop(void)
{
  digitalWrite(13, LOW);
  tries++;
  // Connect to WiFi network
  cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY);
  if(USE_SERIAL) Serial.println(F("Connected!"));


  /* Wait for DHCP to complete */
  if(USE_SERIAL) Serial.println(F("Request DHCP"));
  fail_count=0;
  while (!cc3000.checkDHCP())
  {
    delay(1000);
    if(USE_SERIAL) Serial.println(F("DHCP fail"));
  }  

  // Get the website IP & print it
  ip = 0;
  fail_count = 0;
  if(USE_SERIAL) Serial.print(WEBSITE); 
  if(USE_SERIAL) Serial.print(F(" -> "));
  
  /*
  while (ip == 0 && fail_count < 5) 
  {
    fail_count++;
    if (! cc3000.getHostByName(WEBSITE, &ip)) 
    {

      if(USE_SERIAL) Serial.print(fail_count);
      if(USE_SERIAL) Serial.println(F("Couldn't resolve!"));


    }
    else
    {
      if(USE_SERIAL) Serial.print(F("\n\nhost resolved"));
      if(USE_SERIAL) Serial.println(ip);
    }
    delay(500);
  }
  */
  ip = cc3000.IP2U32(91,100,105,227);
  
  if (ip != 0)
  {
    cc3000.printIPdotsRev(ip);

    // Send request
    Adafruit_CC3000_Client client = cc3000.connectTCP(ip, 80);
    if (client.connected()) {
      if(USE_SERIAL) Serial.println("Connected!");
      client.fastrprint(F("GET "));
      client.fastrprint(WEBPAGE);
      client.fastrprint(F(" HTTP/1.1\r\n"));
      client.fastrprint(F("Host: ")); 
      client.fastrprint(WEBSITE); 
      client.fastrprint(F("\r\nAccept: application/json"));
      client.fastrprint(F("\r\n"));
      client.fastrprint(F("\r\n"));
      client.println();
    } 
    else 
    {
      if(USE_SERIAL) Serial.println(F("Connection failed"));    
      return;
    }

    if(USE_SERIAL) Serial.println(F("-------------------------------------"));
    /* Read data until either the connection is closed, or the idle timeout is reached. */
    unsigned long lastRead = millis();
    boolean jsonStarted=false;
    while (client.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
      while (client.available()) {
        char c = client.read();
        //if(USE_SERIAL) Serial.print(c);
        // skip all of the header crap...
        if (c=='{' && !jsonStarted)
        {
          jsonStarted = true;
          successes++;
        }

        if (jsonStarted)
          {
           // Serial.print(c);
           digitalWrite(13, HIGH);
          }
      }
    }
    client.close();
    if(USE_SERIAL) Serial.println(F("\n-------------------------------------"));

  }
  else
  {
    if(USE_SERIAL) Serial.println(F("\n\nERROR: Could not resolve IP. "));
  }

  if(USE_SERIAL) Serial.print("Free RAM: "); 
  //if(USE_SERIAL) Serial.println(getFreeRam(), DEC);
  if(USE_SERIAL) Serial.println(F("\n\nDisconnecting"));

  fail_count = 0;
  while(!cc3000.disconnect() )
  {
    if(USE_SERIAL) Serial.println(F("\nFAILED to disconnect"));
    delay(1000);
    fail_count++;
    if (fail_count == 5)
    {
      cc3000.reboot(); 
      delay(5000);
      break;
    } 
  }

  if(USE_SERIAL) Serial.print("Tries: ");
  if(USE_SERIAL) Serial.println (tries);
  
  if(USE_SERIAL) Serial.print("Successes: ");
  if(USE_SERIAL) Serial.println (successes);
  
  // Wait 10 seconds until next update
  delay(3000);

}



