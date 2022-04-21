#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include <WiFi.h>
#include "time.h"
#include <DHT.h>

#define DHTPIN 17        // data DHT11
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

bool useFahrenheit       = false; // Default to Celsius
//bool showTemp            = true; // Default to Temp / Humidity
long lastDebounceButton1 = 0;    // Holds Button1 last debounce
long lastDebounceButton2 = 0;    // Holds Button2 last debounce
long debounceDelay       = 200;  // 200ms between re-polling

#include "Button2.h"
#define BUTTON1_PIN  0
#define BUTTON2_PIN  35
Button2 button;

//credentials
const char* ssid       = "SSID";
const char* password   = "PASSWORD";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;  // + 1h
const int   daylightOffset_sec = 3600;  // +1h

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

uint32_t targetTime = 0;       // for next 1 second timeout
struct tm timeinfo;          // https://koor.fr/C/ctime/struct_tm.wp
int hh, mm, ss, wday, year, mon, yday;
const char * days[] = {"Sun,", "Mon,", "Tue,", "Wed,", "Thu,", "Fri,", "Sat,"};
char buf[64];

byte omm = 99;
boolean initial = 1;
byte xcolon = 0;
unsigned int colour = 0;
byte cpos = 0;   // second colon

// left button toggle
boolean toggle = 1; 
boolean back = 1;   // back from scan

//static uint8_t conv2d(const char* p) {
//  uint8_t v = 0;
//  if ('0' <= *p && *p <= '9')
//    v = *p - '0';
//  return 10 * v + *++p - '0';
//}

//uint8_t hh=conv2d(__TIME__), mm=conv2d(__TIME__+3), ss=conv2d(__TIME__+6);  // Get H, M, S from compile time

void printLocalTime(){ 
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");  
  Serial.println(buf);
  
}

// INTRPT Function to execute when Button 1 is Pushed
void IRAM_ATTR toggleButton1() {
  if ((millis() - lastDebounceButton1) > debounceDelay) { 
    if (useFahrenheit){useFahrenheit = false;}
    else {useFahrenheit = true;}
    lastDebounceButton1 = millis();
  }
}

// INTRPT Function to execute when Button 2 is Pushed
void IRAM_ATTR toggleButton2() {
  if ((millis() - lastDebounceButton2) > debounceDelay) { 
    if (useFahrenheit){useFahrenheit = false;}
    else {useFahrenheit = true;}
    //Serial.println(useFahrenheit);
    lastDebounceButton2 = millis();
  }
}

void click(Button2& btn) {
    Serial.println("Long click detected\n");
    toggle = 1 - toggle; 
    tft.fillScreen(TFT_BLACK);   
}

void setup(void) {
  Serial.begin(115200);
  // init DH11
  dht.begin();
  
  // init 2 buttons
  //left button by click detection
  button.begin(BUTTON1_PIN);
  button.setLongClickDetectedHandler(click);
  
  // right button by interrupt
  pinMode(BUTTON2_PIN, INPUT);
  attachInterrupt(BUTTON2_PIN, toggleButton2, FALLING);
  
  // connected to internet
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");
  
  //get the date and time and print on serial port
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  // get local time
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
  }
  size_t written = strftime(buf, 64, "%b %d %Y", &timeinfo);
  hh=timeinfo.tm_hour, mm=timeinfo.tm_min, ss=timeinfo.tm_sec, wday=timeinfo.tm_wday;
  mon=timeinfo.tm_mon, year=timeinfo.tm_year, yday=timeinfo.tm_yday;
  
  // start screen
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); // Note: the new fonts do not draw the background colour

  // init timer to 1 second
  targetTime = millis() + 1000;   
}

// by default, show clock,temperature,humidity
void show_clock() {
  float t = dht.readTemperature(useFahrenheit);
  float h = dht.readHumidity();
  tft.setRotation(1);
  if (back) tft.fillScreen(TFT_BLACK);
  if (targetTime < millis()) {
    targetTime = millis()+1000;
    ss++;              // Advance second
    if (ss==60) {
      ss=0;
      omm = mm;
      mm++;            // Advance minute
      if(mm>59) {
        mm=0;
        hh++;          // Advance hour
        if (hh>23) {
          hh=0;
        }
      }
    }

    if (ss==0 || initial) {
      initial = 0;
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setCursor (8, 52);
      //tft.print(__DATE__); // This uses the standard ADAFruit small font

      //tft.setTextColor(TFT_BLUE, TFT_BLACK);
      //tft.drawCentreString("It is windy",120,48,2); // Next size up font 2

      //tft.setTextColor(0xF81F, TFT_BLACK); // Pink
      //tft.drawCentreString("12.34",80,100,6); // Large font 6 only contains characters [space] 0 1 2 3 4 5 6 7 8 9 . : a p m
    }

    // Update digital time
    byte xpos = 6;
    byte ypos = 0;
    
    if (omm != mm  || back) { // Only redraw every minute to minimise flicker
      // Uncomment ONE of the next 2 lines, using the ghost image demonstrates text overlay as time is drawn over it
      tft.setTextColor(0x39C4, TFT_BLACK);  // Leave a 7 segment ghost image, comment out next line!
      //tft.setTextColor(TFT_BLACK, TFT_BLACK); // Set font colour to black to wipe image
      // Font 7 is to show a pseudo 7 segment display.
      // Font 7 only contains characters [space] 0 1 2 3 4 5 6 7 8 9 0 : .
      tft.drawString("88:88:88",xpos,ypos,7); // Overwrite the text to clear it
      tft.setTextColor(0xFBE0, TFT_BLACK); // Orange
      omm = mm;
      if (hh<10) xpos+= tft.drawChar('0',xpos,ypos,7);
      xpos+= tft.drawNumber(hh,xpos,ypos,7);
      xcolon=xpos;
      xpos+= tft.drawChar(':',xpos,ypos,7);
      if (mm<10) xpos+= tft.drawChar('0',xpos,ypos,7);
      xpos+= tft.drawNumber(mm,xpos,ypos,7);
      xpos+= tft.drawChar(':',xpos,ypos,7);
      cpos = xpos;
      back = 0;
    }
    xpos = cpos;
    tft.setTextColor(0xFBE0, TFT_BLACK); // Orange
    if (ss<10) xpos+= tft.drawChar('0',xpos,ypos,7);
    tft.drawNumber(ss,xpos,ypos,7);      
    //}

    if (ss%2) { // Flash the colon
      //tft.setTextColor(0x39C4, TFT_BLACK);
      xpos+= tft.drawChar(':',xcolon,ypos,7);
      tft.setTextColor(0xFBE0, TFT_BLACK);
      colour = random(0xFFFF); tft.setTextColor(colour);
      if (mon==0) {tft.fillRect (0, 64, 250, 30, TFT_BLACK);tft.drawString("Happy New Year !",20,64,4);}
      //else {
        colour = random(0xFFFF); tft.setTextColor(colour);
        //tft.drawString("T:",8,100,4);
        tft.fillRect (0, 100, 120, 30, TFT_BLACK);
        tft.drawString(String(t),8,100,4);
        if (useFahrenheit){tft.drawString("F",80,100,4);}
        else {tft.drawString("C",80,100,4);}
        colour = random(0xFFFF); tft.setTextColor(colour);
        //tft.drawString("H:",118,100,4);
        tft.fillRect (118, 100, 80, 30, TFT_BLACK);
        tft.drawString(String(h),130,100,4);
        tft.drawString(" %",193,100,4);
      //}
    }
    else {
      tft.drawChar(':',xcolon,ypos,7);
      colour = random(0xFFFF);
      // Erase the old text with a rectangle, the disadvantage of this method is increased display flicker
      tft.fillRect (0, 64, 250, 30, TFT_BLACK);
      tft.setTextColor(colour);
      tft.drawRightString(days[wday],70,64,4); // Right justified string drawing to x position 75
      
      //String scolour = String(colour,HEX);
      //scolour.toUpperCase();
      //char buffer[20];
      //scolour.toCharArray(buffer,20);
      //tft.drawString(buffer,82,64,4);
      tft.drawString(buf,82,64,4);      
    }
  } 

}

// show wifi scan results when left button is clicked
void show_scan() {
   back = 1;
   tft.setRotation(0);

   tft.setTextColor(TFT_YELLOW, TFT_BLACK); // Note: the new fonts do not draw the background colour

    // Set WiFi to station mode and disconnect from an AP if it was previously connected
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    //Serial.println("Setup done");
    Serial.println("Scan start ...");

    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    //Serial.println("Scan done");
    tft.fillScreen(TFT_BLACK);
    if (n == 0) {
        Serial.println("No networks found");
    } else {
        Serial.print(n);
        Serial.println(" Networks found");
        for (int i = 0; i < n; ++i) {
            button.loop();
            // Print SSID and RSSI for each network found
            Serial.print(i + 1);
      tft.setCursor (0, 8*i);
      tft.print(i+1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setCursor (18, 8*i);
      tft.print(WiFi.SSID(i).substring(0,14)); // This uses the standard ADAFruit small font
            Serial.print(" (");
      tft.setCursor (112, 8*i);
      //tft.print("(");
            Serial.print(WiFi.RSSI(i));
      tft.print(WiFi.RSSI(i));
            Serial.print(")");
      //tft.print(")");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
            delay(10);
        }
    }
    Serial.println("");

    // Wait a bit before scanning again
    //delay(5000);
}

void loop() {  
  button.loop();
  if (toggle) {
    show_clock();
    //showScrn1();
  }
  else show_scan();
}
