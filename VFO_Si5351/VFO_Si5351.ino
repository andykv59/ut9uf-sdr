
// UT9UF SDR Teensy 3.2 + Teensy Audio +Si5351
// VFO moduele based on Si5351 and TFT 1.8 SPI (ST7735 chip)
//
// TFT definition - this Teensy3 native optimized version requires specific pins
//

#include <asserts.h>
#include <errors.h>

#define tft_led 0    // TFT back LED we use pin 0
#define BACKLIGHT 0 // TFT back LED we use pin 0
#define sclk 14      // SCLK can also use pin 14
#define mosi 7       // MOSI can also use pin 7
#define cs   2       // CS & DC can use pins 2, 6, 9, 10, 15, 20, 21, 22, 23
#define dc   3       //  but certain pairs must NOT be used: 2+10, 6+9, 20+23, 21+22
#define rst  1       // RST can use any pin
//#define sdcs 4     // CS for SD card, can use any pin


//SPI connections for Banggood 1.8" display
//const int8_t sclk   = 14;
//const int8_t mosi   = 7;
//const int8_t cs     = 2;
//const int8_t dc     = 3;
//const int8_t rst    = 1;

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>             // SPI comms for TFT
#include <Wire.h>            // I2C comms for Si5351
//#include <si5351.h>        // Si5351 library github.com/NT7S/Si5351
#include <Adafruit_SI5351.h> // Si5351 library from Adafruit
#include <Encoder.h>         // Encoder 
#include "display.h"         // Display processing
#define SI5351

#ifdef SI5351
#include <si5351.h>
#endif

#ifdef SI5351
Si5351 si5351;
#endif

// TFT related
#if defined(__SAM3X8E__)
    #undef __FlashStringHelper::F(string_literal)
    #define F(string_literal) string_literal
#endif

// Option 1: use any pins but a little slower
Adafruit_ST7735 tft = Adafruit_ST7735(cs, dc, mosi, sclk, rst);

// Option 2: must use the hardware SPI pins
// (for UNO thats sclk = 13 and sid = 11) and pin 10 must be
// an output. This is much faster - also required if you want
// to use the microSD card (see the image drawing example)
//Adafruit_ST7735 tft = Adafruit_ST7735(cs, dc, rst);

// Rotary Encoder
// UI switch definitions
// encoder switch
Encoder tune(16, 17);
#define TUNE_STEP       5     // slow tuning rate 5hz steps
#define FAST_TUNE_STEP   500  // fast tuning rate 500hz steps

// Switches between pin and ground for USB/LSB/CW modes
const int8_t ModeSW =21;    // USB/LSB
const int8_t BandSW =20;    // band selector
const int8_t TuneSW =6;     // low for fast tune - encoder pushbutton


#define  DEBUG

// in receive mode we use an audio IF to avoid the noise, offset and hum below ~ 1khz
#define IF_FREQ 11000       // IF Oscillator frequency
#define CW_FREQ 700         // audio tone frequency used for CW

extern void setup_display(void);         // Initialize TFT
extern void show_band(String bandname);  // show band

// band selection stuff
struct band {
  long freq;
  String name;
};

#define SOFTROCK_40M

#ifdef SOFTROCK_40M

#define BAND_80M  0   // these can be in any order but indexes need to be sequential for band switching code
#define BAND_60M  1
#define BAND_49M  2
#define BAND_40M  3
#define BAND_31M  4
#define BAND_30M  5

#define FIRST_BAND BAND_80M
#define LAST_BAND  BAND_30M
#define NUM_BANDS  LAST_BAND - FIRST_BAND + 1

struct band bands[NUM_BANDS] = {
  3580000,"80M",
  5000000,"60M",
  6000000,"49M",
  7040000,"40M",
  9500000,"31M",
  10115000,"30M"
};

#define STARTUP_BAND BAND_40M    // 
#endif

 
void setup(void) {
  //pinMode(tft_led, OUTPUT);     // TFT back LED pin mode
  //digitalWrite(tft_led, HIGH);  // TFT back LED - on
  Serial.begin(9600);
#ifdef DEBUG
 while (!Serial) ; // wait for connection
 Serial.println("Initializing...");
#endif 
  Serial.print("hello!");

  pinMode(BACKLIGHT, INPUT_PULLUP); // yanks up display BackLight signal
  pinMode(ModeSW, INPUT_PULLUP);    // USB/LSB switch
  pinMode(BandSW, INPUT_PULLUP);    // filter width switch
  pinMode(TuneSW, INPUT_PULLUP);    // tuning rate = high
#ifdef DEBUG
  Serial.println("TFT light ON, switchess - PULLED-UP");
#endif

// initialize the TFT and show signon message etc
  SPI.setMOSI(7); // set up HW SPI for use with the audio card - alternate pins
  SPI.setSCK(14);  
  setup_display();
  
  // set up initial band and frequency
  show_band(bands[STARTUP_BAND].name);

  // set up clk gen
  si5351.init(SI5351_CRYSTAL_LOAD_8PF);
  si5351.set_correction(-100);  // I did a by ear correction to WWV
  // Set CLK0 to output 14 MHz with a fixed PLL frequency
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  si5351.set_freq((unsigned long)bands[STARTUP_BAND].freq * MASTER_CLK_MULT, SI5351_PLL_FIXED, SI5351_CLK0);
  

  // If your TFT's plastic wrap has a Black Tab, use the following:
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
  // If your TFT's plastic wrap has a Red Tab, use the following:
  //tft.initR(INITR_REDTAB);   // initialize a ST7735R chip, red tab
  // If your TFT's plastic wrap has a Green Tab, use the following:
  //tft.initR(INITR_GREENTAB); // initialize a ST7735R chip, green tab

  Serial.println("init");

  uint16_t time = millis();
  tft.fillScreen(ST7735_BLACK);
  time = millis() - time;

  Serial.println(time, DEC);
  delay(500);



  Serial.println("done");
  delay(1000);
}

void loop() {
  tft.invertDisplay(true);
  delay(500);
  tft.invertDisplay(false);
  delay(500);
}


