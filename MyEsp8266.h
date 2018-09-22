/*
   my esp8266 wifi function
*/
#ifndef all_header
  #define all_header
  
  /* define what you need */
  //#define USE_ETHERNET
  #define USE_WIFI
  #define V1  // Select iottalk version
  //define V2
  #define DF_LIST {"ESP12F_IDF", "ESP12F_ODF"}
  #define DM_NAME  "ESP12F" // Device Module name  
  #define debug_mode

  //#define USE_SSL

  /* include general/common library */
  #include <ArduinoJson.h>        // Json library

  /* choose the physical layer */
  #ifdef USE_ETHERNET
    #include <UIPEthernet.h> //ENC28J60 module driver
    #define MAX_HTTP_PACKAGE_SIZE 512
  #elif defined USE_WIFI
    #include <EEPROM.h>
    #include <ESP8266WiFi.h>
    #include <ESP8266WebServer.h>
    #include <ESP8266WiFiMulti.h>
    #include <ESP8266HTTPClient.h>
    #include <ESP8266TrueRandom.h> // uuid library
    //#define FORCE_CONNECT  //if you don't want into ap_setting(),enable it
  #endif

  /* set server ip */
  #ifdef V1
    #define DEFAULT_SERVER_IP "140.113.215.7"
  #elif defined V2
    #define DEFAULT_SERVER_IP "140.113.199.198"
    #include <PubSubClient.h> // MQTT library
  #endif

  /* set server port*/
  #ifdef USE_SSL
    #define ServerPort 443
  #else
    #define ServerPort 9999
  #endif

  /* pin out */
  #define UPLOAD       0  // when you want to upload code to esp8266, this pin must be LOW, will not be used on Nodemcu
  #define LEDPIN       2  // on board led
  #define CLEAREEPROM  13 //hold for 5 second , it will erease the contain in eeprom

  #ifdef USE_ETHERNET
    #define ETHERNET_SO  12
    #define ETHERNET_SI  13
    #define ETHERNET_SCK 14
    #define ETHERNET_CS  15
  #endif
#endif


#ifdef debug_mode
#endif

typedef struct httpresp{
  int HTTPStatusCode;
  char* payload;
}httpresp;

#ifdef USE_ETHERNET
  void connect_to_ethernet(void);
  String prepare_http_package(const char* HTTP_Type, const char* feature, const char* payload);
  httpresp Send_HTTP(const char* HTTP_Type, const char* feature, const char* payload, bool WillResp);
  httpresp Eth_GET(const char* feature);
  httpresp Eth_PUT(const char* value, const char* feature );
  httpresp Eth_POST(const char* payload);
#endif

#ifdef USE_WIFI
    //connect to wifi
    int WIFI_init(void);
    int connect_to_wifi(char *wifiSSID, char *wifiPASS);

    //EEPROM
    void clr_eeprom(int force); //clear eeprom (and wifi disconnect?)
    void save_WiFi_AP_Info(char *wifiSSID, char *wifiPASS, char *ServerIP);
    //uint8_t  read_WiFi_AP_Info(char *wifiSSID, char *wifiPASS, char *ServerIP);
    uint8_t  read_WiFi_AP_Info(void);

    //switch to sta  mode
    String scan_network(void);
    void handleRoot(void);
    void handleNotFound(void);
    void start_web_server(void);
    void AP_mode(void);
    void saveInfoAndConnectToWiFi(void);
#endif



