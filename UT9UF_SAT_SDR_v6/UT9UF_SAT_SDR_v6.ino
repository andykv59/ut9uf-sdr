
// UT9UF SDR Teensy 3.2 + Teensy Audio +Si5351
// VFO moduele based on Si5351 and TFT 2.8 SPI (tjctm24028-spi)
//
// TFT definition - this Teensy3 native optimized version requires specific pins
//
#define ver "v.00.06.00 github.com/andykv59/ut9uf-sdr"

#include <asserts.h>
#include <errors.h>

#define tft_led 0    // TFT back LED we use pin 0
#define BACKLIGHT 0 // TFT back LED we use pin 0
//#define sclk 14      // SCLK can also use pin 14
#define TFT_CLK 14      // SCLK can also use pin 14
//#define mosi 7       // MOSI can also use pin 7
#define TFT_MOSI 7       // MOSI can also use pin 7
//#define cs   2       // CS & DC can use pins 2, 6, 9, 10, 15, 20, 21, 22, 23


#define TFT_CS   2       // CS & DC can use pins 2, 6, 9, 10, 15, 20, 21, 22, 23
//#define dc   3       //  but certain pairs must NOT be used: 2+10, 6+9, 20+23, 21+22
#define TFT_DC   3       //  but certain pairs must NOT be used: 2+10, 6+9, 20+23, 21+22
//#define rst  1       // RST can use any pin
#define TFT_RST  1       // RST can use any pin
//#define sdcs 4     // CS for SD card, can use any pin
//int r = 0; // set TFT display rotation 0,1,2,3

//SPI connections for Banggood 1.8" display
//const int8_t sclk   = 14;
//const int8_t mosi   = 7;
//const int8_t cs     = 2;
//const int8_t dc     = 3;
//const int8_t rst    = 1;

#include <Metro.h>            //Comes with Teensy libs
#include <Audio.h>            //Comes with Teensy libs
#include <Adafruit_GFX.h>    // Core graphics library
//#include <Adafruit_ST7735.h> // TFT Hardware-specific library for 1,8 inch non-sensor display
#include <Adafruit_ILI9341.h> // TFT Hardware-specific library for 2,8 touch sensor display tjctm24028-spi
//#include <Adafruit_STMPE610.h> // TFT touch sensor lib for 2,8
#include <SPI.h>             // SPI comms for TFT
#include <Wire.h>            // I2C comms for Si5351
//#include "si5351.h"        // Si5351 library old one
#include <si5351.h>
//#include <Adafruit_SI5351.h> // Si5351 library from Adafruit
#include <Encoder.h>         // Encoder 
#include "display.h"         // Display processing
#include "filters.h"         // Audio filters

Si5351 si5351;              //Take an instance of Si5351

//#define MASTER_CLK_MULT  4  //SoftRock requires 4x clock
#define MASTER_CLK_MULT  1  //ut9uf-sdr requires 1x clock no sn74hc74 chip in use


// TFT related
//#if defined(__SAM3X8E__)
//    #undef __FlashStringHelper::F(string_literal)
//    #define F(string_literal) string_literal
//#endif

// Option 1: use any pins but a little slower
//Adafruit_ST7735 tft = Adafruit_ST7735(cd, dc, mosi, sclk, rst);

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST);

// Option 2: must use the hardware SPI pins
// (for UNO thats sclk = 13 and sid = 11) and pin 10 must be
// an output. This is much faster - also required if you want
// to use the microSD card (see the image drawing example)
//Adafruit_ST7735 tft = Adafruit_ST7735(cs, dc, rst);

// Rotary Encoder
// UI switch definitions
// encoder switch
Encoder tune(16, 17);
#define TUNE_STEP1       1     // slow tuning rate 1hz steps
#define TUNE_STEP2       25     // slow tuning rate 25hz steps
#define TUNE_STEP3       250     // fast tuning rate steps
#define FAST_TUNE_STEP   250  // fast tuning rate 250hz steps
int tunestep = TUNE_STEP1;
String tune_text="1 Hz Tune";  

// Switches between pin and ground for USB/LSB/CW modes
const int8_t ModeSW =21;    // USB/LSB
const int8_t BandSW =20;    // band selector
const int8_t TuneSW =6;     // low for fast tune - encoder pushbutton

#define  DEBUG  // if this mode is enabled - you MUST open port monitor in order to start radio

// Setup the phased mode of si5351 - CLK0 and CLK2 are in use and phase 90 degrees shift is done by si5351
// taken from http://py2ohh.w2c.com.br/
#define SI5351_PHASED

#ifdef SI5351_PHASED
double vfomhz;
int evendivisor = 100;
int oldevendivisor = 0;
#endif

float LPFcoeff = 0.3;               // used for low pass filtering the spectrum display


// in receive mode we use an audio IF to avoid the noise, offset and hum below ~ 1khz
#define IF_FREQ 11000       // IF Oscillator frequency
#define CW_FREQ 700         // audio tone frequency used for CW

//void sinewave(void);
extern void agc(void);                                        // RX agc function
extern void setup_display(void);
//extern void init_display(void);
extern void show_spectrum(float line_gain, float LPFcoeff, int mode);   // spectrum display draw
extern void show_waterfall(void);                             // waterfall display
//extern void show_radiomode(String mode);                      // show filter bandwidth now it's part of show_bandwidth()
extern void show_band(String bandname);                       // show band
extern void show_frequency(long int freq);                    // show frequency
extern void show_tunestep (String S);                         // show tune step
extern void show_bandwidth (int M, long int FU, long int FL); // show filter bandwidth
extern void show_notch (int notchF, int MODE);                // show notch


/*extern void setup_display(void);         // Initialize TFT
extern void intro_display(void);         // Show welcome screen
extern void main_display(void);          // Start main screen
extern void show_Smeter(void);           // Smeter display
extern void show_band(String bandname);  // show band
extern void agc(void);                   // RX agc function
extern void show_spectrum(void);         // spectrum display draw
extern void show_waterfall(void);        // waterfall display
extern void show_bandwidth(int filterwidth);  // show filter bandwidth
extern void show_radiomode(String mode); // show filter bandwidth
extern void show_frequency(long freq);   // show frequency
*/

// SW configuration defines
//#define SW_AGC   // define for Loftur's SW AGC - this has to be tuned carefully for your particular implementation
// codec hardware AGC works but it looks at the entire input bandwidth 
// ie codec HW AGC works on the strongest signal, not necessarily what you are listening to
// it should work well for ALC (mic input) though
#define HW_AGC // define for codec AGC 
//#define CW_WATERFALL // define for experimental CW waterfall - needs faster update rate
#define AUDIO_STATS    // shows audio library CPU utilization etc on serial console
#define DEBUG_PIN   4

// band selection stuff
struct band {
  long freq; // frequency in Hz
  String name; // name of band
  int mode;    // mode name
  int bandwidthU; // upper bandwidth
  int bandwidthL; // lower bandwidth
  int RFgain;
};

#define BAND_80M  0   // these can be in any order but indexes need to be sequential for band switching code
#define BAND_60M  1
#define BAND_49M  2
#define BAND_40M  3
#define BAND_31M  4
#define BAND_30M  5
#define BAND_20M  6
#define BAND_17M  7
#define BAND_15M  8
#define BAND_12M  9
#define BAND_10M  10

#define FIRST_BAND BAND_80M
#define LAST_BAND  BAND_10M
#define NUM_BANDS  LAST_BAND - FIRST_BAND + 1

// radio operation mode defines used for filter selections etc
#define modeAM        0
#define modeUSB       1
#define modeLSB       2
#define modeDSB       3
#define modeStereoAM  4
#define modeSAM       5   // not in use at the moment
#define firstmode modeAM
#define lastmode modeStereoAM
#define startmode modeAM

struct band bands[NUM_BANDS] = {
  3580000,"80M", modeLSB, 0000, 2400, 0,
  5000000,"60M", modeLSB, 0000, 2400, 0,
  6000000,"49M", modeLSB, 0000, 2400, 0,
  7000000,"40M", modeLSB, 0000, 2400, 0,
  9500000,"31M", modeUSB, 2400, 0000, 0,
  10100000,"30M", modeUSB, 2400, 0000, 0,
  14000000,"20M", modeUSB, 2400, 0000, 0,
  18068000,"17M", modeUSB, 2400, 0000, 0,
  21000000,"15M", modeUSB, 2400, 0000, 0,
  24890000,"12M", modeUSB, 2400, 0000, 0,
  28000000,"10M", modeUSB, 2400, 0000, 0, 
};

#define STARTUP_BAND BAND_40M    // 
int band = STARTUP_BAND;

#define  SSB_USB  0
#define  SSB_LSB  1
#define  CW       2
#define  CWR      3

//#define CW_WATERFALL // define for experimental CW waterfall - needs faster update rate

//---------------------------------------------------------------------------------------------------------
// audio definitions 
//  RX & TX audio input definitions
const int inputTX = AUDIO_INPUT_MIC;
const int inputRX = AUDIO_INPUT_LINEIN;

#define  INITIAL_VOLUME 1.0   // 0-1.0 output volume on startup
#define  CW_SIDETONE_VOLUME    0.25    // 0-1.0 level for CW TX mode
#define  SSB_SIDETONE_VOLUME    0.8   // 0-1.0 adjust to hear SSB TX audio thru the headphones
#define  MIC_GAIN      30      // mic gain in db 
#define  RX_LEVEL_I    1.0    // 0-1.0 adjust for RX I/Q balance
#define  RX_LEVEL_Q    1.0    // 0-1.0 adjust for RX I/Q balance
#define  SSB_TX_LEVEL_I    1.0   // 0-1.0 adjust for SSB TX I/Q balance
#define  SSB_TX_LEVEL_Q    0.956  // 0-1.0 adjust for SSB TX I/Q balance
#define  CW_TX_LEVEL_I    1.0   // 0-1.0 adjust for CW TX I/Q balance
#define  CW_TX_LEVEL_Q    0.956 // 0-1.0 adjust for CW TX I/Q balance

// channel assignments for output Audioselectors
// 
#define ROUTE_RX       0    // used for all recieve modes
#define ROUTE_SSB_TX      1    // SSB modes
#define ROUTE_CW_TX       2    // CW modes

// arrays used to set output audio routing and levels
// 3 radio modes x 4 channels for each audioselector
float audiolevels_I[3][4] = { 
  RX_LEVEL_I,0,0,0,        // RX mode channel levels
  0,SSB_TX_LEVEL_I,0,0,    // SSB TX mode channel levels
  0,0,CW_TX_LEVEL_I,0  //CW  TX mode channel levels
};

float audiolevels_Q[3][4] = { 
  RX_LEVEL_Q,0,0,0,        // RX mode channel levels
  0,SSB_TX_LEVEL_Q,0,0,    // SSB TX mode channel levels
  0,0,CW_TX_LEVEL_Q,0  //CW  TX mode channel levels
};

// Create the Audio components.  These should be created in the
// order data flows, inputs/sources -> processing -> outputs
//    
AudioInputI2S       audioinput;           // Audio Shield: mic or line-in

// FIR filters
AudioFilterFIR      Hilbert45_I;
AudioFilterFIR      Hilbert45_Q;
AudioFilterFIR      FIR_BPF;
AudioFilterFIR      postFIR;

AudioMixer4         Summer;        // Summer (add inputs)
AudioAnalyzeFFT256  myFFT;      // Spectrum Display
AudioSynthWaveform  IF_osc;           // Local Oscillator
AudioSynthWaveform  CW_tone_I;           // Oscillator for CW tone I (sine)
AudioSynthWaveform  CW_tone_Q;           // Oscillator for CW tone Q (cosine)
AudioEffectMultiply    Mixer;         // Mixer (multiply inputs)
AudioAnalyzePeak     Smeter;        // Measure Audio Peak for S meter
AudioMixer4         Audioselector_I;   // Summer used for AGC and audio switch
AudioMixer4         Audioselector_Q;   // Summer used as audio selector
AudioAnalyzePeak    AGCpeak;       // Measure Audio Peak for AGC use
AudioOutputI2S      audioOutput;   // Audio Shield: headphones & line-out

AudioControlSGTL5000 audioShield;  // Create an object to control the audio shield.

//---------------------------------------------------------------------------------------------------------
// Create Audio connections to build a software defined Radio Receiver
//
AudioConnection c1(audioinput, 0, Hilbert45_I, 0);// Audio inputs to +/- 45 degree filters
AudioConnection c2(audioinput, 1, Hilbert45_Q, 0);
AudioConnection c3(Hilbert45_I, 0, Summer, 0);    // Sum the shifted filter outputs to supress the image
AudioConnection c4(Hilbert45_Q, 0, Summer, 1);
//
AudioConnection c10(Summer, 0, myFFT, 0);         // FFT for spectrum display
AudioConnection c11(Summer,0, FIR_BPF, 0);        // 2.4 kHz USB or LSB filter centred at either 12.5 or 9.5 kHz
//                                                // ( local oscillator zero beat is at 11 kHz, see NCO )
AudioConnection c12(FIR_BPF, 0, Mixer, 0);        // IF from BPF to Mixer
AudioConnection c13(IF_osc, 0, Mixer, 1);            // Local Oscillator to Mixer (11 kHz)
//
AudioConnection c20(Mixer, 0, postFIR, 0);        // 2700Hz Low Pass filter or 200 Hz wide CW filter at 700Hz on audio output
AudioConnection c30(postFIR,0, Smeter, 0);        // RX signal S-Meter measurement point
//
// RX is mono output , but for TX we need I and Q audio channel output
// two summers (I and Q) on the output used to select different audio paths for different RX and TX modes
// 
AudioConnection c31(postFIR,0, Audioselector_I, ROUTE_RX);    // mono RX audio and AGC Gain loop adjust
AudioConnection c32(postFIR,0, Audioselector_Q, ROUTE_RX);    // mono RX audio to 2nd channel
AudioConnection c33(Hilbert45_I,0, Audioselector_I, ROUTE_SSB_TX);    // SSB TX I audio and ALC Gain loop adjust
AudioConnection c34(Hilbert45_Q,0, Audioselector_Q, ROUTE_SSB_TX);    // SSB TX Q audio and ALC Gain loop adjust
AudioConnection c35(CW_tone_I,0, Audioselector_I, ROUTE_CW_TX);    // CW TX I audio 
AudioConnection c36(CW_tone_Q,0, Audioselector_Q, ROUTE_CW_TX);    // CW TX Q audio
// note that last mixer input is unused - PSK mode ???
//
AudioConnection c40(Audioselector_I, 0, AGCpeak, 0);          // AGC Gain loop measure
AudioConnection c41(Audioselector_I, 0, audioOutput, 0);      // Output the sum on both channels 
AudioConnection c42(Audioselector_Q, 0, audioOutput, 1);
//---------------------------------------------------------------------------------------------------------

// various timers
Metro five_sec = Metro(5000); // Set up a 5 second Metro
Metro ms_100 = Metro(100);  // Set up a 100ms Metro
Metro ms_50 = Metro(50);  // Set up a 50ms Metro for polling switches
Metro lcd_upd = Metro(10);  // Set up a Metro for LCD updates
Metro CW_sample = Metro(10);  // Set up a Metro for LCD updates
//Metro Smetertimer = Metro(50); // Smeter timer

#ifdef CW_WATERFALL
Metro waterfall_upd = Metro(25);  // Set up a Metro for waterfall updates
#endif
 
void setup(void) {
  Serial.begin(9600);
  Serial.println("Initializing Radio...");
/*
  pinMode(tft_led, OUTPUT);     // TFT back LED pin mode
  digitalWrite(tft_led, HIGH);  // TFT back LED - on
*/

#ifdef DEBUG
 while (!Serial) ; // wait for connection
 Serial.println("Entering debug mode...");
 Serial.println("UT9UF-SAT-SDR, hello!");
#endif

  //pinMode(BACKLIGHT, INPUT_PULLUP); // yanks up display BackLight signal
  pinMode(ModeSW, INPUT_PULLUP);    // USB/LSB switch
  pinMode(BandSW, INPUT_PULLUP);    // filter width switch
  pinMode(TuneSW, INPUT_PULLUP);    // tuning rate = high
  
#ifdef DEBUG
  Serial.println("TFT light ON, switchess PULLUP ... done");
#endif

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(16);

  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.volume(INITIAL_VOLUME);
  audioShield.unmuteLineout();

#ifdef DEBUG
  Serial.println("Audio shield enabled ...");
#endif

#ifdef HW_AGC
  /* COMMENTS FROM Teensy Audio library:
    Valid values for dap_avc parameters
    maxGain; Maximum gain that can be applied
    0 - 0 dB
    1 - 6.0 dB
    2 - 12 dB
    lbiResponse; Integrator Response
    0 - 0 mS
    1 - 25 mS
    2 - 50 mS
    3 - 100 mS
    hardLimit
    0 - Hard limit disabled. AVC Compressor/Expander enabled.
    1 - Hard limit enabled. The signal is limited to the programmed threshold (signal saturates at the threshold)
    threshold
    floating point in range 0 to -96 dB
    attack
    floating point figure is dB/s rate at which gain is increased
    decay
    floating point figure is dB/s rate at which gain is reduced
  */
  audioShield.autoVolumeControl(2, 1, 0, -30, 3, 20); // see comments above
  audioShield.autoVolumeEnable();

#endif


// initialize the TFT and show signon message etc
  SPI.setMOSI(TFT_MOSI); // set up HW SPI for use with the audio card - alternate pins
  SPI.setSCK(TFT_CLK);  
  
  tft.begin();
  tft.setRotation(3);           // 1 = Normal  0 = 90° 2 = 90° 3 = up-side-down
  tft.fillScreen(BLACK);        // BLACK
  setup_display();
  
//  intro_display();
//  delay(4000);

//  main_display();

  
  //delay(4000);
  //tft.fillScreen(BLACK);
  //tft.setCursor(0, 115);
  //tft.drawFastVLine(80, 0,60,RED);
  
#ifdef DEBUG
  Serial.println("setup-display ... done");
#endif  


#ifdef DEBUG
  Serial.println("show initial band ... done");
  Serial.print("evendivisor is ");
  Serial.println(evendivisor);
#endif
// here startup message
  tft.setCursor(pos_x_frequency-6, pos_y_frequency);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.print("UT9UF AMSAT-SDR"); 
  tft.setCursor(pos_x_time,pos_y_time);
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.print(ver);             // Display Version
  delay (4000);
  tft.fillRect(pos_x_frequency-24, pos_y_frequency, 220, 16, BLACK);    // Clear startup text
  tft.fillRect(pos_x_time, pos_y_time, 240, 8, BLACK);                   // erase for time display

  
  // set up initial band and frequency
  show_band(bands[STARTUP_BAND].name);
  show_tunestep(tune_text);

// set up clk gen
  si5351.set_correction(0, SI5351_PLL_INPUT_XO);  // There is a calibration sketch in File/Examples/si5351Arduino-Jason where you can determine the correction by using the serial monitor
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
   
#ifdef SI5351_PHASED
  si5351.set_freq_manual((unsigned long)bands[STARTUP_BAND].freq * MASTER_CLK_MULT * 100ULL, evendivisor * (unsigned long)bands[STARTUP_BAND].freq * MASTER_CLK_MULT * 100ULL, SI5351_CLK0);
  si5351.set_freq_manual((unsigned long)bands[STARTUP_BAND].freq * MASTER_CLK_MULT * 100ULL, evendivisor * (unsigned long)bands[STARTUP_BAND].freq * MASTER_CLK_MULT * 100ULL, SI5351_CLK2);
  si5351.set_phase(SI5351_CLK0, 0);
  si5351.set_phase(SI5351_CLK2, evendivisor);
  si5351.pll_reset(SI5351_PLLA);
#else
  // Set CLK0 to output 14 MHz with a fixed PLL frequency
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
//  si5351.set_freq((unsigned long)bands[STARTUP_BAND].freq * MASTER_CLK_MULT, SI5351_PLL_FIXED, SI5351_CLK0);
  si5351.set_freq((unsigned long)bands[STARTUP_BAND].freq * MASTER_CLK_MULT * 100ULL, SI5351_CLK0);
#endif
 
#ifdef DEBUG
  Serial.println("si5351 and start frequency initialization ... done");
#endif 

//  setup_RX(SSB_USB);  // set up the audio chain for USB reception
  setup_RX(bands[band].mode, bands[band].bandwidthU, bands[band].bandwidthL);

#ifdef DEBUG
  Serial.println("setup_RX_USB audio chain skipped");
#endif 

#ifdef DEBUG
  Serial.println("audio RX path initialized");
#endif

}

void loop() {
//  static uint8_t mode=SSB_USB, modesw_state=0;
  static uint8_t modesw_state=0;
//  static uint8_t band=STARTUP_BAND, Bandsw_state=0;
  static uint8_t Bandsw_state=0;
  static long encoder_pos=0, last_encoder_pos=999;
  long encoder_change;

  // tune radio using encoder switch  

  encoder_pos=tune.read();
  if (encoder_pos != last_encoder_pos) {
    encoder_change=encoder_pos-last_encoder_pos;
    last_encoder_pos=encoder_pos; 
    
    // press encoder button for fast tuning
    if (digitalRead(TuneSW)) bands[band].freq+=encoder_change*tunestep;  // tune the master vfo - normal steps
    else bands[band].freq +=encoder_change*FAST_TUNE_STEP;  // fast tuning steps
    

#ifdef SI5351_PHASED
    vfomhz = bands[band].freq/10000;
#ifdef DEBUG
  Serial.print("VFOmhz is ");
  Serial.println(vfomhz);
#endif
    alteraevendivisor();   
    si5351.set_freq_manual((unsigned long)bands[band].freq * MASTER_CLK_MULT * 100ULL, evendivisor * (unsigned long)bands[band].freq * MASTER_CLK_MULT * 100ULL, SI5351_CLK0);
    si5351.set_freq_manual((unsigned long)bands[band].freq * MASTER_CLK_MULT * 100ULL, evendivisor * (unsigned long)bands[band].freq * MASTER_CLK_MULT * 100ULL, SI5351_CLK2);

    si5351.set_phase(SI5351_CLK0, 0);

#ifdef DEBUG
  Serial.print("evendivisor now ");
  Serial.println(evendivisor);
#endif
    
    si5351.set_phase(SI5351_CLK2, evendivisor);
    if (evendivisor != oldevendivisor) { //reset if evendivisor is changed
      si5351.pll_reset(SI5351_PLLA);
      oldevendivisor = evendivisor;
    }
#else
//    si5351.set_freq((unsigned long)bands[band].freq * MASTER_CLK_MULT, SI5351_PLL_FIXED, SI5351_CLK0);
    si5351.set_freq((unsigned long)bands[band].freq * MASTER_CLK_MULT * 100ULL, SI5351_CLK0);
#endif

    show_frequency(bands[band].freq + IF_FREQ);  // frequency we are listening to
  }

   // every 50 ms, adjust the volume and check the switches
  if (ms_50.check() == 1) {
    float vol = analogRead(15);
    vol = vol / 1023.0;
    audioShield.volume(vol);
    
//#ifdef DEBUG
//  Serial.println("audioShield volume skipped");
//#endif   
 
    if (!digitalRead(ModeSW)) {
       if (modesw_state==0) { // switch was pressed - falling edge
         if(++bands[band].mode > lastmode) bands[band].mode=firstmode;               // cycle thru radio modes 
//         if (bands[band].mode == modeUSB && notchF <=-400) notchF = notchF *-1;      // this flips the notch filter round, when you go from LSB --> USB and vice versa
//         if (bands[band].mode == modeLSB && notchF >=400) notchF = notchF *-1;
       
//         if(++mode > CWR) mode=SSB_USB; // cycle thru radio modes 
//         setup_RX(mode);  // set up the audio chain for new mode
           setup_RX(bands[band].mode, bands[band].bandwidthU, bands[band].bandwidthL);
           show_bandwidth(bands[band].mode, bands[band].bandwidthU, bands[band].bandwidthL);

#ifdef DEBUG
  Serial.println("setup_RX mode done");
#endif   
         
         modesw_state=1; // flag switch is pressed
       }
    }
    else modesw_state=0; // flag switch not pressed
  
    if (!digitalRead(BandSW)) {
       if (Bandsw_state==0) { // switch was pressed - falling edge
         if(++band > LAST_BAND) band=FIRST_BAND; // cycle thru radio bands 
         show_band(bands[band].name); // show new band
//         setup_mode(bands[band].mode); 
         setup_RX(bands[band].mode, bands[band].bandwidthU, bands[band].bandwidthL);

#ifdef SI5351_PHASED
    vfomhz = bands[band].freq/10000;
#ifdef DEBUG
  Serial.print("VFOmhz is ");
  Serial.println(vfomhz);
#endif
    alteraevendivisor();   
    si5351.set_freq_manual((unsigned long)bands[band].freq * MASTER_CLK_MULT * 100ULL, evendivisor * (unsigned long)bands[band].freq * MASTER_CLK_MULT * 100ULL, SI5351_CLK0);
    si5351.set_freq_manual((unsigned long)bands[band].freq * MASTER_CLK_MULT * 100ULL, evendivisor * (unsigned long)bands[band].freq * MASTER_CLK_MULT * 100ULL, SI5351_CLK2);

    si5351.set_phase(SI5351_CLK0, 0);
    si5351.set_phase(SI5351_CLK2, evendivisor);
    if (evendivisor != oldevendivisor) { //reset if evendivisor is changed
      si5351.pll_reset(SI5351_PLLA);
      oldevendivisor = evendivisor;
    }

#else
//         si5351.set_freq((unsigned long)bands[band].freq * MASTER_CLK_MULT, SI5351_PLL_FIXED, SI5351_CLK0);
           si5351.set_freq((unsigned long)bands[band].freq * MASTER_CLK_MULT * 100ULL, SI5351_CLK0);
#endif
         show_frequency(bands[band].freq + IF_FREQ);  // frequency we are listening to
         Bandsw_state=1; // flag switch is pressed
       }
    }
    else Bandsw_state=0; // flag switch not pressed  
  }    

#ifdef SW_AGC
  agc();  // Automatic Gain Control function
#endif  

  //
  // Draw Spectrum Display
  //
  //  if ((lcd_upd.check() == 1) && myFFT.available()) show_spectrum();
  if ((lcd_upd.check() == 1)) show_spectrum(bands[band].RFgain/10, LPFcoeff, bands[band].mode);
  //if ((Smetertimer.check() == 1)) show_Smeter();

  if ((CW_sample.check() == 1)) {
    digitalWrite(DEBUG_PIN,1); // for timing measurements
    delay(1);
    digitalWrite(DEBUG_PIN,0); // 
  }
  
#ifdef CW_WATERFALL
  if ((waterfall_upd.check() == 1) && myFFT.available()) show_waterfall();
#endif

#ifdef AUDIO_STATS
  //
  // DEBUG - Microcontroller Load Check
  //
  // Change this to if(1) to monitor load

  /*
  For PlaySynthMusic this produces:
  Proc = 20 (21),  Mem = 2 (8)
  */  
    if (five_sec.check() == 1)
    {
      Serial.print("Proc = ");
      Serial.print(AudioProcessorUsage());
      Serial.print(" (");    
      Serial.print(AudioProcessorUsageMax());
      Serial.print("),  Mem = ");
      Serial.print(AudioMemoryUsage());
      Serial.print(" (");    
      Serial.print(AudioMemoryUsageMax());
      Serial.println(")");
    }
#endif

}   // end of loop

// function to set up audio routes for the four channels on the two output audio summers 
// summers are used to change audio routing at runtime
// 
void Audiochannelsetup(int route)
{
  for (int i=0; i<4 ; ++i) {
      Audioselector_I.gain(i,audiolevels_I[route][i]); // set gains on audioselector channels
      Audioselector_Q.gain(i,audiolevels_Q[route][i]); 
  }
}

//void setup_RX(int mode)
void setup_RX(int MO, int bwu, int bwl)
{
  AudioNoInterrupts();   // Disable Audio while reconfiguring filters

  audioShield.inputSelect(inputRX); // RX mode uses line ins
  Audiochannelsetup(ROUTE_RX);   // switch audio path to RX processing chain

  CW_tone_I.amplitude(0);  // turn off cw oscillators to reduce cpu use
  CW_tone_Q.amplitude(0);
    
  // set IF oscillator to 11 kHz for RX
  // IF_osc.begin(1.0,IF_FREQ,TONE_TYPE_SINE); - obsolet due to new version of Teensey lib
  IF_osc.begin(1.0,IF_FREQ,WAVEFORM_SINE);
  
  // Initialize the wideband +/-45 degree Hilbert filters
  Hilbert45_I.begin(RX_hilbertm45,HILBERT_COEFFS);
  Hilbert45_Q.begin(RX_hilbert45,HILBERT_COEFFS);

  if ((MO == SSB_LSB) || (MO == CWR))             // LSB modes
    FIR_BPF.begin(firbpf_lsb,BPF_COEFFS);       // 2.4kHz LSB filter
  else FIR_BPF.begin(firbpf_usb,BPF_COEFFS);       // 2.4kHz USB filter

  switch (MO) {
    case modeDSB:  // to change - in reality this is CWR
      postFIR.begin(postfir_700,COEFF_700);     // 700 Hz LSB filter
//      show_bandwidth(LSB_NARROW);
        show_bandwidth(bands[band].mode, bands[band].bandwidthU, bands[band].bandwidthL);
        show_bandwidth(bands[band].mode, bands[band].bandwidthU, bands[band].bandwidthL);
//      show_radiomode("CWR");
    break;
    case modeLSB:
      postFIR.begin(postfir_lpf,COEFF_LPF);     // 2.4kHz LSB filter
//      show_bandwidth(LSB_WIDE);
        show_bandwidth(bands[band].mode, bands[band].bandwidthU, bands[band].bandwidthL);
//      show_radiomode("LSB");
    break;
    case modeAM:   // to change - in reality this is CW
      postFIR.begin(postfir_700,COEFF_700);     // 700 Hz CW filter
//      show_bandwidth(USB_NARROW);
        show_bandwidth(bands[band].mode, bands[band].bandwidthU, bands[band].bandwidthL);
//      show_radiomode("CW");
    break;
    case modeUSB:
      postFIR.begin(postfir_lpf,COEFF_LPF);     // 2.4kHz USB filter
//      show_bandwidth(USB_WIDE);
        show_bandwidth(bands[band].mode, bands[band].bandwidthU, bands[band].bandwidthL);
//      show_radiomode("USB");
    break;
  }
  AudioInterrupts(); 
}

// set up radio for TX modes - USB, LSB etc
void setup_TX(int mode)
{
  AudioNoInterrupts();   // Disable Audio while reconfiguring filters

  FIR_BPF.end(); // turn off the BPF - IF filters are not used in TX mode
  postFIR.end();     // turn off 2.4kHz post filter  

  switch (mode) {
    case CW:
      CW_tone_I.begin(1.0,CW_FREQ,WAVEFORM_SINE);
      CW_tone_Q.begin(1.0,CW_FREQ,WAVEFORM_SINE);
      CW_tone_Q.phase(90);
      Audiochannelsetup(ROUTE_CW_TX);   // switch audio outs to CW I & Q 
      audioShield.volume(CW_SIDETONE_VOLUME);   // fixed level for TX  
    break;
    case CWR:
      CW_tone_I.begin(1.0,CW_FREQ,WAVEFORM_SINE);
      CW_tone_Q.begin(1.0,CW_FREQ,WAVEFORM_SINE);
      CW_tone_I.phase(90);
      Audiochannelsetup(ROUTE_CW_TX);   // switch audio outs to CW I & Q 
      audioShield.volume(CW_SIDETONE_VOLUME);   // fixed level for TX  
    break;
    case SSB_USB:
      // Initialize the +/-45 degree Hilbert filters
      Hilbert45_I.begin(TX_hilbert45,HILBERT_COEFFS);
      Hilbert45_Q.begin(TX_hilbertm45,HILBERT_COEFFS);
      Audiochannelsetup(ROUTE_SSB_TX);   // switch audio outs to TX Hilbert filters
      audioShield.inputSelect(inputTX); // SSB TX mode uses mic in
      audioShield.micGain(MIC_GAIN);  // have to adjust mic gain after selecting mic in
      audioShield.volume(SSB_SIDETONE_VOLUME);   // fixed level for TX  
    break;
    case SSB_LSB:
  // Initialize the +/-45 degree Hilbert filters
      Hilbert45_I.begin(TX_hilbertm45,HILBERT_COEFFS); // swap filters for LSB mode
      Hilbert45_Q.begin(TX_hilbert45,HILBERT_COEFFS);
      Audiochannelsetup(ROUTE_SSB_TX);   // switch audio outs to TX Hilbert filters
      audioShield.inputSelect(inputTX); // SSB TX mode uses mic in
      audioShield.micGain(MIC_GAIN); // have to adjust mic gain after selecting mic in
      audioShield.volume(SSB_SIDETONE_VOLUME);   // fixed level for TX  
    break;
  }
  AudioInterrupts(); 
}

#ifdef SI5351_PHASED
// tricky set of vars for phased mode of si5351
void alteraevendivisor()
{
  if (vfomhz < 685) {
    evendivisor = 126;
  }
  if ((vfomhz >= 685) && (vfomhz < 950)) {
    evendivisor = 88;
  }
  if ((vfomhz >= 950) && (vfomhz < 1360)) {
    evendivisor = 64;
  }
  if ((vfomhz >= 1360) && (vfomhz < 1750)) {
    evendivisor = 44;
  }
  if ((vfomhz >= 1750) && (vfomhz < 2500)) {
    evendivisor = 34;
  }
  if ((vfomhz >= 2500) && (vfomhz < 3600)) {
    evendivisor = 24;
  }
  if ((vfomhz >= 3600) && (vfomhz < 4500)) {
    evendivisor = 18;
  }
  if ((vfomhz >= 4500) && (vfomhz < 6000)) {
    evendivisor = 14;
  }
  if ((vfomhz >= 6000) && (vfomhz < 8000)) {
    evendivisor = 10;
  }
  if ((vfomhz >= 8000) && (vfomhz < 10000)) {
    evendivisor = 8;
  }
  if ((vfomhz >= 10000) && (vfomhz < 14660)) {
    evendivisor = 6;
  }
  if ((vfomhz >= 15000) && (vfomhz < 22000)) {
    evendivisor = 4;
  }
}
#endif
