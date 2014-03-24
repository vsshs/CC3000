// Libraries
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"
#include<stdlib.h>
#include <Adafruit_NeoPixel.h>


#define DEVICE_ID "1111"
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


#define WLAN_SSID       "smdkgljirmksmnbsndblksnlb"
#define WLAN_PASS       "72F70&0CFa3AiafVyXp%ZoFIB$eDF3%"

#define WLAN_SSID       "smartportal"
#define WLAN_PASS       "smartportal"

// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2


#define IDLE_TIMEOUT_MS  3000      // Amount of time to wait (in milliseconds) with no data 
// received before closing the connection.  If you know the server
// you're accessing is quick to respond, you can reduce this value.

uint32_t ip;

#define USE_SERIAL 1 // disables all serial output besides response data

unsigned long tries = 0;
unsigned long successes = 0;


///#define PINR 3
#define PING 5
//#define PINB 6
#define PINBUZZER 8



#define PIN 6
Adafruit_NeoPixel strip = Adafruit_NeoPixel(3, PIN, NEO_GRB + NEO_KHZ800);
int rgb[] = {0, 0, 0};
bool usingMega = 1;

void setup(void)
{
	strip.begin();
	strip.show(); // Initialize all pixels to 'off'

	pinMode(PINBUZZER, OUTPUT);  

	digitalWrite(PINBUZZER, LOW);

	// Initialize

	if (usingMega)
	{
		Serial.begin(115200);
		Serial1.begin(9600);
	}
	else
	{
		Serial.begin(9600);
	}
	if (usingMega) Serial.println(F("\nInitializing..."));
	if(!usingMega) cc3000.setPrinter(0); // if no mega - no printing from wifi module...

	if (!cc3000.begin())
	{
		if(usingMega) Serial.println(F("Couldn't begin()! Check your wiring?"));
		while(1);
	}

	ip = cc3000.IP2U32(192,168,0,67);

	for (int i = 0; i < 3; i++)
	{
		colorWipe(strip.Color(255, 0, 0), 50); // Red
		colorWipe(strip.Color(0, 255, 0), 50); // Green
		colorWipe(strip.Color(0, 0, 255), 50); // Blue
	}

	colorWipe(strip.Color(255, 255, 255), 0); // off
	//strip.show(); // Initialize all pixels to 'off'
}
//http://events2.vsshs.com/api/Test/TestMethod
#define WEBSITE      "192.168.0.67"
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
	if(usingMega) Serial.println("LOOP...");


	doRFIDNonMega();

	doWifiStuff();

	checkBuzzerStatus();

	checkBlinking();
}

unsigned long dhcpTimeout = 5000;
unsigned long t;

bool turn_buzzer_on = false;
unsigned long turn_buzzer_on_timestamp = 0;

bool turn_blinking_on = false;
bool turn_blinking_on_last = false;
// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
	for(uint16_t i=0; i<strip.numPixels(); i++) {
		strip.setPixelColor(i, c);
		strip.show();
		if (wait > 0)
			delay(wait);
	}
}

void checkBlinking()
{
	Serial.println("check blinking");



	if (turn_blinking_on)
	{
		turn_blinking_on_last = !turn_blinking_on_last;

		if (turn_blinking_on_last)
		{
			if (usingMega)
				Serial.println("on");
			colorWipe(strip.Color(rgb[0], rgb[1], rgb[2]), 0);
		}
		else
		{
			if (usingMega)
				Serial.println("off");
			colorWipe(strip.Color(0, 0, 0), 0);
		}

	}
	else
	{
		colorWipe(strip.Color(rgb[0], rgb[1], rgb[2]), 0);
	}
}



void doWifiStuff()
{
	
	
	// Connect to WiFi network
	if (!cc3000.checkConnected())
	{
		cc3000.disconnect();
		cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY);
	}
	if(usingMega) Serial.println(F("Connected!"));


	/* Wait for DHCP to complete */
	if(usingMega) Serial.println(F("Request DHCP"));
	fail_count=0;



	for(t=millis(); !cc3000.checkDHCP() && ((millis() - t) < dhcpTimeout); delay(1000));
	if(cc3000.checkDHCP()) 
	{
		if(usingMega) Serial.println(F("OK"));
	} 
	else 
	{
		if(usingMega) Serial.println(F("failed"));
		cc3000.disconnect();
		return;
	}

	if (ip != 0)
	{
		if(usingMega) cc3000.printIPdotsRev(ip);

		// Send request
		Adafruit_CC3000_Client client = cc3000.connectTCP(ip, 80);
		if (client.connected()) {

			if(usingMega) Serial.println("Connected!");

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
			if(usingMega) Serial.println(F("Connection failed"));    
			return;
		}

		if(usingMega) Serial.println(F("-------------------------------------"));

		/* Read data until either the connection is closed, or the idle timeout is reached. */
		unsigned long lastRead = millis();
		boolean jsonStarted=false;
		//int rgb[] = {0, 0, 0};

		while (client.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) 
		{
			int currentValue = 0;
			int currentPosition = 0;

			while (client.available()) 
			{
				char c = client.read();
				//if(USE_SERIAL) Serial.print(c);

				if (jsonStarted)
				{
					if (c == '"')
						break;
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
							Serial.print ("R"); Serial.print (rgb[0]);
							Serial.print (" G");Serial.print (rgb[1]);
							Serial.print (" B");Serial.println (rgb[2]);

							if(usingMega) Serial.print ("buzzer: ");
							if(usingMega) Serial.println(currentValue);

							if (currentValue > 0)
							{
								turn_buzzer_on = true;
							}
						}
						if (currentPosition == 4)
						{
							if(usingMega) Serial.print ("blink: ");
							if(usingMega) Serial.println(currentValue);
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

		//analogWrite(PINR, rgb[0]);
		//analogWrite(PING, rgb[1]);
		//analogWrite(PINB, rgb[2]);

		//colorWipe(strip.Color(rgb[0], rgb[1], rgb[2]), 0);

		if(usingMega) Serial.println(F("\n-------------------------------------"));

	}
	else
	{
		if(usingMega) Serial.println(F("\n\nERROR: Could not resolve IP. "));
	}


	fail_count = 0;

}


void checkBuzzerStatus()
{
	if (turn_buzzer_on)
	{
		turn_buzzer_on = false;
		turn_buzzer_on_timestamp = millis();
		if(usingMega) Serial.println("Setting buzzer timestamp");
	}

	if (turn_buzzer_on_timestamp > 0 && millis() - turn_buzzer_on_timestamp  < 15000)
	{
		digitalWrite(PINBUZZER, HIGH);
	}
	else
	{
		turn_buzzer_on_timestamp = 0;
		digitalWrite(PINBUZZER, LOW);
	}
}
void doRFIDNonMega()
{
	if (Serial1.available() > 0)
	{

		while(Serial1.available() && Serial1.read() != 0x02)
		{
			Serial1.read();
		}

		if (Serial1.available() >= 13)
		{
			data[0] = 0x02;
			counter = 1;
			for (int i = 0; i < 14; i++)
			{
				data[counter] = Serial1.read();
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
					for (int i = 0; i < 8; i++)
					{
						char a = (char)data[i+3];
						myTag[i] = a;
						//Serial.print(a);
					}
					myTag[8]= 0x00;
					last_tag_detected = millis();
				} 


				if(usingMega) Serial.println(myTag);
				// empty the rest of the buffer
				while(Serial1.available()) { Serial1.read(); }
			}
		}
		else
		{
			// empty the rest of the buffer
			while(Serial1.available()) { Serial1.read(); }
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