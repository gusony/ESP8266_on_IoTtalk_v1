#include "MyEsp8266.h"



int tcp_connect_error_times = 5;
char ServerIP[50];
const char* fingerprint = "FE BA 2F E1 56 88 9D EC 0B 19 F8 41 BB 9D 6E 55 06 16 DF 8F";
char httpspw[36] ; // store https password
char deviceid[37]; // v1 use 12 char, v2 use 36 char
String httppw = "";


#ifdef USE_ETHERNET
  EthernetClient TCPclient;
  byte mac[6] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x07}; // you can set it as you want
#elif defined USE_WIFI
  byte mac[6];
  char wifissid[50] = "";
  char wifipass[50] = "";
  uint8_t wifimode = 1; //1:AP , 0: STA
  WiFiClient espClient;
  ESP8266WebServer server ( 80 );
  /////////////////////////////////////
  #ifdef USE_SSL
    WiFiClientSecure TCPclient;
  #else
    HTTPClient httpclient;
  #endif
#endif

#ifdef V2
  #ifdef USE_ETHERNET
    PubSubClient client(TCPclient);
  #elif defined USE_WIFI
    PubSubClient client(espClient);
  #endif
#endif

void SetDeviceID(void){
  String DID = ""; //Device ID
#ifdef USE_WIFI
  WiFi.macAddress(mac);
#endif
  
#ifdef V1
  for(int i=0; i<6; i++) DID += mac[i]<0x10 ? "0"+String(mac[i], HEX) : String(mac[i], HEX);
#elif defined V2
  DID = ESP8266TrueRandom.uuidToString(mac);
#endif

  DID.toCharArray(deviceid, DID.length());
}





#ifdef USE_ETHERNET
void connect_to_ethernet(void) {
  while (1) {
    Serial.print("[Ethernet]begin");
    if (Ethernet.begin(mac)) {
      Serial.println(" successful");
      break;
    }
    else
      Serial.println(" fail");
  }
  Serial.print("[Ethernet]localIP:");
  Serial.println(Ethernet.localIP());
}
#endif


#if defined(USE_ETHERNET) || defined(USE_SSL)
String prepare_http_package(const char* HTTP_Type, const char* feature, const char* payload){
  String package = String(HTTP_Type) + " /" + String(deviceid) ;  //sum of http string that will be send out
  if (feature != "")
    package += "/" + String(feature);
  package += " HTTP/1.1\n";
#ifdef USE_SSL
  package += "Host: " + String(ServerIP) + "\n" ; // should not use DEFAULT_SERVER_IP

  if(String(HTTP_Type) != "POST"){
    package += "password-key: "+httppw+"\n";
  }
#endif
  package += "Content-Type: application/json\n";
  if (payload != "") {
    package += "Content-Length: " + String(String(payload).length()) + "\n\n";
    package += String(payload) + "\n";
  }
  
#ifdef debug_mode
  Serial.println(package);
#endif
  return(package);
}
httpresp Send_HTTPS(const char* HTTP_Type, const char* feature, const char* payload, bool WillResp) {
  String temp = "";
  
  char * http_resp_package = (char*)malloc(sizeof(char) * MAX_HTTP_PACKAGE_SIZE);
  memset(http_resp_package, 0, MAX_HTTP_PACKAGE_SIZE);
  
  httpresp result;
  result.HTTPStatusCode = 0;
  result.payload = "";
  
  int resp_timeout = 1000;
  int package_size = 0;
  int string_indexof;

  long start_time = 0;
  
  if (TCPclient.connect(ServerIP, ServerPort)) {
    TCPclient.println(prepare_http_package(HTTP_Type, feature, payload));
    
    start_time = millis();
    while (millis() - start_time < resp_timeout) {
      package_size = TCPclient.available();
      if(package_size > 0){
        TCPclient.read((uint8_t*)http_resp_package, package_size);
        #ifdef debug_mode
          Serial.println(http_resp_package);
        #endif
        
        //get http state code
        string_indexof = String(http_resp_package).indexOf("HTTP/");
        if(string_indexof >= 0)
          result.HTTPStatusCode = (http_resp_package[9] - 48) * 100 + (http_resp_package[10] - 48) * 10 + http_resp_package[11] - 48;
        if(result.HTTPStatusCode != 200)
          Serial.println(http_resp_package);
        
        //get http response payload
        string_indexof = String(http_resp_package).indexOf("GMT");
        if(string_indexof >= 0){
          temp = String(http_resp_package).substring(string_indexof+7);
          temp.toCharArray(result.payload, temp.length());

          //get payload of response package ,so clear memory 
          free(http_resp_package);
          TCPclient.flush();
          TCPclient.stop();
          return (result);
        }
      }
    }
    free(http_resp_package);
    TCPclient.flush();
    TCPclient.stop();
    return (result);
  }

    //can not build tcp connection
    Serial.println("[Send_HTTP]Tcp Connect fail");
    tcp_connect_error_times--;
    if(tcp_connect_error_times<=0){
#if defined(USE_ETHERNET)
      connect_to_ethernet();
#elif defined(USE_SSL)
      connect_to_wifi(wifissid, wifipass);
#endif
      tcp_connect_error_times = 5 ;
    }
    result.HTTPStatusCode = -1;
    free(http_resp_package);
    TCPclient.flush();
    return (result);

    /*
     * TCP Connect Fail maybe:
     * 
     */
}
#endif
httpresp GET(const char* df_name ) {
#if defined(USE_ETHERNET) || defined(USE_SSL)
  return (Send_HTTPS("GET", feature, "", 1));
#else
  httpresp  result;
  httpclient.begin( "http://" + String(ServerIP) + ":"+String(ServerPort)+"/"+String(deviceid)+"/" + String(df_name) );
  httpclient.addHeader("Content-Type", "application/json");
  result.HTTPStatusCode  = httpclient.GET();
  String http_resp = httpclient.getString();
  result.payload = (char*)malloc(sizeof(char)*http_resp.length());
  http_resp.toCharArray(result.payload, http_resp.length());
  return(result);
#endif
}
httpresp PUT(const char* value, const char* df_name ) {
#if defined(USE_ETHERNET) || defined(USE_SSL)
  String S_payload = "{\"data\":[" + String(value) + "]}";
  return (Send_HTTPS("PUT", feature, S_payload.c_str(), 0));
#else
  httpresp result;
  httpclient.begin( "http://" + String(ServerIP) + ":"+String(ServerPort)+"/"+String(deviceid)+"/" + String(df_name));
  httpclient.addHeader("Content-Type", "application/json");
  String data = "{\"data\":[" + String(value) + "]}";
  result.HTTPStatusCode = httpclient.PUT(data);
  return result;
#endif
}
httpresp POST(const char* payload) {
#if defined(USE_ETHERNET) || defined(USE_SSL)
  return (Send_HTTPS("POST", "", payload, 1) );
#else 
  //non https
  httpresp  result;
  httpclient.begin("http://" + String(ServerIP) + ":"+String(ServerPort)+"/"+String(deviceid));
  httpclient.addHeader("Content-Type", "application/json");
  result.HTTPStatusCode = httpclient.POST(String(payload));
  httpclient.getString().toCharArray(result.payload, httpclient.getString().length());
  Serial.print("[POST]");Serial.println(result.payload);
  //result.payload = httpclient.getString().c_str();  // **would not return correctly**
  return result;
#endif
}
//#endif




#ifdef USE_WIFI
//connect to wifi
/*   WiFi init
 *               --------------------------
 *               | check if data in eeprom|
 *               --------------------------
 *                yes|              no|
 *                   |                |
 *      -----------------             V            
 *      |wifi connected?|      --------------
 *      -----------------      |   AP mode  |   <-------
 *      |yes       no |        --------------          |
 *      |             |               | user key in    |
 *      |             V               V                |
 *      |          ------------------------            |
 *      |          |  connect to wifi     |            |
 *      |          ------------------------            |
 *      |            |               |                 |
 *      |         OK |          fail |                 |
 *      |            V               -------------------
 *      --------->  exit
 *    
 *    1.  check if it haven stored WiFi ssid, password and iottalk server ip int eeprom
 *    2.1 YES, go to 3.
 *    2.2 NO, set ESP8266 into AP mode, and waitting user connecting to esp8266 and key in ssid, password and serverip
 *    3.  try to connect to wifi. IF fail, turn to ap mode.
 */
int WIFI_init(void){
  //uint8_t statesCode = read_WiFi_AP_Info(&wifissid[0], &wifipass[0], &ServerIP[0]);
  uint8_t statesCode = read_WiFi_AP_Info();
  
  if( !(statesCode & 1) ){
    String(DEFAULT_SERVER_IP).toCharArray(ServerIP, 50);
    Serial.println("[WiFi_init] ServerIP: "+String(ServerIP));
  }
  
  if (WiFi.status() == WL_CONNECTED) 
   return 1; 
  else if ( (statesCode&0x04)&& (statesCode&0x02) ) 
    if(connect_to_wifi(wifissid, wifipass))
      return 1;
    
  AP_mode();
  
}
int connect_to_wifi(char *wifiSSID, char *wifiPASS){
  Serial.print("[WiFi]Connecting");
  
#ifndef FORCE_CONNECT
  long connecttimeout = millis();
  WiFi.softAPdisconnect(true);
  WiFi.begin(wifiSSID, wifiPASS);
  while (WiFi.status() != WL_CONNECTED && (millis() - connecttimeout < 10000) ) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
#else
  long connecttimeout = 0;
  while (WiFi.status() != WL_CONNECTED  ) {
    if( millis() - connecttimeout > 10000){
      WiFi.begin(wifiSSID, wifiPASS);
      connecttimeout = millis();
    }
    
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
#endif
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFi]Connected");
    wifimode = 0 ;
    return 1;
  }
  else if (millis() - connecttimeout > 10000) {
    Serial.println("[WiFi]Fail");
    return 0;
  }
}


//EEPROM
void clr_eeprom(int force){ //clear eeprom (and wifi disconnect?)
  if (!force) {
    delay(3000);
  }
  if ( (digitalRead(CLEAREEPROM) == LOW) || (force == 1) ) {
    for (int addr = 0; addr < 512; addr++) EEPROM.write(addr, 0); // clear eeprom
    EEPROM.commit();
    Serial.println("Clear EEPROM.");
    delay(50);
  }
}
void save_WiFi_AP_Info(char *wifiSSID, char *wifiPASS, char *ServerIP){  //stoage format: [SSID,PASS,ServerIP]
  char *netInfo[3] = {wifiSSID, wifiPASS, ServerIP};
  int addr = 0, i = 0, j = 0;
#ifdef debug_mode
  Serial.println("[Save WiFi info] Start");
#endif

  EEPROM.write (addr++, '['); // the code is equal to (EEPROM.write (addr,'[');  addr=addr+1;)
  for (j = 0; j < 3; j++) {
    i = 0;
    while (netInfo[j][i] != '\0') {
      EEPROM.write(addr++, netInfo[j][i++]);
      delay(4); //a eeprom write command need 3.3ms
    }
    if (j < 2) {
      EEPROM.write(addr++, ',');
    }
  }
  EEPROM.write (addr++, ']');
  EEPROM.commit();
  delay(50);
  
#ifdef debug_mode
  Serial.println("[Save WiFi info] end");
#endif
}
//uint8_t  read_WiFi_AP_Info(char *wifiSSID, char *wifiPASS, char *ServerIP){ // storage format: [SSID,PASS,ServerIP]
uint8_t  read_WiFi_AP_Info(void){
  char *netInfo[3] = {wifissid, wifipass, ServerIP};
  String readdata = "";
  int addr = 0;
  char temp = EEPROM.read(addr++);
  uint8_t return_state = 0;

  if (temp != '[') {
    return return_state;
  }

  for (int i = 0; i < 3; i++, readdata = "") {
    while (1) {
      temp = EEPROM.read(addr++);
      if (temp == ',' || temp == ']'){
        readdata += '\0';
        break;
      }
      readdata += temp;
    }
    readdata.toCharArray(netInfo[i], 50);
  }


  if (String(wifissid).length () > 0)
    return_state |= 1<<2;

  if (String(wifipass).length () > 0)
    return_state |= 1<<1;

  if (String(ServerIP).length () > 7)
    return_state |= 1<<0;

  return return_state;
}

//switch to sta  mode
String scan_network(void){
  int AP_N, i; //AP_N: AP number
  String AP_List = "<select name=\"SSID\" style=\"width: 150px;\">" ; // make ap_name in a string
  AP_List += "<option value=\"\">請選擇</option>";

  //WiFi.disconnect();
  //delay(100);
  AP_N = WiFi.scanNetworks();

  if (AP_N > 0)
    for (i = 0; i < AP_N; i++)
      AP_List += "<option value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + "</option>";
  else
    AP_List = "<option value=\"\">NO AP</option>";

  AP_List += "</select><br><br>";
  return (AP_List);
}
void handleRoot(void){
  Serial.println("handleRoot");
  String temp = "<html><title>Wi-Fi Setting</title>";
  temp += "<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>";
  temp += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head>";
  temp += "<form action=\"setup\"><div>";
  temp += "SSID:<br>";
  temp += scan_network();
  temp += "Password:<br>";
  temp += "<input type=\"password\" name=\"Password\" vplaceholder=\"輸入AP密碼\" style=\"width: 150px;\">";
  temp += "<br><br>IoTtalk Server IP<br>";
  temp += "<input type=\"serverIP\" name=\"serverIP\" value=\"" + String(ServerIP) + "\" style=\"width: 150px;\">";
  temp += "<br><br><input type=\"submit\" value=\"Submit\" on_click=\"javascript:alert('TEST');\">";
  temp += "</div></form><br>";
  temp += "</html>";
  server.send ( 200, "text/html", temp );
}
void handleNotFound(void){
  Serial.println("Page Not Found ");
  server.send( 404, "text/html", "Page not found.");  
}
void saveInfoAndConnectToWiFi(void){
  Serial.println("Get network information.");

  String temp = "Receive SSID & PASSWORD";
  server.send ( 200, "text/html", temp );

  if (server.arg(0) != "" && server.arg(1) != "" && server.arg(2) != "" ) { //arg[0]-> SSID, arg[1]-> password (both string)
    server.arg(0).toCharArray(wifissid, 50);
    server.arg(1).toCharArray(wifipass, 50);
    server.arg(2).toCharArray(ServerIP, 50);
    server.stop();
    Serial.print("[");
    Serial.print(wifissid);
    Serial.print("][");
    Serial.print(wifipass);
    Serial.print("][");
    Serial.print(ServerIP);
    Serial.println("]");
    
    if(connect_to_wifi(wifissid, wifipass) == 1){
      save_WiFi_AP_Info(wifissid, wifipass, ServerIP);
    }
  }
}
void start_web_server(void){
  server.on ( "/", handleRoot );
  server.on ( "/setup", saveInfoAndConnectToWiFi);
  server.onNotFound ( handleNotFound );
  server.begin();
  while (wifimode) 
    server.handleClient();
}
void AP_mode(void){
  String softapname = "ESP12F-"+String(deviceid);
  Serial.println("[AP_SET]:"+softapname);

  IPAddress ip(192, 168, 0, 1);
  IPAddress gateway(192, 168, 0, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();
  WiFi.softAPConfig(ip, gateway, subnet);
  WiFi.softAP(&softapname[0]);
  
  start_web_server();
}

#ifdef USE_SSL

#endif

#endif


