//#include <SoftwareSerial.h>   // Software simulate Serial library


#ifdef USE_PM25
  #define PMS_TX  NULL        //GPIO14 on nodemcu <---> PMS_RXpin , no bird use
  #define PMS_RX  12        //GPIO12 on nodemcu <---> PMS_TXpin
  #define pms_baudrate  9600
  String read_pm25(void);
#endif

#ifdef USE_LCM1602
  void lcd_print(String Str,int column,int row);
#endif

#ifdef USE_SSD1306
    #ifdef USE_SSD1306_IIC
        #define OLED_RESET 3 //reset pin will not be used, just for declare
        #define OLED_SDA   4 //SDA:04, SCL:05 :those two pin are on board
        #define OLED_CLK   5 //but those two define will not be used
    #elif defined  USE_SSD1306_SPI  //SSD1306 with SPI
        #define OLED_MOSI  13   //D1 on SSD1306
        #define OLED_CLK   14   //D0 on SSD1306
        #define OLED_DC    2
        #define OLED_CS    15
        #define OLED_RESET 16
    #endif
    #include <Adafruit_GFX.h>     // Font on monitor
    #include <Adafruit_SSD1306.h> // OLED (SSD1306) library
    void init_ssd1306(void);
    void OLED_print(String mes);
#endif

#ifdef USE_GPS
    #define GPS_TX  2         // GPIO0 on nodemcu <---> GPS_RXpin , no bird use
    #define GPS_RX  0         // GPIO2 on nodemcu <---> GPS_TXpin
    #define GPS_baudrate  9600
    String get_GPS( String value);
#endif

#ifdef USE_DHT
  #include "DHT.h"              // DHT library
  #define DHTPIN 10         // GPIO10
  #define DHTTYPE DHT11     // DHT 11
  //#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
  //#define DHTTYPE DHT21   // DHT 21 (AM2301)
#endif





/* ****************************************
 * String to Char*
 * String S = "Hello";
 * char *c = const_cast<char*>(S.c_str());
 * ****************************************
 * 
 * /
 */
