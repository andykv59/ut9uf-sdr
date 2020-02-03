// TFT stuff - change these defs for your specific LCD
// 

/*
#define BLACK   ST7735_BLACK
#define WHITE   ST7735_WHITE
#define RED     ST7735_RED
#define GREEN   ST7735_GREEN
#define YELLOW  ST7735_YELLOW
#define BLUE    ST7735_BLUE
*/

#define BLACK   ILI9341_BLACK
#define WHITE   ILI9341_WHITE
#define RED     ILI9341_RED
#define GREEN   ILI9341_GREEN
#define YELLOW  ILI9341_YELLOW
#define BLUE    ILI9341_BLUE

#define  USB_WIDE    0  // filter modes
#define  USB_NARROW  1
#define  LSB_WIDE    2
#define  LSB_NARROW  3

// Screen position defenitions for 320x240 TFT display
//
#define pos_spectrum    100   // y position of spectrum display
#define pos_version     230   // position of version number printing
#define pos_x_frequency 60    // Frequency display 
#define pos_y_frequency 140   // 
#define pos_x_mode      10    // Rx mode display
#define pos_y_mode      120   //  
#define pos_x_smeter    80   // S-meter display  80
#define pos_y_smeter    125   // 

#define pos_x_date      150   // Date display
#define pos_y_date      230   // 
#define pos_x_time      10    // Clock display
#define pos_y_time      230   // 
#define pos_x_menu      250   // Menu item display
#define pos_y_menu      230   //
#define pos_x_menuset   139   // Menu selection display
#define pos_y_menuset   230   // 
#define pos_x_message   10    // Message display
#define pos_y_message   195
#define pos_x_battery   10    // Battery display
#define pos_y_battery   210

#define pos_x_but_ant    177    // Antenna switch button in swith state
#define pos_y_but_ant    3
#define pos_x_but_ab     227    // A/B switch button in swith state
#define pos_y_but_ab     3
#define pos_x_but_sat    277    // SAT switch button in swith state
#define pos_y_but_sat    3
#define pos_x_but_pamp   177    // P.AMP/ATT switch button in swith state
#define pos_y_but_pamp   33
#define pos_x_but_split  227    // SPLIT switch button in swith state
#define pos_y_but_split  33
#define pos_x_but_vm     277    // V/M switch button in swith state
#define pos_y_but_vm     33
#define pos_x_but_filter 177    // Filter switch button in swith state
#define pos_y_but_filter 63
#define pos_x_but_fast   227    // FAST/Lock switch button in swith state
#define pos_y_but_fast   63
#define pos_x_but_rit    277    // RIT switch button in swith state
#define pos_y_but_rit    63
#define pos_x_but_txrx   297    // TX/RX swith state circle
#define pos_y_but_txrx   108


#define pos_x_menu_m1    10    // touch Menu 1 button
#define pos_y_menu_m1    200
#define pos_x_menu_m2    90    // touch Menu 2 button
#define pos_y_menu_m2    200
#define pos_x_menu_m3    167    // touch Menu 3 button
#define pos_y_menu_m3    200
#define pos_x_menu_m4    247    // touch Menu 4 button
#define pos_y_menu_m4    200

#define notchpos        2     // Notch filter
#define notchL          15
#define notchColour     YELLOW

#define font_width 18         // new freq display

#define s_w             10
#define spectrum_span   44.117
//#define posbin          159   // Bin for Rx above IF
#define posbin          80   // Bin for Rx above IF
#define binBW           172   // Hz / bin  (44117 / 256)
