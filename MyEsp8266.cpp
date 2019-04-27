#include "MyEsp8266.h"

int tcp_connect_error_times = 10;
char ServerIP[50];
char deviceid[37]; // v1 use 12 char, v2 use 36 char
String idf_list[10] = IDF_LIST;

//according to 'MyEsp8266.h' , choose the way you want to use to connect Internet
#ifdef USE_ETHERNET
  EthernetClient TCPclient;
  uint8_t mac[6]    = {MACADDRESS};
  uint8_t myIP[4]   = {MYIPADDR};
  uint8_t myMASK[4] = {MYIPMASK};
  uint8_t myDNS[4]  = {MYDNS};
  uint8_t myGW[4]   = {MYGW};
#elif defined USE_WIFI
  byte mac[6];            // use esp8266 itself mac address
  char wifissid[50] = ""; //store Wi-Fi SSID , it can come from user keyying in or read from EEPROM
  char wifipass[50] = ""; //store Wi-Fi password
  uint8_t wifimode = 1;   //1:AP , 0: STA,
  WiFiClient espClient;
  ESP8266WebServer server ( 80 );

  #ifdef USE_SSL
    WiFiClientSecure TCPclient;
    const char* fingerprint = "BF D3 C5 AE D2 75 9F 12 C4 2A 7A 1B 18 F4 9F F5 70 24 22 32";
    String httppw = "";
  #else
    HTTPClient httpclient;
  #endif
#endif


#ifdef V2
  String mqtt_mes= "", ctrl_i, ctrl_o, d_name, rev, IDF_topic;
  bool new_message = false;
  StaticJsonBuffer<512> JB_CD; // Dynamic buffer size is easy to make the esp8266 crash!!!!
                               // CD:ctrl data, i need a better name
  JsonArray& JA_CD = JB_CD.createArray(); // store topic and command of idf/odf, format:[["ESP12F_IDF","topic","command"],["ESP12F_ODF","topic","command"]]

  #ifdef USE_ETHERNET
    PubSubClient MQTTclient(TCPclient);
  #elif defined USE_WIFI
    PubSubClient MQTTclient(espClient);
  #endif
#endif


//general function (通用涵式),  不管Wi-Fi/Ethernet 或 V1/V2 都會用到的
void SetDeviceID(void){
  /*
   * If use wifi , the mac address is in the esp8266 chip
   * If use ethernet, the mac address can be assigned by user
   *
   * With V1, use the mac-address string as deviceID
   * With V2, I use mac-address to generate deviceID(DID)
   */

  String S_deviceid = ""; //String type DeviceID

#ifdef USE_WIFI
  WiFi.macAddress(mac); // get esp8266 wifi chip mac address , and store into 'mac' (byte array)
#endif

#ifdef V1
  for(int i=0; i<6; i++) DID += mac[i]<0x10 ? "0"+String(mac[i], HEX) : String(mac[i], HEX);
#elif defined V2
  S_deviceid = "00010203-0405-0000-0a00-0000ff0f3418";//ESP8266TrueRandom.uuidToString(mac);
#endif

  S_deviceid.toCharArray(deviceid, S_deviceid.length()+1);
  Serial.println("[SerDeviceID]"+String(deviceid));
}
void CheckNetworkStatus(void){ // inclued Wi-Fi and Ethernet
#ifdef USE_WIFI
  if( WiFi.status() != WL_CONNECTED )
    connect_to_wifi();
#elif defined USE_ETHERNET
  #ifdef debug_checknetstatus
  Serial.print("[Ethernet]localIP:");
  Serial.println(Ethernet.localIP());
  #endif
  if(String(Ethernet.localIP()).indexOf("0.0.0.0") >= 0)
    connect_to_ethernet();
#endif
}
void Init(void){
/*
 * Init steps that are no directly related to iottalk are writing in this func.
 * 1. init Serial input/ourput
 * 2. init Device id(include set mac address)
 * 3. init network(ssid&pawd of Wi-Fi are stored in EEPROM -> so init EEPROM)
 */

  // 1. initial serial port
  delay(10);
  Serial.begin(115200);
  Serial.println();
  Serial.println("[Init] Serial OK.");

  // 2.
  SetDeviceID();
  Serial.println("[Init] SetDeviceID OK.");

  // 3. init Network
  // wifi 從EEPROM 裡面讀 IP
  // ethernet 用default IP
#ifdef USE_WIFI
  EEPROM.begin(512);
  pinMode(CLEAREEPROM, INPUT_PULLUP);
  //clr_eeprom(1); // for debug
  WIFI_init();
#elif defined USE_ETHERNET
  connect_to_ethernet(); // get IP by Router DHCP
  String(DEFAULT_SERVER_IP).toCharArray(ServerIP, 50);
#endif
  Serial.println("[Init] ServerIP:"+String(ServerIP));
  Serial.println("[Init] Connect to internet OK");

  //4. if use V2
#ifdef V2
  Serial.println("[Init] Set serverip "+String(ServerIP));
  MQTTclient.setServer(ServerIP, 1883);
  MQTTclient.setCallback(MQTTcallback);
  Serial.println("[Init] MQTT setServer OK");
#endif

  Serial.println("[Init] OK");
}
void GET(httpresp *result, const char* df_name ,bool close_TCP) {
#ifdef debug_GET
  Serial.println("[GET]Start");
#endif
#if defined(USE_ETHERNET) || defined(USE_SSL)
  Send_HTTPS(result, "GET", df_name, "",close_TCP);
#else
  httpclient.begin( "http://" + String(ServerIP) + ":"+String(ServerPort)+"/"+String(deviceid)+"/" + String(df_name) );
  httpclient.addHeader("Content-Type", "application/json");
  result->HTTPStatusCode  = httpclient.GET();
  String http_resp = httpclient.getString();
  http_resp.toCharArray(result->payload, http_resp.length());
  httpclient.end();
#endif
}
void PUT(httpresp *result, const char* value, const char* df_name ) {
#ifdef debug_mode_PUT
  Serial.println("[PUT]Start");
  Serial.println("[PUT]"+(String)df_name+","+(String)value);
#endif
String data = "{\"data\":[" + String(value) + "]}";

#if defined(USE_ETHERNET) || defined(USE_SSL)
  Send_HTTPS(result, "PUT", df_name, data.c_str(), 0);
#else
  httpclient.begin( "http://" + String(ServerIP) + ":"+String(ServerPort)+"/"+String(deviceid)+"/" + String(df_name));
  httpclient.addHeader("Content-Type", "application/json");
  result->HTTPStatusCode = httpclient.PUT(data);
  httpclient.end();
#endif
}
void POST(httpresp *result, const char* payload) {
#ifdef debug_POST
  Serial.println("[POST]Start");
#endif
#if defined(USE_ETHERNET) || defined(USE_SSL)
  Send_HTTPS(result, "POST", "", payload, 0);
#else
  Serial.println("[POST] http://" + String(ServerIP) + ":"+String(ServerPort)+"/"+String(deviceid));
  httpclient.begin("http://" + String(ServerIP) + ":"+String(ServerPort)+"/"+String(deviceid));
  httpclient.addHeader("Content-Type", "application/json");
  result->HTTPStatusCode = httpclient.POST(String(payload));
  String http_resp = httpclient.getString();
  http_resp.toCharArray(result->payload, http_resp.length());
  httpclient.end();
#endif
}


#ifdef V1 // only for V1
int get_DF_index(String target){ // find the index of feature in DF_list
  String df_list[DF_NUM] = DF_LIST;
  for(int i = 0; i <= DF_NUM; i++)
    if(df_list[i] == target)
      return i;
  return -1;
}
#endif

#ifdef V2 // only for V2
int check_idf(String df_name){
  for(int i =0 ; i<10; i++) // should not use const number
    if(df_name == idf_list[i])
      return i;

  return -1;
}
void store(String df_name, String topic, String command){
  DynamicJsonBuffer JB_temp;
  JsonArray& JA_temp = JA_CD.createNestedArray();
  JA_temp.add(df_name);
  JA_temp.add(topic);
  JA_temp.add(command);
  JB_temp.clear();
}
void MQTTcallback(char* topic, byte* payload, int length) {
  mqtt_mes = "";
  
  for (int i = 0; i < length; i++) {
    mqtt_mes += (char)payload[i];
  }
  
  Serial.print("[");
  Serial.print(topic);
  Serial.print("]->");
  Serial.println(mqtt_mes);
  
  if(mqtt_mes.indexOf("command"))
    new_message = true;

}
void V2_PUT(httpresp *result, String ip, String port, String uuid, String payload){
#if defined(USE_ETHERNET) || defined(USE_SSL)
  Send_HTTPS(result, "PUT", "", payload.c_str(), 0);
#else
  HTTPClient http;
  String http_resp = "";
  http.begin("http://"+ip+":"+port+"/"+String(deviceid));
  http.addHeader("Content-Type","application/json");
  result->HTTPStatusCode = http.PUT(payload);
  http_resp = http.getString();
  http_resp.toCharArray(result->payload, http_resp.length()+1);
  httpclient.end();
#endif
}
void get_ctrl_chan(String http_PL){
  Serial.println("[get_cha]"+http_PL);
  DynamicJsonBuffer JB_PUT_resp; //maybe crash , but didn't happend not yet
  JsonObject& JO_PUT_resp = JB_PUT_resp.parseObject(http_PL);
  ctrl_i = JO_PUT_resp["ctrl_chans"][0].as<String>(); Serial.println("ctrl_i:"+ctrl_i);
  //ctrl_i = "00010203-0405-0000-0a00-0000ff0f3418\\ctrl\\i";
  ctrl_o = JO_PUT_resp["ctrl_chans"][1].as<String>(); Serial.println("ctrl_o:"+ctrl_o);
  //ctrl_o = "00010203-0405-0000-0a00-0000ff0f3418\\ctrl\\o";
  d_name = JO_PUT_resp["name"].as<String>();          Serial.println("d_name:"+d_name);
  //d_name ="012.ESP12F";
  rev    = JO_PUT_resp["rev"].as<String>();
  //rev ="660d6056-e940-4d93-87be-d84f119a37ef";
  JB_PUT_resp.clear();
}
String state_rev(String state, String rev){
  String mes;
  DynamicJsonBuffer JB_temp;
  JsonObject& JO_temp = JB_temp.createObject();
  JO_temp["state"] = state;
  JO_temp["rev"] = rev;
  JO_temp.printTo(mes);
  JB_temp.clear();
  return(mes);
}
void MQTT_Conn(void){
  while (!MQTTclient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (MQTTclient.connect(String(deviceid).c_str(), ctrl_i.c_str(), 0, true, state_rev("broken",rev).c_str() )){// connect to mqtt server
      Serial.println("connected state : "+(String)MQTTclient.state());

      if( MQTTclient.subscribe(ctrl_i.c_str()) ) //not necessary
        Serial.println("ctrl_i subscribe successful!, " + ctrl_i);

      if( MQTTclient.subscribe(ctrl_o.c_str()))
        Serial.println("ctil_o subscribe successful!, " + ctrl_o);

      if( MQTTclient.publish(ctrl_i.c_str(), state_rev("online",rev).c_str()) )
        Serial.println("[" +ctrl_i+"]<-"+state_rev("online",rev));

      //Serial.println("Register finish.");
      break;
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(MQTTclient.state());
      Serial.println(" try again in 5 seconds");

      delay(5000);
    }
  }
}
void CtrlHandle(void){
  new_message = false;
  String ok_mes = "{\"state\":\"ok\",\"msg_id\":\"";

  DynamicJsonBuffer JB_temp;  // CD:ctrl data, i need a better name
  JsonObject& JO_temp = JB_temp.parseObject(mqtt_mes);

  DynamicJsonBuffer JB_ok_mes;  // CD:ctrl data, i need a better name
  JsonObject& JO_ok_mes = JB_ok_mes.createObject();

  if(JO_temp.containsKey("command")){ // this CtrlHandle function only care about command, i don't care data
    if( JO_temp["command"].as<String>() == "CONNECT"){
      if(JO_temp.containsKey("odf")){
        store(JO_temp["odf"].as<String>(), JO_temp["topic"].as<String>(),JO_temp["command"].as<String>());
        MQTTclient.subscribe(JO_temp["topic"].as<String>().c_str());
      }
      else if(JO_temp.containsKey("idf"))
        store(JO_temp["idf"].as<String>(), JO_temp["topic"].as<String>(),JO_temp["command"].as<String>());

      ok_mes += JO_temp["msg_id"].as<String>() + "\"}";   //Serial.println("ok_mes = "+ ok_mes);
      MQTTclient.publish(ctrl_i.c_str(), ok_mes.c_str());

      if(check_idf("ESP12F_IDF") != -1)
        IDF_topic = JA_CD[check_idf("ESP12F_IDF")][1].as<String>();
    }
    else if(JO_temp["command"].as<String>() == "DISCONNECT"){
      /* need to unscribe */
      for(int i=0; i< JA_CD.size(); i++)
        if(JA_CD[i][0].as<String>() == JO_temp["idf"].as<String>()  || JA_CD[i][0].as<String>() == JO_temp["odf"].as<String>() ){
          MQTTclient.unsubscribe(JA_CD[i][1].as<String>().c_str());
          Serial.println("[Unsubscribe]"+JA_CD[i][0].as<String>()+", ["+JA_CD[i][1].as<String>())+"]";
          break;
        }

      String remove_df_name = JO_temp["idf"].as<String>().length() > 0 ? JO_temp["idf"].as<String>() : JO_temp["odf"].as<String>();
      for(int i =0; i<JA_CD.size(); i++)
        if( remove_df_name == JA_CD[i][0].as<String>())
          JA_CD.remove(i);
    }
  }
}
#endif


//使用Ethernet 需要自己包HTTP封包 跟 收封包的Func
#if defined(USE_ETHERNET) || defined(USE_SSL)
String prepare_http_package(const char* HTTP_Type, const char* feature, const char* payload){
  // don't use \r
  String package = String(HTTP_Type) + " /" + String(deviceid) ;  //sum of http string that will be send out
  if (feature != "")
    package += "/" + String(feature);
  package += " HTTP/1.1\n";

#ifdef USE_SSL
  package += "Host: " + String(ServerIP) + "\n" ; // should not use DEFAULT_SERVER_IP
  if(String(HTTP_Type) != "POST")
    package += "password-key: "+httppw+"\n";
#else
  package += "Host: "+(String)ServerIP+":"+(String)ServerPort+"\n";
#endif

  package += "Content-Type: application/json\n";
  package += "Connection: keep-alive\n"; // need HTTP/1.1 up , HTTP/1.0 not supported

  if (payload != "") {
    package += "Content-Length: " + String(String(payload).length()) + "\n\n";
    package += String(payload) + "\n\n";
  }

#ifdef debug_prepare_http_package
  Serial.println("[Prep_http_pack]\n------\n"+package+"------");
#endif

  return(package);
}
int Eth_TCP_Connect(void){// connected will return 0 or 1 , fail or success
  int error_code = 0;
  if( TCPclient.connected() != 1){
    Serial.println("[Eth_TCP]TCP break");
    Serial.println("[Eth_TCP] tcp connect to "+(String)ServerIP+", "+(String)ServerPort);
    //switch(error_code = TCPclient.connect(ServerIP, ServerPort)){
    switch(error_code = TCPclient.connect(c_toIPAddr(&ServerIP[0]), ServerPort)){
      case 1 : //Success
        Serial.println("[Eth_TCP] TCP successful");
        return 1;

      case  0: //FAIL
      case -1: //TIMED_OUT
      case -2: //INVALID_SERVER
      case -3: //TRUNCATED
      case -4: //INVALID_RESPONSE
        Serial.println("[Eth_TCP]Tcp Connect Error code ,"+(String)error_code);
        tcp_connect_error_times--;
        if(tcp_connect_error_times <= 0){ // tcp connection error too much times,check out the wifi/ethernet connection
#if defined(USE_ETHERNET)
          connect_to_ethernet();
#elif defined(USE_SSL)
          connect_to_wifi();
#endif
          tcp_connect_error_times = 10 ;
        }
        TCPclient.flush();
        TCPclient.stop();
        return 0;

      default:
        Serial.println("fail");
        Serial.println("[Eth_TCP] Unknow error,"+(String)error_code);
        return 0;
    }
  }

  return 1;

}
String read_ack_package(void){
  char * http_resp_package = (char*)malloc(400);
  String result = "";
  int package_size = 0;
  Serial.println("[ReadACK] enter");

  while( (package_size = TCPclient.available()) > 0){
    Serial.println("1.");
    memset(http_resp_package, 0, 100);
    Serial.println("2.");
    TCPclient.read((uint8_t*)http_resp_package, package_size>=400?399:package_size);
    Serial.println("3."+(String)http_resp_package);
    result += (String)http_resp_package;
    Serial.println("[readack] "+result);
  }
  free(http_resp_package);

  return result;
}
int decodehttp(httpresp *result){ //return payload_length , -1 is error
  int index = 0, httpcode, package_size = 0;
  int buz_size = 512;
  char * buf = (char*)malloc(buz_size);
  String package = "", packet_payload = "";
  int process = 0; // 0:not found '{'
                   // 1:found '{' but not found '}'
                   // 2:found '{' and '}'


  while( (package_size = TCPclient.available()) > 0 || process !=2){ // 可能要處理timeout的問題
    memset(buf, 0, buz_size);
    TCPclient.read((uint8_t*)buf, package_size>buz_size?buz_size:package_size);
    package = (String)buf;
    if(package.indexOf(2) > -1)
      package[package.indexOf(2)] =' ';

    //check HTTP status code
    index =  package.indexOf("HTTP/");
    if(index < 0 && process == 0){  // not found HTTP/
      result->HTTPStatusCode = GetHTTPCodeERROR;
      Serial.println("[DecodeHTTP] Not found HTTP/ ");
      return GetHTTPCodeERROR;
    }
    else if(package.indexOf(" OK") >= 0  && process == 0){ // get http status code
      result->HTTPStatusCode = ((uint8_t)package[index+9] - 48) * 100 + ((uint8_t)package[index+10] - 48) * 10 + (uint8_t)package[index+11] - 48;
    }

    // check content-length = 0
    if( package.indexOf("Content-Length: 0") >= 0 && process == 0) // ok work
      return 0;

    if( (index = package.indexOf("{")) >=0  && process == 0){
      packet_payload += package.substring(index);
      Serial.println("-----1-----\n"+packet_payload+"\n----------");
      process = 1;
    }
    
    if((index = package.indexOf("}")) >=0  && process == 1){
      packet_payload += package;
      Serial.println("-----2-----\n"+package+"\n----------");
      Serial.println("-----2-----\n"+packet_payload+"\n----------");
      process = 2;
    }
  }
  free(buf);

  if(process == 2){
    if(packet_payload.length() < HTTP_RESPONSE_PAYLOAD_SIZE){
      packet_payload.toCharArray(result->payload, packet_payload.length()+1);
      return packet_payload.length();
    }
    else{
      Serial.println("[GetHTTPPayload]HTTP_RESPONSE_PAYLOAD_SIZE not enough");
      Serial.println("[GetHTTPPayload]packet_payload:\n"+packet_payload);
      packet_payload.toCharArray(result->payload, HTTP_RESPONSE_PAYLOAD_SIZE);
      return HTTP_RESPONSE_PAYLOAD_SIZE;
    }
    return GetHTTPPayload_ERROR;
  }
  return GetHTTPPayload_ERROR;
}
void Send_HTTPS(httpresp *result, const char* HTTP_Type, const char* feature, const char* payload, bool close_TCP) {
#ifdef debug_SEND
  Serial.println("[Send]Start, "+(String)HTTP_Type);
#endif

  const int resp_timeout = 1000;
  unsigned long start_time = 0;

  //check tcp connection, build successful->1, fail->0
  if(Eth_TCP_Connect()!=1){
    result->HTTPStatusCode = TCP_CONNECT_ERROR;
    return;
  }

  // send http package using TCP connection
  TCPclient.println(prepare_http_package(HTTP_Type, feature, payload));
  TCPclient.flush();

  // receive response package
  start_time = millis();
  while (1) {
    // timeout return
    if(millis() - start_time >= resp_timeout){
      Serial.println("[SEND] wait http resp timeout");
      result->HTTPStatusCode = HTTP_ACK_TIMEOUT;
      return;
    }

    if(TCPclient.available() > 0){// if available > 0 mean that the sent http package has came back
      // decode HTTP package
      decodehttp(result); // decode that response package as http format
      return;
    }


  }

}
#endif


#ifdef USE_ETHERNET
IPAddress c_toIPAddr (char *ip){
  String S_ip = String(ip);
  int i_ip[4], i;
  int index = 0;
  index = S_ip.indexOf('.');
  i_ip[0] = S_ip.substring(0,index).toInt();
  
  for(i = 1; i < 3; i++){
    i_ip[i] = S_ip.substring(index+1, S_ip.indexOf('.',index+1)).toInt();
    index = S_ip.indexOf('.',index+1);
  }
  i_ip[3] = S_ip.substring(index+1).toInt();
  
  return(IPAddress(i_ip[0],i_ip[1],i_ip[2],i_ip[3]));
}
void connect_to_ethernet(void) {
  while (Ethernet.linkStatus() != 1) {
    Serial.print("[Ethernet]begin");
    Ethernet.begin(mac,myIP,myDNS,myGW,myMASK);//Ethernet.begin(mac);

    //if (Ethernet.localIP() != IPAddress(0,0,0,0) || state_code!= 0) {
    if(Ethernet.linkStatus() == 1){
      Serial.println(" successful");
      break;
    }
    else{
      Serial.println(" fail, "+String(Ethernet.linkStatus()));
    }
    delay(500);
  }
  Serial.print("[Ethernet]localIP:");
  Serial.println(Ethernet.localIP());
}

#endif

#ifdef USE_WIFI
/*  WiFi init
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
  uint8_t statesCode = read_WiFi_AP_Info();

  if( !(statesCode & 1) ){
    String(DEFAULT_SERVER_IP).toCharArray(ServerIP, 50);
    Serial.println("[WiFi_init] ServerIP: "+String(ServerIP));
  }

  if (WiFi.status() == WL_CONNECTED)
   return 1;
  else if ( (statesCode&0x04)&& (statesCode&0x02) )
    if(connect_to_wifi())
      return 1;

  AP_mode();

}
int connect_to_wifi(void){
  Serial.print("[WiFi]Connecting");

#ifndef FORCE_CONNECT
  unsigned long connecttimeout = millis();
  WiFi.softAPdisconnect(true);
  WiFi.begin(wifissid, wifipass);
  while (WiFi.status() != WL_CONNECTED && (millis() - connecttimeout < 10000) ) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
#else
  unsigned long connecttimeout = 0;
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
    Serial.println("[WiFi]Connect to "+String(wifissid)+" successful!");
    wifimode = 0 ;
    return 1;
  }
  else if (millis() - connecttimeout > 10000) {
    Serial.println("[WiFi]Fail");
    return 0;
  }
}
/* EEPROM
 * When connect to wifi successfully, it will rewrite the data in eeprom.
 * If Wifi disconnect, it won't rewrite the data.
 * Wait until Wifi recover, restart the esp12F, it is going to connect wifi again.
 */
void clr_eeprom(int force){ //clear eeprom (and wifi disconnect?)
  if (!force) {
    delay(3000);
  }
  if ( (force == 1) || (digitalRead(CLEAREEPROM) == LOW) ) {
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
  temp += "<br><br>IoTtalk Server IP(optional)<br>";
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
#ifdef debug_mode
    Serial.print("[");
    Serial.print(wifissid);
    Serial.print("][");
    Serial.print(wifipass);
    Serial.print("][");
    Serial.print(ServerIP);
    Serial.println("]");
#endif
    if(connect_to_wifi() == 1){//if(connect_to_wifi(wifissid, wifipass) == 1){
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
  String softapname = "ESP12F-";
  byte mac[6];
  WiFi.macAddress(mac);
  for (int i = 0; i < 6; ++i)
  {
    char buf[3];
    sprintf(buf, "%X", mac[i]);
    if(mac[i] < 0x10 )
      softapname += "0";
    softapname += buf;
  }
  Serial.println("[AP_Name]:"+softapname);

  IPAddress ip(192, 168, 0, 1);
  IPAddress gateway(192, 168, 0, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();
  WiFi.softAPConfig(ip, gateway, subnet);
  WiFi.softAP(&softapname[0]);

  start_web_server();
}
#endif
