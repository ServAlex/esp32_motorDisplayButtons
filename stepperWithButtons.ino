
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
// bothButtons
long bothButtons_pressStartedTime = 0;
long bothButtons_lastRepeatedTime = 0;
bool bothButtons_isPressed = false;
TaskHandle_t core0Task;

void core0TaskCode( void * pvParameters ){
  Serial.print("core 0 task is running on core ");
  Serial.println(xPortGetCoreID());

  for(;;)
  {
    delay(10);
    readButtons();
    //report();
  } 
}

uint8_t gHue = 0; // rotating "base color" used by many of the patterns
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current


int maxBrightness = 10;
int brightness = 0;
int brightnessMultiplier = 28;

int currentMenu = 0;
int menuCount = 3;

int delayAmmount = 200;
int delayAmmountMax = 100000;
int minDelayAmmount = 30;

void button2PresHandler()
{
  //Serial.println("b 1");
    delayAmmount = max(minDelayAmmount, ((int)(delayAmmount*1.05))%delayAmmountMax);
    report();
}

uint8_t fpsMultiplier = 1;

void button1PresHandler()
{
  delayAmmount = max(minDelayAmmount, ((int)((delayAmmount)/1.05)+delayAmmountMax)%delayAmmountMax);
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

int mode = 2;

void bothButtonsPressHandler()
{
    mode = ++mode%4;
    report();
}

void report()
{
    clearScreen();
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("Delay " + String(delayAmmount) + "/" + String(delayAmmountMax/1000)+"k", tft.width() / 2, tft.height() / 2);
    String modeName = "";
    switch(mode)
    {
        case 0:
            modeName = "oscilate";
            break;
        case 1:
            modeName = "driveWithSin";
            break;
        case 2:
            modeName = "driveWithMicros";
            break;
        case 3:
            modeName = "driveWithDelays";
            break;
    }

  tft.drawString(modeName, tft.width() / 2, tft.height() / 2 + 30);

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

uint64_t lastStateSwitch = 0;
bool state = false;
  
int swingMin = 35;
int swingMax = 300;
float switchDelay = 0.0064;
uint64_t lastDelaySwitch = 0;
int swingDir = 1;
int swingRange = swingMax - swingMin;
float swing = 0.f;

void loop()
{
    //oscilate();
    //driveWithSin();
    //driveWithMicros();
    //driveWithDelays();
    switch(mode)
    {
        case 0:
            oscilate();
            break;
        case 1:
            driveWithSin();
            break;
        case 2:
            driveWithMicros();
            break;
        case 3:
            driveWithDelays();
            break;
    }
}

void oscilate()
{
  uint64_t time = micros();
  if(time - lastStateSwitch >= delayAmmount)
  {
    digitalWrite(STEP_PIN, state?HIGH:LOW);
    state = !state;
    lastStateSwitch = time;
  }

  if(time - lastDelaySwitch >= switchDelay*1000000)
  {
    float fase = sin(swing);
    swingDir = (int)round((fase+1)/2);
    float period = 1-abs(fase);
    delayAmmount = (int)((period)*swingRange+swingMin);

    //Serial.println(String(swingDir) + " "+String(fase) + " "+String((fase+1)/2) + " "+String(period));
    digitalWrite(DIR_PIN, swingDir?HIGH:LOW);
    swing+=0.1f;
    lastDelaySwitch = time;
  }
}
void driveWithSin()
{
  uint64_t time = micros();
  if(time - lastStateSwitch >= delayAmmount)
  {
    digitalWrite(STEP_PIN, state?HIGH:LOW);
    state = !state;
    lastStateSwitch = time;
  }
  if(time - lastDelaySwitch >= switchDelay*1000000)
  {
    delayAmmount = (int)((sin(swing)+1)/2*swingRange + swingMin);
    swing+=0.1f;
    lastDelaySwitch = time;
  }
}
void driveWithMicros()
{
  uint64_t time = micros();
  if(time - lastStateSwitch >= delayAmmount)
  {
    digitalWrite(STEP_PIN, state?HIGH:LOW);
    state = !state;
    lastStateSwitch = time;
  }
}
void driveWithDelays()
{
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(delayAmmount);
  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(delayAmmount);
}

int pos = 0;

void readButtons()
{
    long time = millis();

    int button1Val = digitalRead(BUTTON_1);
    int button2Val = digitalRead(BUTTON_2);

    bool button1PressHappened = false;
    bool button2PressHappened = false;
    bool bothButtonsPressHappened = false;

    if (button1Val != HIGH) 
    {
        // button pressed
        if(button1_isPressed)
        {
        // been pressed before
        if(time - button1_pressStartedTime > repeatStart && time - button1_lastRepeatedTime > repeatPeriod)
        {
            button1_lastRepeatedTime = time;
            //button1PresHandler();
            button1PressHappened = true;
        }
        }
        else
        {
        // pressed for the first time
        button1_isPressed = true;
        button1_pressStartedTime = time;
        button1_lastRepeatedTime = 0;
        //button1PresHandler();
            button1PressHappened = true;
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
                //button2PresHandler();
                button2PressHappened = true;
            }
        }
        else
        {
        // pressed for the first time
        button2_isPressed = true;
        button2_pressStartedTime = time;
        button2_lastRepeatedTime = 0;
        //button2PresHandler();
            button2PressHappened = true;
        }
    }
    else
    {
        // button not pressed
        button2_isPressed = false;
        button2_pressStartedTime = 0;
        button2_lastRepeatedTime = 0;
    }

    if (button1_isPressed && button2_isPressed) 
    {
        // button pressed
        if(bothButtons_isPressed)
        {
            // been pressed before
            if(time - bothButtons_pressStartedTime > repeatStart && time - bothButtons_lastRepeatedTime > repeatPeriod)
            {
                bothButtons_lastRepeatedTime = time;
                bothButtonsPressHappened = true;
            }
        }
        else
        {
        // pressed for the first time
        bothButtons_isPressed = true;
        bothButtons_pressStartedTime = time;
        bothButtons_lastRepeatedTime = 0;
        //button1PresHandler();
            bothButtonsPressHappened = true;
        }
    }
    else
    {
        // button not pressed
        bothButtons_isPressed = false;
        bothButtons_pressStartedTime = 0;
        bothButtons_lastRepeatedTime = 0;
    }


    if(bothButtonsPressHappened)
    {
        bothButtonsPressHandler();
        Serial.println(mode);
    }
    else
    {
        if(button1PressHappened)
            button1PresHandler();

        if(button2PressHappened)
            button2PresHandler();
    }
}
