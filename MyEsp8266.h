/*
   my esp8266 wifi function
*/


#ifndef all_header
  #define all_header
  #include "config.h"

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

    #ifdef USE_SSL
      #include <WiFiClientSecure.h>
    #endif
    
  #endif

  /* set server ip */
  #ifdef  V2
    #include <PubSubClient.h> // MQTT library
  #endif
#endif

// Error code
#define TCP_CONNECT_ERROR -800
#define TCP_RECV_FBACK_BUT_NOT_HTTP 801
#define GetHTTPPayload_ERROR -802
#define GetHTTPCodeERROR -803
#define NO_NEW_DATA -804
#define HTTP_ACK_TIMEOUT -805

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
  IPAddress c_toIPAddr (char *ip);
  String prepare_http_package(const char* HTTP_Type, const char* feature, const char* payload);
  int Eth_TCP_Connect(void);
  void Send_HTTPS(httpresp *result, const char* HTTP_Type, const char* feature, const char* payload, bool close_TCP);
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
int check_idf(String df_name);
void store(String df_name, String topic, String command);
void MQTTcallback(char* topic, byte* payload, int length);
void get_ctrl_chan(String http_PL);
String state_rev(String state, String rev);
void MQTT_Conn(void);
void CtrlHandle(void);
void V2_PUT(httpresp *result, String ip, String port, String uuid, String payload);

