/*
   my esp8266 wifi function
*/
#ifndef all_header
  #define all_header

  /* define what you need */
  #define USE_ETHERNET
  //# define USE_WIFI
  //#define USE_SSL
  #define V1
  //#define V2
  #define DF_LIST {"ESP12F_IDF", "ESP12F_ODF","ESP12F_testlatency"}
  #define DF_NUM 3   
  #define DM_NAME  "ESP12F" // Device Module name

  //#define MAX_HTTP_PACKAGE_SIZE 1024
  #define HTTP_RESPONSE_PAYLOAD_SIZE 512

  #define debug_mode
  #ifdef debug_mode
//    #define debug_prepare_http_package
//    #define debug_SEND
//    #define debug_GET
//    #define debug_mode_PUT
//    #define debug_POST
//    #define debug_getprofile
//    #define debug_register
//    #define debug_checknetstatus
//    #define debug_getProfile
//    #define debug_pull
//    #define debug_push
//    #define debug_ETH_TCP
  #endif


// Error code
#define TCP_CONNECT_ERROR -800
#define TCP_RECV_FBACK_BUT_NOT_HTTP 801
#define GetHTTPPayload_ERROR -802
#define GetHTTPCodeERROR -803
#define NO_NEW_DATA -804
#define HTTP_ACK_TIMEOUT -805





  /* include general/common library */
  #include <ArduinoJson.h>        // Json library

  /* choose the physical layer */
  #ifdef USE_ETHERNET
    #include <UIPEthernet.h> //ENC28J60 module driver
  #elif defined USE_WIFI
    #include <EEPROM.h>
    #include <ESP8266WiFi.h>
    #include <ESP8266WebServer.h>
    #include <ESP8266WiFiMulti.h>
    #include <ESP8266HTTPClient.h>
    #include <ESP8266TrueRandom.h> // uuid library

    #ifdef USE_SSL
      #include <WiFiClientSecure.h>
    #endif
    //#define FORCE_CONNECT  //if you don't want into ap_setting(),enable it
  #endif

  /* set server ip */
  #ifdef V1
    #ifdef USE_SSL
      #define DEFAULT_SERVER_IP "test.iottalk.tw"
    #else
      #define DEFAULT_SERVER_IP "140.113.199.181"
    #endif
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
  #define CLEAREEPROM  4 //hold for 5 second , it will erease the contain in eeprom
  #define TX           "TXD"
  #define RX           "RXD"

  #ifdef USE_ETHERNET
    #define ETHERNET_SO  12
    #define ETHERNET_SI  13
    #define ETHERNET_SCK 14
    #define ETHERNET_CS  15
  #endif
#endif


typedef struct httpresp{
  int HTTPStatusCode;
  char* payload;
}httpresp;

void SetDeviceID(void);
void CheckNetworkStatus(void);
void Init(void);

int get_DF_index(String target);

//#ifdef USE_ETHERNET
  void connect_to_ethernet(void);
  String prepare_http_package(const char* HTTP_Type, const char* feature, const char* payload);
  int Eth_TCP_Connect(void);
  void Send_HTTPS(httpresp *result, const char* HTTP_Type, const char* feature, const char* payload);
  void GET(httpresp *result, const char* feature ,bool close_TCP);
  void PUT(httpresp *result, const char* value, const char* feature );
  void POST(httpresp *result, const char* payload);
//#endif

#ifdef USE_WIFI
    //connect to wifi
    int WIFI_init(void);
    int connect_to_wifi(void);

    //EEPROM
    void clr_eeprom(int force); //clear eeprom (and wifi disconnect?)
    void save_WiFi_AP_Info(char *wifiSSID, char *wifiPASS, char *ServerIP);
    uint8_t  read_WiFi_AP_Info(void);

    //switch to sta  mode
    String scan_network(void);
    void handleRoot(void);
    void handleNotFound(void);
    void start_web_server(void);
    void AP_mode(void);
    void saveInfoAndConnectToWiFi(void);
#endif



