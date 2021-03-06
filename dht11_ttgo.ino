//////////////////////////////////////////////////////////////////
// DHT22 Test program for the ESP32 TTGO-T Display              //
// Jordan Rubin 2020                                            //
// More at https://www.youtube.com/c/jordanrubin6502            //
// http://technocoma.blogspot.com                               //
//////////////////////////////////////////////////////////////////

// Provides a graphical output on the built in display of the TTGO for both
// Temperature and humidity or switch between those and heat index with a
// push of a button.  The second button switches everything between Celsius
// and Fahrenheit. Both switches have built in interupt and debounce.

// Directly related to the video https://youtu.be/u7277VShso4

#include <DHT.h>
#include <TFT_eSPI.h>
#include <SPI.h>

#define DHTPIN 17
#define DHTTYPE DHT11
#define BUTTON1PIN 35
#define BUTTON2PIN 0

DHT dht(DHTPIN, DHTTYPE);
TFT_eSPI tft = TFT_eSPI();

bool useFahrenheit       = true; // Default to Fahrenheit
bool showTemp            = true; // Default to Temp / Humidity
long lastDebounceButton1 = 0;    // Holds Button1 last debounce
long lastDebounceButton2 = 0;    // Holds Button2 last debounce
long debounceDelay       = 200;  // 200ms between re-polling

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
    if (showTemp){showTemp = false;}
    else {showTemp = true;}
    lastDebounceButton2 = millis();
  }
}

// What to display if showTemp = true
void showScrn1() {
  float t = dht.readTemperature(useFahrenheit);
  float h = dht.readHumidity();
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(0, 30);
  tft.setFreeFont(&Orbitron_Light_24);
  tft.println("Temp     Humidity");
  tft.drawLine(0, 35, 250, 35, TFT_BLUE);
  tft.setCursor(0, 60);
  tft.print(t);
  if (useFahrenheit){tft.print(F("f"));}
  else {tft.print(F("c"));}
  tft.setCursor(130, 60);
  tft.print(h);tft.print(F("%"));
}

// What to display if showTemp = false
void showScrn2() {
  float t = dht.readTemperature(useFahrenheit);
  float h = dht.readHumidity();
  float hi = dht.computeHeatIndex(t, h, useFahrenheit);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setCursor(50, 30);
  tft.setFreeFont(&Orbitron_Light_24);
  tft.println("Heat Index");
  tft.drawLine(0, 35, 250, 35, TFT_BLUE);
  tft.setFreeFont(&Orbitron_Light_32);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(60, 100);
  tft.print(hi);
  if (useFahrenheit){tft.print(F("f"));}
  else {tft.print(F("c"));}
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON1PIN, INPUT);
  pinMode(BUTTON2PIN, INPUT);
  attachInterrupt(BUTTON1PIN, toggleButton1, FALLING);
  attachInterrupt(BUTTON2PIN, toggleButton2, FALLING);
  dht.begin();
  tft.begin();
  tft.setRotation(1); //Landscape
}

void loop() {
  delay(1000);  //Required by this device to function properly
  if (showTemp){showScrn1();} // Temp Humidity
  else {showScrn2();}         // Heat Index
}
