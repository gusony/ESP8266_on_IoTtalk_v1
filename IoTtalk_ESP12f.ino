/*
 * IoTtalk - ESP12F Version 5.0 
 */
 
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFiMulti.h>
#include "ESP8266HTTPClient.h"

/*const*/ char* ssid = "";
/*const*/ char* password = "";
String url = "http://140.113.199.199:9999/";
HTTPClient http;
uint8_t wifimode = 1; //1:AP , 0: STA 
unsigned long five_min;

IPAddress ip(192,168,0,1);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);

ESP8266WebServer server ( 80 );

void handleRoot() {
  String temp="<html><form action=\"action_page.php\">";
  temp += "SSID:<br>";
  temp += "<input type=\"text\" name=\"SSID\" value=\"\"><br>";
  temp += "Password:<br>";
  temp += "<input type=\"text\" name=\"Password\" value=\"\">";
  temp += "<br><br><input type=\"submit\" value=\"Submit\">";
  temp += "</form></html>";
  server.send ( 200, "text/html", temp );
}

void handleNotFound() {
  if (server.arg(0) != "")
    wifimode=0;
}

void setup() {
  uint8_t MAC_array[6];
  int i;
  char temp[100]="                                                    ";
  char temp2[100]="                                                    ";
  Serial.begin(115200);
  pinMode(2, OUTPUT);//GPIO2
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(ip,gateway,subnet);
  WiFi.softAP("IoTtalk-ESP12F");  
  Serial.println(WiFi.softAPIP());
  
  if ( MDNS.begin ( "esp8266" ) ) 
    Serial.println ( "MDNS responder started" );
    
  server.on ( "/", handleRoot );
  server.onNotFound ( handleNotFound );
  server.begin();
  Serial.println ( "AP started" );

  while(wifimode){
    server.handleClient();
  }
  /////////////////////////////////////////////////////
  Serial.println("-----Connect to Wi-Fi-----");
  server.arg(0).toCharArray(temp,100);
  server.arg(1).toCharArray(temp2,100);
  //Serial.println(ssid);
  //Serial.println(password);
  
  WiFi.begin(temp, temp2);//進void setup 一開始就要先執行 後執行會連不上
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println ( WiFi.localIP() );
  //Append the mac address to url string
  WiFi.macAddress(MAC_array);//get esp12f mac address
  for (i=0;i<6;i++){
    if( MAC_array[i]<0x10 )
      url+="0";
    url+=String(MAC_array[i],HEX);
  }
  
  //send the register packet
  Serial.println("[HTTP] begin...");
  Serial.println("[HTTP] POST..." + url);
  
  http.begin(url);
  http.addHeader("Content-Type","application/json");
  int httpCode = http.POST("{\"profile\": {\"d_name\": \"01.ESP12F\", \"dm_name\": \"ESP12F\", \"is_sim\": false, \"df_list\": [\"esp12f_LED\"]}}");

  Serial.println("[HTTP] POST... code:" + (String)httpCode );
  Serial.println(http.getString());

  http.end();
  
  url +="/esp12f_LED";
  Serial.println("[HTTP] URL: "+url);
  
  five_min = millis();
}

void loop() {
  //Serial.printf("Stations connected = %d\n", WiFi.softAPgetStationNum());

  
  int httpCode;//http state code
  int i ;
  String get_ret_str;//After send GET request , store the return string
  int Brackets_index;// find the third '[' in get_ret_str 
  
  
  if(millis() - five_min >1000 ) {
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
  }
  

}
