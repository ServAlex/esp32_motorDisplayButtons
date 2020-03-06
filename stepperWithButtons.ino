
#define FRAMES_PER_SECOND  (120*2*2)

#define BUTTON_1 0
#define BUTTON_2 35

/////// display includes and defines
#include <TFT_eSPI.h>
#include <SPI.h>
#include "WiFi.h"
#include <Wire.h>
#include "esp_adc_cal.h"

#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN   0x10
#endif

#define TFT_MOSI            19
#define TFT_SCLK            18
#define TFT_CS              5
#define TFT_DC              16
#define TFT_RST             23

#define TFT_BL          4  // Display backlight control pin
#define ADC_EN          14
#define ADC_PIN         34

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library

char buff[512];
int vref = 1100;
int btnCick = false;

///////

/* tmc 2208 stepper wiring
ground:
en, ground
3.3:
sw1 sw2 voi
33:
step
32:
dir
4 modor pins to stepper
12v power supply:
vmot and ground near vmot + electrolytic cap (25v, 200uf) across 12v rails
*/


// motor

#define STEP_PIN  33
#define DIR_PIN   32

//////


int repeatStart = 1000;
int repeatPeriod = 100;

// button 1
long button1_pressStartedTime = 0;
long button1_lastRepeatedTime = 0;
bool button1_isPressed = false;
// button 2
long button2_pressStartedTime = 0;
long button2_lastRepeatedTime = 0;
bool button2_isPressed = false;
TaskHandle_t core0Task;

void core0TaskCode( void * pvParameters ){
  Serial.print("core 0 task is running on core ");
  Serial.println(xPortGetCoreID());

  for(;;)
  {
    delay(10);
    readButtons();
  } 
}

uint8_t gHue = 0; // rotating "base color" used by many of the patterns
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current


int maxBrightness = 10;
int brightness = 0;
int brightnessMultiplier = 28;

int currentMenu = 0;
int menuCount = 3;

int delayAmmount = 1000;
int delayAmmountMax = 100000;

void button2PresHandler()
{
  //Serial.println("b 1");
  delayAmmount = ((int)(delayAmmount*1.05))%delayAmmountMax;
  report();
}

uint8_t fpsMultiplier = 1;

void button1PresHandler()
{
  delayAmmount = ((int)((delayAmmount)/1.05)+delayAmmountMax)%delayAmmountMax;
  report();
  /*
  switch (currentMenu)
  {
  case 0:
    break;
  case 1:
    break;
  case 2:
    //Serial.println("");
    break;
  
  default:
    break;
  }
//  Serial.println(fpsMultiplier);
  report();
  */
}

void report()
{
  clearScreen();
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("Delay " + String(delayAmmount) + "/" + String(delayAmmountMax/1000)+"k", tft.width() / 2, tft.height() / 2);

  //tft.drawString("Pattern " + String(gCurrentPatternNumber+1) + "/" + ARRAY_SIZE(gPatterns), tft.width() / 2, tft.height() / 2 - 30);
  //tft.drawString("Speed " + String(fpsMultiplier) + "/10", tft.width() / 2, tft.height() / 2);
  //tft.drawString("Brghts " + String(brightness+1) + "/10", tft.width() / 2, tft.height() / 2 + 30);
  //tft.fillCircle(35, tft.height()/2-30 + currentMenu*30, 3, TFT_GREEN);
  //tft.drawCircle(35, tft.height()/2-30 + currentMenu*30, 3, TFT_GREEN);
}

void clearScreen()
{
  tft.fillScreen(TFT_BLACK);
}

void writeCenter(String str)
{
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(str, tft.width() / 2, tft.height() / 2);
}

void setup() {

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);

  digitalWrite(DIR_PIN, HIGH);

  delay(1000);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
//  tft.setTextColor(TFT_WHITE);
  tft.setCursor(0, 0);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);

  if (TFT_BL > 0) { // TFT_BL has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
        pinMode(TFT_BL, OUTPUT); // Set backlight pin to output mode
        digitalWrite(TFT_BL, TFT_BACKLIGHT_ON); // Turn backlight on. TFT_BACKLIGHT_ON has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
  }


  Serial.begin(115200);
  Serial.println("Start");

  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);

  report();

  //delay(1000); // 3 second delay for recovery
 
   xTaskCreatePinnedToCore(
                    core0TaskCode,   // Task function.
                    "core0Task",     // name of task.
                    10000,       // Stack size of task 
                    NULL,        // parameter of the task
                    0,           // priority of the task 
                    &core0Task,      // Task handle to keep track of created task 
                    0);          // pin task to core 0 
  delay(200); 
}


  
void loop()
{
  //FastLED.delay(1000/(30*fpsMultiplier)); 
  //FastLED.delay(1000/(FRAMES_PER_SECOND)); 

  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(delayAmmount);
  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(delayAmmount);
}


/*
/// beatsin8 generates an 8-bit sine wave at a given BPM,
///           that oscillates within a given range.
static inline uint8_t bitsaw8( accum88 beats_per_minute, uint8_t lowest = 0, uint8_t highest = 255,
                            uint32_t timebase = 0, uint8_t phase_offset = 0)
{
    uint8_t beat = beat8( beats_per_minute, timebase);
    uint8_t rangewidth = highest - lowest;
    uint8_t scaledbeat = scale8( beat, rangewidth);
    uint8_t result = lowest + scaledbeat;
    return result;
}
*/

int pos = 0;

void readButtons()
{
  long time = millis();

  int button1Val = digitalRead(BUTTON_1);
  int button2Val = digitalRead(BUTTON_2);

  if (button1Val != HIGH) 
  {
    // button pressed
    if(button1_isPressed)
    {
      // been pressed before
      if(time - button1_pressStartedTime > repeatStart && time - button1_lastRepeatedTime > repeatPeriod)
      {
        button1_lastRepeatedTime = time;
        button1PresHandler();
      }
    }
    else
    {
      // pressed for the first time
      button1_isPressed = true;
      button1_pressStartedTime = time;
      button1_lastRepeatedTime = 0;
      button1PresHandler();
    }
  }
  else
  {
    // button not pressed
    button1_isPressed = false;
    button1_pressStartedTime = 0;
    button1_lastRepeatedTime = 0;
  }
  
  if (button2Val != HIGH) 
  {
    // button pressed
    if(button2_isPressed)
    {
      // been pressed before
      if(time - button2_pressStartedTime > repeatStart && time - button2_lastRepeatedTime > repeatPeriod)
      {
        button2_lastRepeatedTime = time;
        button2PresHandler();
      }
    }
    else
    {
      // pressed for the first time
      button2_isPressed = true;
      button2_pressStartedTime = time;
      button2_lastRepeatedTime = 0;
      button2PresHandler();
    }
  }
  else
  {
    // button not pressed
    button2_isPressed = false;
    button2_pressStartedTime = 0;
    button2_lastRepeatedTime = 0;
  }
}
