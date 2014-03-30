// Libraries
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"
#include<stdlib.h>
#include <Adafruit_NeoPixel.h>
#include "utility/netapp.h"
#include <avr/wdt.h>

#define HOME
#define DEVICE_ID "1114"
// Define CC3000 chip pins
#define ADAFRUIT_CC3000_IRQ   2
#define ADAFRUIT_CC3000_VBAT  7
#define ADAFRUIT_CC3000_CS    4

// Create CC3000 instances
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
										 SPI_CLOCK_DIV2); // you can change this clock speed

// WLAN parameters
//#define WLAN_SSID       "smdkgljirmksmnbsndblksnlb"
//#define WLAN_PASS       "72F70&0CFa3AiafVyXp%ZoFIB$eDF3%"

#define WLAN_SSID       "pitlab-local"
#define WLAN_PASS       "none123456s"

#define WLAN_SSID       "smartportal"
#define WLAN_PASS       "smartportal"

#ifdef HOME
	#define WLAN_SSID       "smdkgljirmksmnbsndblksnlb"
	#define WLAN_PASS       "72F70&0CFa3AiafVyXp%ZoFIB$eDF3%"
#endif


//#define WLAN_SSID       "pitlab-local"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2


#define IDLE_TIMEOUT_MS  3000      // Amount of time to wait (in milliseconds) with no data 
// received before closing the connection.  If you know the server
// you're accessing is quick to respond, you can reduce this value.

#ifdef HOME
uint32_t ip = cc3000.IP2U32(91,100,105,227);
#else
uint32_t ip = cc3000.IP2U32(192,168,0,67);
#endif


#define USE_SERIAL 1 // disables all serial output besides response data

unsigned long tries = 0;
unsigned long successes = 0;


///#define PINR 3
//#define PING 5
//#define PINB 6
#define PINBUZZER 9
#define PIN_NEOPIXEL 8
Adafruit_NeoPixel strip = Adafruit_NeoPixel(3, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
volatile int rgb[] = {0, 0, 0};
volatile int rgb_old[] = {0, 0, 0};
bool usingMega = 0;
unsigned long dhcpTimeout = 5000;
unsigned long t;

bool turn_buzzer_on = false;
unsigned long turn_buzzer_on_timestamp = 0;

volatile bool turn_blinking_on = false;
volatile bool turn_blinking_on_last = false;

unsigned long tempCounter = 0;
//#define DO_PRINTING
uint8_t copyCR;
void setup(void)
{
  copyCR = MCUCR; 
  MCUSR = 0; 
	wdt_reset();  
	wdt_disable();
Serial.begin(9600); // use serial for rfid stuff
Serial.println("booting up");

	//wdt_disable();
	//pinMode(13, OUTPUT);

	
	// initialize timer1 
	noInterrupts();           // disable all interrupts
	TCCR1A = 0;
	TCCR1B = 0;

	TCNT1 = 34286;            // preload timer 65536-16MHz/256/2Hz
	TCCR1B |= (1 << CS12);    // 256 prescaler 
	TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt
	interrupts();             // enable all interrupts
	
	strip.begin();
	strip.show(); // Initialize all pixels to 'off'

	pinMode(PINBUZZER, OUTPUT);  

	digitalWrite(PINBUZZER, LOW);

	// Initialize






	cc3000.setPrinter(0); // if no mega - no printing from wifi module...
	

        
	if (!cc3000.begin())
	{
		Serial.println(F("Couldn't begin()! Check your wiring?"));

		while(1);
	}



	for (int i = 0; i < 2; i++)
	{
		colorWipe(strip.Color(255, 0, 0), 25); // Red
		colorWipe(strip.Color(0, 255, 0), 25); // Green
		colorWipe(strip.Color(0, 0, 255), 25); // Blue
	}

	colorWipe(strip.Color(0, 0, 0), 0); // off
	
	//strip.show(); // Initialize all pixels to 'off'
	unsigned long aucDHCP = 14400;
	unsigned long aucARP = 3600;
	unsigned long aucKeepalive = 10;
	unsigned long aucInactivity = 40;
	if (netapp_timeout_values(&aucDHCP, &aucARP, &aucKeepalive, &aucInactivity) != 0) {
#ifdef DO_PRINTING
		Serial.println("Error setting inactivity timeout!");
#endif
	}

Serial.println("init complete");
	//wdt_disable();
}



ISR(TIMER1_OVF_vect)        // interrupt service routine that wraps a user defined function supplied by attachInterrupt
{
	TCNT1 = 34286;            // preload timer
	// Check for blinking stuff
	if (turn_blinking_on)
	{
		turn_blinking_on_last = !turn_blinking_on_last;

		if (turn_blinking_on_last) {
			colorWipe(strip.Color(rgb[0], rgb[1], rgb[2]), 0);
		}
		else {
			colorWipe(strip.Color(0, 0, 0), 0);
		}
	}
	else {
		if (!turn_blinking_on_last)
		{
			turn_blinking_on_last = true;
			colorWipe(strip.Color(rgb[0], rgb[1], rgb[2]), 0);
		}
		if (rgb_old[0] != rgb[0] || rgb_old[1] != rgb[1] || rgb_old[2] != rgb[2])
		{
			rgb_old[0] = rgb[0]; 
			rgb_old[1] = rgb[1];
			rgb_old[2] = rgb[2];
			colorWipe(strip.Color(rgb[0], rgb[1], rgb[2]), 0);

		}
	}
}


#ifdef HOME
#define WEBSITE  "umbraco.vsshs.com"//    "192.168.0.67"
#else
#define WEBSITE  "192.168.0.67"//    "192.168.0.67"
#endif

#define WEBPAGE "/api/Patient/checkpatient"

char fail_count;


unsigned long buzzerOn = 0;

int counter;
byte data[14];
byte hexBlock1,hexBlock2,hexBlock3,hexBlock4,hexBlock5;
byte hexCalculatedChecksum,hexChecksum;
char myTag[9];
unsigned long last_tag_detected = 999999;

void loop(void)
{

	//wdt_reset();

	doRFIDNonMega();

	doWifiStuff();

	checkBuzzerStatus();

	
}

unsigned char loop_colorWipeVar;

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
	for( loop_colorWipeVar=0; loop_colorWipeVar<strip.numPixels(); loop_colorWipeVar++) {
		strip.setPixelColor(loop_colorWipeVar, c);
		strip.show();
		if (wait > 0)
			delay(wait);
	}
}

char stop = 0;

unsigned long lastRequest = millis();
unsigned long lastRequest_now = millis();
char c;
unsigned long lastRead = millis();
boolean jsonStarted=false;

void doWifiStuff()
{
  
	//Serial.println(F("1 doWifiStuff()"));
	lastRequest_now = millis();

	if (lastRequest_now - lastRequest < 250) 
	{ return; }

tempCounter++;
  if (tempCounter % 250 == 0)
  Serial.println(tempCounter);
	//digitalWrite(13, HIGH);
	lastRequest = lastRequest_now;
	// Connect to WiFi network
	if (!cc3000.checkConnected())
	{
		//Serial.println(F("3. doWifiStuff()"));
		//cc3000.disconnect();
		//Serial.println(F("4. doWifiStuff()"));
		//Serial.print(F("Free RAM: ")); Serial.println(getFreeRam(), DEC);
		cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY);
	}



#ifdef DO_PRINTING
	Serial.println(F("Connected to AP!"));
	Serial.println(F("Request DHCP"));
#endif
	fail_count=0;


	/* Wait for DHCP to complete */
	for(t=millis(); !cc3000.checkDHCP() && ((millis() - t) < dhcpTimeout); delay(1000));
	if(cc3000.checkDHCP()) 
	{
#ifdef DO_PRINTING
		Serial.println(F("OK"));
#endif
	} 
	else 
	{
#ifdef DO_PRINTING
		Serial.println(F("failed"));
#endif
		cc3000.disconnect();
		return;
	}

	//if(usingMega) cc3000.printIPdotsRev(ip);


	if (!cc3000.checkConnected())
	{
		cc3000.disconnect();
		cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY);
	}

	//wdt_enable(WDTO_8S);

	wdt_enable(WDTO_8S);


	//delay(10000);
	// Send request
	Adafruit_CC3000_Client client = cc3000.connectTCP(ip, 80);

	if (client.connected()) {
		//if(usingMega) Serial.println("Connected!");

		client.fastrprint(F("GET "));
		client.fastrprint(WEBPAGE);
		client.fastrprint(F("?tagId="));
		char* tag = getTagNumber();
		client.fastrprint(tag);
		client.fastrprint(F("&deviceAuth="));
		client.fastrprint(DEVICE_ID);
		//client.fastrprint(F("&lastUpdated="));
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

#ifdef DO_PRINTING
		if(usingMega) Serial.println(F("Connection failed"));    
#endif
		return;
	}
        wdt_reset();
	//if(usingMega) Serial.println(F("-------------------------------------"));

	/* Read data until either the connection is closed, or the idle timeout is reached. */
	
	//bool stop = false;
	//int rgb[] = {0, 0, 0};
	 //Serial.println(F("-------------------------------------"));
	stop = 0;
	lastRead = millis();
	jsonStarted = false;
	while (client.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS) && stop == 0) 
	{
		int currentValue = 0;
		int currentPosition = 0;

		while (client.available()&& (millis() - lastRead < IDLE_TIMEOUT_MS)&& stop == 0) 
		{
                        wdt_reset();
			c = client.read();
			lastRead = millis();
			//Serial.print(c);

			if (jsonStarted)
			{
				if (c == '"')
				{
					stop=1;
					break;
				}
				//if(usingMega) Serial.println(c);
				if (c != '/')
				{
					currentValue = currentValue * 10 + c - '0';
				}
				else
				{
					//if(usingMega) Serial.print ("Current value: ");
					//if(usingMega) Serial.println(currentValue);
					// LED status
					if (currentPosition < 3)
					{
						rgb[currentPosition]  = currentValue;
					}

					// buzzer
					if (currentPosition == 3)
					{
#ifdef DO_PRINTING
						Serial.print ("R"); Serial.print (rgb[0]);
						Serial.print (" G");Serial.print (rgb[1]);
						Serial.print (" B");Serial.println (rgb[2]);

						if(usingMega) Serial.print ("buzzer: ");
						if(usingMega) Serial.println(currentValue);
#endif
						if (currentValue > 0)
						{
							turn_buzzer_on = true;
						}
					}
					if (currentPosition == 4)
					{
#ifdef DO_PRINTING
						Serial.print ("blink: ");
						Serial.println(currentValue);
#endif
						if (currentValue > 0)
						{
							turn_blinking_on = true;
						}
						else
						{
							turn_blinking_on = false;
						}
					}

					currentPosition ++;
					currentValue = 0;
				}
			}

			// skip all of the header crap...
			if (c=='"' && !jsonStarted)
			{
				jsonStarted = true;
				successes++;
			}

		}
	}
	client.close();
	
	fail_count = 0;

	//wdt_disable();
#ifdef DO_PRINTING
	Serial.println(successes);
#endif

	wdt_reset();
	wdt_disable();
	/*
	Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
	*/

	//digitalWrite(13, LOW);
}


void checkBuzzerStatus()
{
	if (turn_buzzer_on)
	{
		turn_buzzer_on = false;
		turn_buzzer_on_timestamp = millis();
		//if(usingMega) Serial.println("Setting buzzer timestamp");
	}

	if (turn_buzzer_on_timestamp > 0 && millis() - turn_buzzer_on_timestamp  < 3000)
	{
		digitalWrite(PINBUZZER, HIGH);
	}
	else
	{
		turn_buzzer_on_timestamp = 0;
		digitalWrite(PINBUZZER, LOW);
	}
}

unsigned long lastrfid = millis();
unsigned long lastrfid_now = millis();
unsigned char loop_doRFIDNonMega, loop_doRFIDNonMega2;

#ifdef DO_PRINTING
HardwareSerial rfidSerial = Serial1;
#else
HardwareSerial rfidSerial = Serial;
#endif
void doRFIDNonMega()
{
	lastrfid_now = millis();

	if (lastrfid_now - lastrfid < 150)
	{
		return;
	}
	/////Serial.println("Checking rfid");

	lastrfid = lastrfid_now;
	if (rfidSerial.available() > 0)
	{

		while(rfidSerial.available() && rfidSerial.read() != 0x02)
		{
			rfidSerial.read();
		}

		if (rfidSerial.available() >= 13)
		{
			data[0] = 0x02;
			counter = 1;
			for (loop_doRFIDNonMega = 0; loop_doRFIDNonMega < 14; loop_doRFIDNonMega++)
			{
				data[counter] = rfidSerial.read();
				counter++;
			}
			if(data[0] == 0x02 && data[13] == 0x03)
			{

				hexBlock1 = AsciiCharToNum(data[1])*16 + AsciiCharToNum(data[2]);
				hexBlock2 = AsciiCharToNum(data[3])*16 + AsciiCharToNum(data[4]);
				hexBlock3 = AsciiCharToNum(data[5])*16 + AsciiCharToNum(data[6]);
				hexBlock4 = AsciiCharToNum(data[7])*16 + AsciiCharToNum(data[8]);
				hexBlock5 = AsciiCharToNum(data[9])*16 + AsciiCharToNum(data[10]);

				hexChecksum = AsciiCharToNum(data[11])*16 + AsciiCharToNum(data[12]);

				//XOR algorithm to calculate checksum of ID blocks.
				hexCalculatedChecksum = hexBlock1 ^ hexBlock2 ^ hexBlock3 ^ hexBlock4 ^ hexBlock5;

				if ( hexCalculatedChecksum == hexChecksum )
				{
					for (loop_doRFIDNonMega2 = 0; loop_doRFIDNonMega2 < 8; loop_doRFIDNonMega2++)
					{

						myTag[loop_doRFIDNonMega2] = data[loop_doRFIDNonMega2+3];
						//Serial.print(a);
					}
					myTag[8]= 0x00;
					last_tag_detected = millis();
				} 


				// Serial.println(myTag);
				// empty the rest of the buffer
				while(rfidSerial.available()) { rfidSerial.read(); }
			}
		}
		else
		{
			// empty the rest of the buffer
			while(rfidSerial.available()) { rfidSerial.read(); }
		}

	}
}




char* getTagNumber()
{
	if (millis() - last_tag_detected < 1000)
		return myTag;
	else
		return "0";
}


uint8_t AsciiCharToNum(byte data) {
	//First substract 48 to convert the char representation
	//of a number to an actual number.
	data -= '0';
	//If it is greater than 9, we have a Hex character A-F.
	//Substract 7 to get the numeral representation.
	if (data > 9) 
		data -= 7;
	return data;
} 
