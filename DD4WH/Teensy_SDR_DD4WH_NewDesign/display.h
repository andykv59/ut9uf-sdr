// TFT stuff - change these defs for your specific LCD
#define BLACK   ILI9341_BLACK
#define WHITE   ILI9341_WHITE
#define RED     ILI9341_RED
#define GREEN   ILI9341_GREEN
#define YELLOW  ILI9341_YELLOW
#define BLUE    ILI9341_BLUE


// Screen position defenitions for 320x240 TFT display
//
#define pos_spectrum    100   // y position of spectrum display
#define pos_version     230   // position of version number printing
#define pos_x_frequency 80    // Frequency display  60
#define pos_y_frequency 150   // 
#define pos_x_mode      10    // Rx mode display
#define pos_y_mode      120   //  
#define pos_x_smeter    100   // S-meter display  80
#define pos_y_smeter    130   // 

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

#define notchpos        2     // Notch filter
#define notchL          15
#define notchColour     YELLOW

#define font_width 18         // new freq display

#define s_w             10
#define spectrum_span   44.117
#define posbin          159   // Bin for Rx above IF
#define binBW           172   // Hz / bin  (44117 / 256)


