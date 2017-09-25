#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include "ESP8266HTTPClient.h"
#include <String.h>

#define Serial Serial
const char* ssid     = "MIRC311";//"dlink DIR-632";
const char* password = "pcs54784";//"035731924";
String url = "http://140.113.199.199:9999/";
HTTPClient http;

unsigned long five_min;

void setup() {
  uint8_t MAC_array[6];
  int i;
  
  Serial.begin(115200);
  pinMode(2, OUTPUT);//GPIO2
  
  //connect to wifi
  Serial.println("-----Connect to Wi-Fi-----");
  WiFi.begin(ssid, password);//進void setup 一開始就要先執行 後執行會連不上
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

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

