/*
 * IoTtalk V2 - ESP12F Version 6.0 
 * can register on iottalk V2 server
 */
 
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFiMulti.h>
#include "ESP8266HTTPClient2.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>

/*const*/ char* ssid = "";
/*const*/ char* password = "";
String url = "http://140.113.199.198:9992/";
const char* mqtt_server = "140.113.199.198";
#define mqtt_port  1883
HTTPClient http;
uint8_t wifimode = 1; //1:AP , 0: STA 
unsigned long five_min;

IPAddress ip(192,168,0,1);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);

ESP8266WebServer server ( 80 );
WiFiClient espClient;
PubSubClient client(espClient);
DynamicJsonBuffer jsonBuffer;
String uuid;

void handleRoot(void) {
  String temp="<html><form action=\"action_page.php\">";
  temp += "SSID:<br>";
  temp += "<input type=\"text\" name=\"SSID\" value=\"\"><br>";
  temp += "Password:<br>";
  temp += "<input type=\"text\" name=\"Password\" value=\"\">";
  temp += "<br><br><input type=\"submit\" value=\"Submit\">";
  temp += "</form></html>";
  server.send ( 200, "text/html", temp );
}

void handleNotFound(void) {
  if (server.arg(0) != "")
    wifimode=0;
}

void uuid4(void){
  int i ;
  for (i =0; i<36; i++)
    if(i==8 || i==13 || i==18 || i==23 )
      uuid+='-';
    else
      uuid+=String(random(16),HEX);
  
  url+=uuid;
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void init_wifi(void){
  /*
   * user key in the ssid and password of wifi ap ,
   * and then esp8266 connect to wifi
   */
  char wifi_ssid[100]=" ";
  char wifi_pass[100]=" ";
  uint8_t MAC_array[6];//maybe not need
  
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(ip,gateway,subnet);
  WiFi.softAP("IoTtalk-ESP12F");  
  //Serial.println(WiFi.softAPIP());
  
  if ( MDNS.begin ( "esp8266" ) ) 
    Serial.println ( "MDNS responder started" );
  /*  
  server.on ( "/", handleRoot );
  server.onNotFound ( handleNotFound );
  server.begin();
  Serial.println ( "AP started" );

  while(wifimode){ server.handleClient(); }
  
  Serial.println("-----Connect to Wi-Fi-----");
  server.arg(0).toCharArray(wifi_ssid,100);
  server.arg(1).toCharArray(wifi_pass,100);
  */
  //WiFi.begin(wifi_ssid, wifi_pass);//進void setup 一開始就要先執行 後執行會連不上
  WiFi.begin("dir632","035731924");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println ( WiFi.localIP() );
}

void reg_iot(void){
  /*
   * 1. generate UUID of this device
   * 2. send http PUT packet to server
   * 3. send mqtt connect packet
   * 4. send mqtt subscrib packet
   * 5. send mqtt publish packet
   */
  String temp, str_http_ret,temp2;
  char willtopic[100];
  int willqos=true;
  int willretain=0;
  char willmessage[100];
  int pub_sub_code;
  
  uuid4();
  
  
  //
  Serial.println("[HTTP] Begin register...");
  Serial.println("[HTTP] PUT /" + url);  
  http.begin(url);
  http.addHeader("Content-Type","application/json");
  int httpCode = http.PUT("{\"name\": \"Dummy_ESP8266\", \"idf_list\": [[\"Dummy_Sensor\", [\"None\"]]], \"odf_list\": [[\"Dummy_Control\", [\"None\"]]], \"accept_protos\": [\"mqtt\"], \"profile\": {\"model\": \"Dummy_Device\"}}");
  Serial.println("[HTTP] POST... code:" + (String)httpCode );
  Serial.print("Return : ");
  str_http_ret = http.getString();
  JsonObject& root = jsonBuffer.parseObject(str_http_ret);
  Serial.println(str_http_ret);
  http.end();

  
  root["ctrl_chans"][0].as<String>().toCharArray(willtopic,100);
  temp="{\"state\": \"broken\", \"rev\": \"" + root["rev"].as<String>() +"\"}" ;
  temp.toCharArray(willmessage,100);
  
  Serial.print("Attempting MQTT connection...");
  // Attempt to connect
  if (client.connect(&uuid[0], &willtopic[0], willqos, boolean(willretain), &willmessage[0])) {
    Serial.println("connected");
    
    temp = root["ctrl_chans"][1].as<String>();
    if(client.subscribe(&temp[0],1))
      Serial.println("subscribe successed");
    Serial.println("subscribe:"+temp);
    
    temp = root["ctrl_chans"][0].as<String>();
    temp2 = "{\"state\": \"online\", \"rev\": \"" + root["rev"].as<String>() +"\"}";
    if(client.publish(&temp[0],&temp2[0]))
      Serial.println("publish successed");
    Serial.println("publish:"+temp);
  } 
  else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");
    // Wait 5 seconds before retrying
    delay(5000);
  }
}

void setup() {
  int i;
  
  Serial.begin(115200);
  pinMode(2, OUTPUT);//GPIO2
  
  init_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback);
  reg_iot();

  five_min = millis();
}

void loop() {
  //Serial.printf("Stations connected = %d\n", WiFi.softAPgetStationNum());

  
  int httpCode;//http state code
  int i ;
  String get_ret_str;//After send GET request , store the return string
  int Brackets_index;// find the third '[' in get_ret_str 
  
  
  if(millis() - five_min >1000 ) {

    
    /*
    Serial.println("---------------------------------------------------");
    http.begin(url);
    http.addHeader("Content-Type","application/json");
    httpCode = http.GET();
    
    Serial.println("[HTTP] GET... code:" + (String)httpCode );
    get_ret_str = http.getString();
    Serial.println(get_ret_str);
    Brackets_index=0;
    for (i=0;i<3;i++)
      Brackets_index=get_ret_str.indexOf("[",Brackets_index+1);
    
    if (get_ret_str[Brackets_index+1] == '1')
      digitalWrite(2,HIGH);
    else if(get_ret_str[Brackets_index+1] == '0')
      digitalWrite(2,LOW);
    
    http.end();
    
    five_min =millis();
    */
  }
  

}
