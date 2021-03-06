// code for the TFT display

#include "analyze_fft256iq.h"
#include <Metro.h>
#include "display.h"
#include <Audio.h>
#include <Adafruit_GFX.h>        // LCD Core graphics library
#include <Adafruit_ILI9341.h>
extern Adafruit_ILI9341 tft;

int pos_centre_f = 200; // Rx above IF, IF = 5515 190

extern AudioAnalyzeFFT256IQ  myFFT;
extern int ExtractDigit(long int n, int k);                  // Extract digits

float uv, uvold, dbuv, dbm, s;// microvolts, db-microvolts, s-units, volts, dbm, sold = old S-meter value, deltas = abs difference between sold and s
float dbmold = 0;
char string[80];  
int le, bwhelp, left, lsb; 
int pixelold[320] = {0};
int pixelnew[320];
int Ubn, Lbn;
uint8_t sch = 0;
bool freq_flag = 0;
int oldnotchF = 10000;
uint8_t digits_old [] = {9,9,9,9,9,9,9,9,9,9};

int oldM = 9;               // display Mode yes or no
long int oldBW, tstBW = 0;  // display BW yes or no

Metro Smetertimer = Metro(50);
  
//extern AudioAnalyzeFFT256  myFFT;      // FFT for Spectrum Display

/*
void init_display(void) {
    tft.initR(INITR_BLACKTAB);
}
*/

/****************************************************************************** 
 *      S E T U P    D I S P L A Y                                            *
 ******************************************************************************/
void setup_display(void) {
  tft.setTextColor(WHITE);
  tft.setTextWrap(true);

  freq_flag = 0;                // reset to display frequency
    
// Show mid screen tune position
  tft.drawFastVLine(pos_centre_f, 0,pos_spectrum+1, RED); //WHITE);

// draw S-Meter layout
  tft.drawFastHLine (pos_x_smeter, pos_y_smeter-1, 9*s_w, WHITE);
  tft.drawFastHLine (pos_x_smeter, pos_y_smeter+3, 9*s_w, WHITE);
  tft.fillRect(pos_x_smeter, pos_y_smeter-3, 2, 2, WHITE);
  tft.fillRect(pos_x_smeter+8*s_w, pos_y_smeter-3, 2, 2, WHITE);
  tft.fillRect(pos_x_smeter+2*s_w, pos_y_smeter-3, 2, 2, WHITE);
  tft.fillRect(pos_x_smeter+4*s_w, pos_y_smeter-3, 2, 2, WHITE);
  tft.fillRect(pos_x_smeter+6*s_w, pos_y_smeter-3, 2, 2, WHITE);
  tft.fillRect(pos_x_smeter+7*s_w, pos_y_smeter-4, 2, 3, WHITE);
  tft.fillRect(pos_x_smeter+3*s_w, pos_y_smeter-4, 2, 3, WHITE);
  tft.fillRect(pos_x_smeter+5*s_w, pos_y_smeter-4, 2, 3, WHITE);
  tft.fillRect(pos_x_smeter+s_w, pos_y_smeter-4, 2, 3, WHITE);
  tft.fillRect(pos_x_smeter+9*s_w, pos_y_smeter-4, 2, 3, WHITE);
  tft.drawFastHLine (pos_x_smeter+9*s_w, pos_y_smeter-1, 3*s_w*2+2, GREEN);
  tft.drawFastHLine (pos_x_smeter+9*s_w, pos_y_smeter+3, 3*s_w*2+2, GREEN);
  tft.fillRect(pos_x_smeter+11*s_w, pos_y_smeter-4, 2, 3, GREEN);
  tft.fillRect(pos_x_smeter+13*s_w, pos_y_smeter-4, 2, 3, GREEN);
  tft.fillRect(pos_x_smeter+15*s_w, pos_y_smeter-4, 2, 3, GREEN);
  tft.drawFastVLine (pos_x_smeter-1, pos_y_smeter-1, 5, WHITE); 
  tft.drawFastVLine (pos_x_smeter+15*s_w+2, pos_y_smeter-1, 5, GREEN);

  tft.setCursor(pos_x_smeter - 4, pos_y_smeter - 13);
  tft.setTextColor(WHITE);
  tft.setTextWrap(true);
  tft.print("S 1");
  tft.setCursor(pos_x_smeter + 28, pos_y_smeter - 13);
  tft.print("3");
  tft.setCursor(pos_x_smeter + 48, pos_y_smeter - 13);
  tft.print("5");
  tft.setCursor(pos_x_smeter + 68, pos_y_smeter - 13);
  tft.print("7");
  tft.setCursor(pos_x_smeter + 88, pos_y_smeter - 13);
  tft.print("9");
  tft.setCursor(pos_x_smeter + 120, pos_y_smeter - 13);
  tft.print("+20dB");
} // end void setupdisplay


/****************************************************************************** 
 *      D R A W    S P E C T R U M                                            *
 ******************************************************************************/
// draw the spectrum display and S-meter
// this version draws 1/10 of the spectrum per call (32 pixels) but we run it 10x the speed
// this allows other stuff to run without blocking for so long

void show_spectrum(float line_gain, float LPFcoeff, int M) {
  static int startx=0, endx;
  endx = startx + 32;     // was 16
  int scale=3;
  float avg = 0.0;
  
// Draw pixels of spectrum display  mFFT.output index < 256
//
  for (int16_t x=startx; x < endx; x+=1) {
    if ((x > 1) && (x < 318))                 // was 159
// moving window - weighted average of 5 points of the spectrum to smooth spectrum in the frequency domain
// weights:  x: 50% , x-1/x+1: 36%, x+2/x-2: 14% 
      avg = myFFT.output[(x)*32/40]*0.5 + myFFT.output[(x-1)*32/40]*0.18 + myFFT.output[(x-2)*32/40]*0.07 + myFFT.output[(x+1)*32/40]*0.18 + myFFT.output[(x+2)*32/40]*0.07;
    else 
      avg =  myFFT.output[(x)*32/40];
// pixelnew[x] = LPFcoeff * 2 * sqrt (abs(myFFT.output[(x)*16/10])*scale) + (1 - LPFcoeff) * pixelold[x];
// low pass filtering of the spectrum pixels to smooth/slow down spectrum in the time domain
// experimental LPF for spectrum:  ynew = LPCoeff * x + (1-LPCoeff) * yprevious; here: A = 0.3 to 0.5 seems to be a good idea
    pixelnew[x] = LPFcoeff * 2 * sqrt (abs(avg)*scale) + (1 - LPFcoeff) * pixelold[x];
                
//  alternate way: draw pixels
//  only plot pixel, if at a new position
    if(x != pos_centre_f) {
      if (pixelold[x] != pixelnew[x]) { 
        tft.drawPixel(x, pos_spectrum-1-pixelold[x], BLACK);    // delete old pixel
        tft.drawPixel(x, pos_spectrum-1-pixelnew[x], WHITE);    // write new pixel
        pixelold[x] = pixelnew[x];
      }
    }
  }

/****************************************************************************** 
 *       D R A W   S - M E T E R                                              *
 ******************************************************************************/
// Calculate S units. 50uV = S9
// measure signal strength of carrier of AM Transmission at exact frequency
// Exact freq = 256/2 + (4411/binBW) = 153 = posbin  (See display.h)
// For LSB and USB take the max signal on 1200Hz (which is half of the normal SSB bandwidth) from carrier freq
//
  Lbn = posbin - (1200/binBW);
  Ubn = posbin + (1200/binBW);
  
  if (Smetertimer.check()==1) {
    uv = myFFT.output[posbin]+myFFT.output[posbin+1]+myFFT.output[posbin+2]+myFFT.output[posbin+3]+myFFT.output[posbin-1]+myFFT.output[posbin-2]+myFFT.output[posbin-3];
    if (M == 1)  // USB
      uv = myFFT.output[Ubn]+myFFT.output[Ubn+1]+myFFT.output[Ubn+2]+myFFT.output[Ubn+3]+myFFT.output[Ubn-1]+myFFT.output[Ubn-2]+myFFT.output[Ubn-3];
    else if (M == 2)  // LSB
      uv = myFFT.output[Lbn]+myFFT.output[Lbn+1]+myFFT.output[Lbn+2]+myFFT.output[Lbn+3]+myFFT.output[Lbn-1]+myFFT.output[Lbn-2]+myFFT.output[Lbn-3];
    
// low pass filtering for Smeter values 
    uv = 0.1 * uv + 0.9 * uvold;
      
    if (uv == 0) dbm = -130;
    else {
      dbm = 20*log10(uv)-83.5-25.7-7.5-1.5*line_gain;     //dbm standardized to 15.26Hz Receiver Bandwidth  calibrate -7.5 dB
    }
      
// constant 83.5dB determined empirically by measuring a carrier with a Perseus SDR
// and comparing the output of the Teensy FFT
// 25.7dB is INA163 gain in frontend 
// dbm measurement on the Perseus standardized to RBW of 15.26Hz 
// float vol = analogRead(15);
// vol = vol / 1023.0;
// now calculate S-value from dbm

    s = 9.0 + ((dbm + 73.0) / 6.0);
    if (s <0.0) s=0.0;
    if ( s > 9.0) {
      dbuv = dbm + 73.0;
      s = 9.0;
    }
    else dbuv = 0.0;
     
    tft.drawFastHLine(pos_x_smeter, pos_y_smeter, s*s_w+1, BLUE);
    tft.drawFastHLine(pos_x_smeter+s*s_w+1, pos_y_smeter, (9*s_w+1)-s*s_w+1, BLACK);
    tft.drawFastHLine(pos_x_smeter, pos_y_smeter+1, s*s_w+1, WHITE);
    tft.drawFastHLine(pos_x_smeter+s*s_w+1, pos_y_smeter+1, (9*s_w+1)-s*s_w+1, BLACK);
    tft.drawFastHLine(pos_x_smeter, pos_y_smeter+2, s*s_w+1, BLUE);
    tft.drawFastHLine(pos_x_smeter+s*s_w+1, pos_y_smeter+2, (9*s_w+1)-s*s_w+1, BLACK);

    if(dbuv>30) dbuv=30;
    tft.drawFastHLine(pos_x_smeter+9*s_w+1, pos_y_smeter, (dbuv/5)*s_w+1, RED);
    tft.drawFastHLine(pos_x_smeter+9*s_w+(dbuv/5)*s_w+1, pos_y_smeter, (6*s_w+1)-(dbuv/5)*s_w, BLACK);
    tft.drawFastHLine(pos_x_smeter+9*s_w+1, pos_y_smeter+1, (dbuv/5)*s_w+1, RED);
    tft.drawFastHLine(pos_x_smeter+9*s_w+(dbuv/5)*s_w+1, pos_y_smeter+1, (6*s_w+1)-(dbuv/5)*s_w, BLACK);
    tft.drawFastHLine(pos_x_smeter+9*s_w+1, pos_y_smeter+2, (dbuv/5)*s_w+1, RED);
    tft.drawFastHLine(pos_x_smeter+9*s_w+(dbuv/5)*s_w+1, pos_y_smeter+2, (6*s_w+1)-(dbuv/5)*s_w, BLACK);

    uvold = uv;
  } // end if (Smeter Timer)   
  
  startx+=32;
  if (startx >=320) startx=0;
}  // end of Spectrum display


/****************************************************************************** 
 *     D R A W    W A T E R F A L L                                           *
 ******************************************************************************/
// waterfall dsiplay
void show_waterfall(void) {
  // experimental waterfall display for CW -
  // this should probably be on a faster timer since it needs to run as fast as possible to catch CW edges
  //  FFT bins are 22khz/128=171hz wide 
  // cw peak should be around 11.6khz - 
  static uint16_t waterfall[80];  // array for simple waterfall display
  static uint8_t w_index=0,w_avg;
  waterfall[w_index]=0;
  for (uint8_t y=66;y<67;++y)  // sum of bin powers near cursor - usb only
      waterfall[w_index]+=(uint8_t)(abs(myFFT.output[y])); // store bin power readings in circular buffer
  waterfall[w_index]|= (waterfall[w_index]<<5 |waterfall[w_index]<<11); // make it colorful
  int8_t p=w_index;
  for (uint8_t x=158;x>0;x-=2) {
    tft.fillRect(x,65,2,4,waterfall[p]);
    if (--p<0 ) p=79;
  }
  if (++w_index >=80) w_index=0; 
}

/******************************************************************************** 
 *    S H O W    B A N D W I D T H ,  M O D E  A N D  5 K H z  M A R K E R S    *
 ********************************************************************************/
void show_bandwidth (int M, long int FU, long int FL) {
  uint8_t zaehler;
  uint8_t digits[4];
  
  tstBW = FU + FL;          // to test change of BW
  
// print mode only if necessary
  if (M != oldM)  {
    tft.setTextSize(2);
    tft.setTextColor(GREEN);
    tft.fillRect(pos_x_mode, pos_y_mode, 64, 16, BLACK);  // erase old mode string
    tft.setCursor(pos_x_mode, pos_y_mode);  
  
    switch (M) {
    case 0: //AM
      tft.print("AM"); 
      break;
    case 3: //DSB
      tft.print("DSB"); 
      break;
    case 4: //StereoAM
      tft.print("SAM"); 
      break;
    case 2: //LSB
      tft.print("LSB"); 
      break;
    case 1:  //USB
      tft.print("USB"); 
      break;
    } // end switch
  oldM = M;
  }    

    bwhelp = FU /100;                       // BW Upper
    int leU = bwhelp*32/spectrum_span;      // Max 58
    bwhelp = FL /100;                       // BW Lower
    int leL = bwhelp*32/spectrum_span;      // Max 58
    float kHz = (FU + FL) / 1000.0;

// print BW text  
//    tft.fillRect(pos_x_mode, pos_y_frequency+3, 72, 22, BLACK); // erase old BW string
//    tft.setCursor(pos_x_mode, pos_y_frequency+3);
//    sprintf(string,"%02.1fk",kHz);
//    tft.setTextSize(2);
//    tft.setTextColor(GREEN);
//    tft.print(string);

    zaehler = 3;

    while (zaehler--) {             // Get BW digits
      digits[zaehler] = ExtractDigit(kHz*10, zaehler);
      Serial.print(digits[zaehler]);
    }
    Serial.println(kHz);
//  2: 10KHz  1: 1KHz, 0: 100Hz

    tft.setTextSize(2);
    tft.setTextColor(GREEN);  
    zaehler = 3;
    if (digits[2] != 0) sch = 12;  else sch = 0;
    
    while (zaehler--) {                                       // counts from 2 to 0
      if (digits[zaehler] != digits_old[zaehler]) {           // digit has changed 
        if (zaehler == 2 && digits[2] != 0)  {                // print only when > 0
          tft.setCursor(pos_x_mode, pos_y_frequency);         // set print position allways 0
          tft.fillRect( pos_x_mode, pos_y_frequency, 12, 16, BLACK);            // delete old digit
          tft.print(digits[2]);                               // write new digit
        }
        if (zaehler == 1) {
          tft.setCursor(pos_x_mode + sch, pos_y_frequency);   // set print position 0 or 12
          tft.fillRect( pos_x_mode + sch, pos_y_frequency, 12, 16, BLACK);    // delete old digit
          tft.print(digits[1]);                               // write new digit
        }
        if (zaehler == 0) {
          tft.setCursor(pos_x_mode + sch + 24, pos_y_frequency);   // set print position 24 or 36
          tft.fillRect( pos_x_mode + sch + 24, pos_y_frequency, 12, 16, BLACK);    // delete old digit
          tft.print(digits[0]);                               // write new digit
        }
        digits_old[zaehler] = digits[zaehler]; 
      }
    }
// print period
    sch = sch + 12;
    tft.setCursor(pos_x_mode + sch, pos_y_frequency);                 // set print position
    tft.fillRect( pos_x_mode + sch, pos_y_frequency, 12, 16, BLACK);  // delete old digit
    tft.print(".");
// print 'K'
    tft.setCursor(pos_x_mode + sch + 24, pos_y_frequency);                 // set print position
    tft.fillRect( pos_x_mode + sch + 24, pos_y_frequency, 24, 16, BLACK);  // delete old digits
    tft.print("K");    
  
// draw upper sideband indicator
    tft.drawFastHLine(pos_centre_f, pos_spectrum + 1, leU, RED);
    tft.drawFastHLine(pos_centre_f, pos_spectrum + 2, leU, RED);
    tft.drawFastHLine(pos_centre_f, pos_spectrum + 3, leU, RED);
    tft.drawFastHLine(pos_centre_f, pos_spectrum, leU, RED);
    tft.drawFastHLine(pos_centre_f+leU, pos_spectrum + 1, 58-leU, BLACK);    // erase rest
    tft.drawFastHLine(pos_centre_f+leU, pos_spectrum + 2, 58-leU, BLACK);
    tft.drawFastHLine(pos_centre_f+leU, pos_spectrum + 3, 58-leU, BLACK);
    tft.drawFastHLine(pos_centre_f+leU, pos_spectrum, 58-leU, BLACK);
  
// draw lower sideband indicator   
    left = pos_centre_f - leL; 
    tft.drawFastHLine(left+1, pos_spectrum + 1, leL, RED);
    tft.drawFastHLine(left+1, pos_spectrum + 2, leL, RED);
    tft.drawFastHLine(left+1, pos_spectrum + 3, leL, RED);
    tft.drawFastHLine(left+1, pos_spectrum, leL, RED);
    tft.drawFastHLine(pos_centre_f-57, pos_spectrum + 1, 58-leL, BLACK);        // erase rest
    tft.drawFastHLine(pos_centre_f-57, pos_spectrum + 2, 58-leL, BLACK);
    tft.drawFastHLine(pos_centre_f-57, pos_spectrum + 3, 58-leL, BLACK);
    tft.drawFastHLine(pos_centre_f-57, pos_spectrum, 58-leL, BLACK);

// draw 9 yellow points each 36 pixels = 5 KHz
// spectrum_span = 44.117
    for (int16_t x=20; x < 320; x+=36) {
      if (x != pos_centre_f) tft.fillRect(x, pos_spectrum, 2, 3, YELLOW);
    }

  tft.setTextColor(WHITE); // set text color to white for other print routines not to get confused ;-)
  tft.setTextSize(1);
}  // end of show bandwidth, mode and markers



/****************************************************************************** 
 *      S H O W    T U N E S T E P                                            *
 ******************************************************************************/
// show tunestep
void show_tunestep(String S) {
  char string[80]; 
  tft.fillRect(pos_x_menu, pos_y_menu, 70, 10, BLACK);   // erase old string
  tft.setTextColor(YELLOW);                              // changed to yellow, was white
  tft.setCursor(pos_x_menu, pos_y_menu);
  tft.print(S);
  tft.setTextColor(WHITE);
}


/****************************************************************************** 
 *      S H O W    B A N D N A M E                                            *
 ******************************************************************************/
// show band
void show_band(String bandname) {
//  tft.fillRect(100, 85, 19, 7, BLACK);        // erase old string
//  tft.setTextColor(WHITE);
//  tft.setCursor(100, 85);
//  tft.print(bandname);
}


/****************************************************************************** 
 *      S H O W    N O T C H                                                  *
 ******************************************************************************/
void show_notch(int notchF, int MODE) {
// pos_centre_f is the x position of the Rx centre
// pos_spectrum is the y position of the spectrum display 
// notch display should be at x = pos_centre_f +- notch frequency and y = 20 
//  LSB: 
  pos_centre_f+=1; // = pos_centre_f + 1;
// delete old indicator
  tft.drawFastVLine(pos_centre_f +1 + 320/spectrum_span * oldnotchF / 1000, notchpos, notchL, BLACK);
  tft.drawFastVLine(pos_centre_f    + 320/spectrum_span * oldnotchF / 1000, notchpos, notchL, BLACK);
  tft.drawFastVLine(pos_centre_f -1 + 320/spectrum_span * oldnotchF / 1000, notchpos, notchL, BLACK);
  tft.drawFastHLine(pos_centre_f -4 + 320/spectrum_span * oldnotchF / 1000, notchpos+notchL, 9, BLACK);
  tft.drawFastHLine(pos_centre_f -3 + 320/spectrum_span * oldnotchF / 1000, notchpos+notchL + 1, 7, BLACK);
  tft.drawFastHLine(pos_centre_f -2 + 320/spectrum_span * oldnotchF / 1000, notchpos+notchL + 2, 5, BLACK);
  tft.drawFastHLine(pos_centre_f -1 + 320/spectrum_span * oldnotchF / 1000, notchpos+notchL + 3, 3, BLACK);
  tft.drawFastVLine(pos_centre_f    + 320/spectrum_span * oldnotchF / 1000, notchpos+notchL + 4, 2, BLACK);

  tft.drawFastVLine(pos_centre_f +1 - 320/spectrum_span * oldnotchF / 1000, notchpos, notchL, BLACK);
  tft.drawFastVLine(pos_centre_f    - 320/spectrum_span * oldnotchF / 1000, notchpos, notchL, BLACK);
  tft.drawFastVLine(pos_centre_f -1 - 320/spectrum_span * oldnotchF / 1000, notchpos, notchL, BLACK);
  tft.drawFastHLine(pos_centre_f -4 - 320/spectrum_span * oldnotchF / 1000, notchpos+notchL, 9, BLACK);
  tft.drawFastHLine(pos_centre_f -3 - 320/spectrum_span * oldnotchF / 1000, notchpos+notchL + 1, 7, BLACK);
  tft.drawFastHLine(pos_centre_f -2 - 320/spectrum_span * oldnotchF / 1000, notchpos+notchL + 2, 5, BLACK);
  tft.drawFastHLine(pos_centre_f -1 - 320/spectrum_span * oldnotchF / 1000, notchpos+notchL + 3, 3, BLACK);
  tft.drawFastVLine(pos_centre_f    - 320/spectrum_span * oldnotchF / 1000, notchpos+notchL + 4, 2, BLACK);

  tft.drawFastVLine(pos_centre_f - 1, 0,pos_spectrum + 1, RED); // Rewrite mid screen tune position

  if (notchF >= 400 || notchF <= -400) {
// draw new indicator according to mode
    switch (MODE)  {
      case 2: //modeLSB:
          tft.drawFastVLine(pos_centre_f +1 - 320/spectrum_span * notchF / -1000, notchpos, notchL, notchColour);
          tft.drawFastVLine(pos_centre_f    - 320/spectrum_span * notchF / -1000, notchpos, notchL, notchColour);
          tft.drawFastVLine(pos_centre_f -1 - 320/spectrum_span * notchF / -1000, notchpos, notchL, notchColour);
          tft.drawFastHLine(pos_centre_f -4 - 320/spectrum_span * notchF / -1000, notchpos+notchL, 9, notchColour);
          tft.drawFastHLine(pos_centre_f -3 - 320/spectrum_span * notchF / -1000, notchpos+notchL + 1, 7, notchColour);
          tft.drawFastHLine(pos_centre_f -2 - 320/spectrum_span * notchF / -1000, notchpos+notchL + 2, 5, notchColour);
          tft.drawFastHLine(pos_centre_f -1 - 320/spectrum_span * notchF / -1000, notchpos+notchL + 3, 3, notchColour);
          tft.drawFastVLine(pos_centre_f    - 320/spectrum_span * notchF / -1000, notchpos+notchL + 4, 2, notchColour);
      break;
      case 1: //modeUSB:
          tft.drawFastVLine(pos_centre_f +1 + 320/spectrum_span * notchF / 1000, notchpos, notchL, notchColour);
          tft.drawFastVLine(pos_centre_f    + 320/spectrum_span * notchF / 1000, notchpos, notchL, notchColour);
          tft.drawFastVLine(pos_centre_f -1 + 320/spectrum_span * notchF / 1000, notchpos, notchL, notchColour);
          tft.drawFastHLine(pos_centre_f -4 + 320/spectrum_span * notchF / 1000, notchpos+notchL, 9, notchColour);
          tft.drawFastHLine(pos_centre_f -3 + 320/spectrum_span * notchF / 1000, notchpos+notchL + 1, 7, notchColour);
          tft.drawFastHLine(pos_centre_f -2 + 320/spectrum_span * notchF / 1000, notchpos+notchL + 2, 5, notchColour);
          tft.drawFastHLine(pos_centre_f -1 + 320/spectrum_span * notchF / 1000, notchpos+notchL + 3, 3, notchColour);
          tft.drawFastVLine(pos_centre_f    + 320/spectrum_span * notchF / 1000, notchpos+notchL + 4, 2, notchColour);
      break;
      case 0: // modeAM:
          tft.drawFastVLine(pos_centre_f +1 + 320/spectrum_span * notchF / 1000, notchpos, notchL, notchColour);
          tft.drawFastVLine(pos_centre_f    + 320/spectrum_span * notchF / 1000, notchpos, notchL, notchColour);
          tft.drawFastVLine(pos_centre_f -1 + 320/spectrum_span * notchF / 1000, notchpos, notchL, notchColour);
          tft.drawFastHLine(pos_centre_f -4 + 320/spectrum_span * notchF / 1000, notchpos+notchL, 9, notchColour);
          tft.drawFastHLine(pos_centre_f -3 + 320/spectrum_span * notchF / 1000, notchpos+notchL + 1, 7, notchColour);
          tft.drawFastHLine(pos_centre_f -2 + 320/spectrum_span * notchF / 1000, notchpos+notchL + 2, 5, notchColour);
          tft.drawFastHLine(pos_centre_f -1 + 320/spectrum_span * notchF / 1000, notchpos+notchL + 3, 3, notchColour);
          tft.drawFastVLine(pos_centre_f    + 320/spectrum_span * notchF / 1000, notchpos+notchL + 4, 2, notchColour);

          tft.drawFastVLine(pos_centre_f +1 - 320/spectrum_span * notchF / 1000, notchpos, notchL, notchColour);
          tft.drawFastVLine(pos_centre_f    - 320/spectrum_span * notchF / 1000, notchpos, notchL, notchColour);
          tft.drawFastVLine(pos_centre_f -1 - 320/spectrum_span * notchF / 1000, notchpos, notchL, notchColour);
          tft.drawFastHLine(pos_centre_f -4 - 320/spectrum_span * notchF / 1000, notchpos+notchL, 9, notchColour);
          tft.drawFastHLine(pos_centre_f -3 - 320/spectrum_span * notchF / 1000, notchpos+notchL + 1, 7, notchColour);
          tft.drawFastHLine(pos_centre_f -2 - 320/spectrum_span * notchF / 1000, notchpos+notchL + 2, 5, notchColour);
          tft.drawFastHLine(pos_centre_f -1 - 320/spectrum_span * notchF / 1000, notchpos+notchL + 3, 3, notchColour);
          tft.drawFastVLine(pos_centre_f    - 320/spectrum_span * notchF / 1000, notchpos+notchL + 4, 2, notchColour);
      break;
      case 3: //modeDSB:
      case 4: //modeStereoAM:
        if (notchF <=-400) {
          tft.drawFastVLine(pos_centre_f +1 - 320/spectrum_span * notchF / -1000, notchpos, notchL, notchColour);
          tft.drawFastVLine(pos_centre_f    - 320/spectrum_span * notchF / -1000, notchpos, notchL, notchColour);
          tft.drawFastVLine(pos_centre_f -1 - 320/spectrum_span * notchF / -1000, notchpos, notchL, notchColour);
          tft.drawFastHLine(pos_centre_f -4 - 320/spectrum_span * notchF / -1000, notchpos+notchL, 9, notchColour);
          tft.drawFastHLine(pos_centre_f -3 - 320/spectrum_span * notchF / -1000, notchpos+notchL + 1, 7, notchColour);
          tft.drawFastHLine(pos_centre_f -2 - 320/spectrum_span * notchF / -1000, notchpos+notchL + 2, 5, notchColour);
          tft.drawFastHLine(pos_centre_f -1 - 320/spectrum_span * notchF / -1000, notchpos+notchL + 3, 3, notchColour);
          tft.drawFastVLine(pos_centre_f    - 320/spectrum_span * notchF / -1000, notchpos+notchL + 4, 2, notchColour);
        }
        if (notchF >=400) {
          tft.drawFastVLine(pos_centre_f +1 + 320/spectrum_span * notchF / 1000, notchpos, notchL, notchColour);
          tft.drawFastVLine(pos_centre_f    + 320/spectrum_span * notchF / 1000, notchpos, notchL, notchColour);
          tft.drawFastVLine(pos_centre_f -1 + 320/spectrum_span * notchF / 1000, notchpos, notchL, notchColour);
          tft.drawFastHLine(pos_centre_f -4 + 320/spectrum_span * notchF / 1000, notchpos+notchL, 9, notchColour);
          tft.drawFastHLine(pos_centre_f -3 + 320/spectrum_span * notchF / 1000, notchpos+notchL + 1, 7, notchColour);
          tft.drawFastHLine(pos_centre_f -2 + 320/spectrum_span * notchF / 1000, notchpos+notchL + 2, 5, notchColour);
          tft.drawFastHLine(pos_centre_f -1 + 320/spectrum_span * notchF / 1000, notchpos+notchL + 3, 3, notchColour);
          tft.drawFastVLine(pos_centre_f    + 320/spectrum_span * notchF / 1000, notchpos+notchL + 4, 2, notchColour);
        }
      break;
    }
  }
  oldnotchF = notchF;
  pos_centre_f -= 1;
} // end void show_notch

/****************************************************************************** 
 *      E X T R A C T   D I G I T   ( F R E Q )                               *
 ******************************************************************************/
int ExtractDigit(long int n, int k) {
  switch (k) {
    case 0: return n%10;
    case 1: return n/10%10;
    case 2: return n/100%10;
    case 3: return n/1000%10;
    case 4: return n/10000%10;
    case 5: return n/100000%10;
    case 6: return n/1000000%10;
    case 7: return n/10000000%10;
    case 8: return n/100000000%10;
  }
}

/****************************************************************************** 
 *      S H O W    F R E Q U E N C Y                                          *
 ******************************************************************************/
// show frequency
void show_frequency(long int freq) { 

  tft.setTextSize(3);
  tft.setTextColor(WHITE);
  uint8_t zaehler;
  uint8_t digits[10];
  zaehler = 8;

  while (zaehler--) {
    digits[zaehler] = ExtractDigit (freq, zaehler);
//    Serial.print(digits[zaehler]);
//    Serial.print(".");
//    7: 10Mhz, 6: 1Mhz, 5: 100khz, 4: 10khz, 3: 1khz, 2: 100Hz, 1: 10Hz, 0: 1Hz
  }
//    Serial.print("xxxxxxxxxxxxx");

  zaehler = 8;
  while (zaehler--) {                     // counts from 7 to 0
    if (zaehler < 6) sch = 7;             // (khz)
    if (zaehler < 3) sch = 14;            // (Hz)
    if (digits[zaehler] != digits_old[zaehler] || !freq_flag) { // digit has changed (or frequency is displayed for the first time after power on)
      if (zaehler == 7) {
        sch = 0;
        tft.setCursor(pos_x_frequency + font_width * (8-zaehler) + sch, pos_y_frequency);        // set print position
        tft.fillRect(pos_x_frequency + font_width * (8-zaehler) + sch, pos_y_frequency, font_width, 24, BLACK); // delete old digit
        if (digits[7] != 0) tft.print(digits[zaehler]);                                         // write new digit in white
      }
      if (zaehler == 6) {
        sch = 0;
        tft.setCursor(pos_x_frequency + font_width * (8-zaehler) + sch, pos_y_frequency);        // set print position
        tft.fillRect(pos_x_frequency + font_width * (8-zaehler) + sch, pos_y_frequency, font_width, 24, BLACK); // delete old digit
        if (digits[6]!=0 || digits[7] != 0) tft.print(digits[zaehler]);                         // write new digit in white
      }
      if (zaehler == 5) {
        sch = 7;
        tft.setCursor(pos_x_frequency + font_width * (8-zaehler) + sch, pos_y_frequency);        // set print position
        tft.fillRect(pos_x_frequency + font_width * (8-zaehler) + sch, pos_y_frequency, font_width, 24, BLACK); // delete old digit
        if (digits[5] != 0 || digits[6]!=0 || digits[7] != 0) tft.print(digits[zaehler]);       // write new digit in white
      }
      if (zaehler < 5) {            // print the digit
        tft.setCursor(pos_x_frequency + font_width * (8-zaehler) + sch, pos_y_frequency); // set print position
        tft.fillRect(pos_x_frequency + font_width * (8-zaehler) + sch, pos_y_frequency, font_width, 24, BLACK); // delete old digit
        tft.print(digits[zaehler]); // write new digit in white
      }
      digits_old[zaehler] = digits[zaehler]; 
    }
  }

  // Display period
  tft.setTextSize(2);
  if (digits[7] == 0 && digits[6] == 0)
    tft.fillRect(pos_x_frequency + font_width * 3 -0,  pos_y_frequency + 18, 3, 3, BLACK);    // no period
  else
    tft.fillRect(pos_x_frequency + font_width * 3 -0,  pos_y_frequency + 18, 3, 3, YELLOW);   // MHz
    
  tft.fillRect(pos_x_frequency + font_width * 7 -10, pos_y_frequency + 18, 3, 3, YELLOW);     // KHz
  
  if (!freq_flag) {
    tft.setCursor(pos_x_frequency + font_width * 9 + 18, pos_y_frequency + 7); // set print position
    tft.setTextColor(GREEN);
    tft.print("Hz");
  }
  
  freq_flag = 1;
  tft.setTextSize(1);
  tft.setTextColor(WHITE);  
}    
