#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include "ESP8266HTTPClient.h"

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
  /*
  http.setUserAgent("python-requests/2.18.1");
  http.addHeader("Connection","keep-alive");
  http.useHTTP10(true);
  http.addHeader("Accept-Encoding","gzip,deflate");
  */
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
  String Str_ret = http.getString();
  int httpCode;
  
  if(millis() - five_min >10000){
    //Serial.println("\n[HTTP] PUT..." + url);
    //http.begin(url);
    /*
    http.setUserAgent("python-requests/2.18.1");
    http.addHeader("Connection","keep-alive");
    http.useHTTP10(true);
    http.addHeader("Accept-Encoding","gzip,deflate");
    */
    //http.addHeader("Content-Type","application/json");
    //httpCode = http.PUT("{\"data\":["+String(random(0,10))+"]}");
    
    //Serial.println("[HTTP] POST... code:" + (String)httpCode );
    //Serial.println(http.getString());
    
    //http.end();
    
    /////////////////////////////////////
    delay(1000);
    Serial.println("---------------------------------------------------");
    /////////////////////////////////////
    http.begin(url);
    /*
    http.setUserAgent("python-requests/2.18.1");
    http.addHeader("Connection","keep-alive");
    http.useHTTP10(true);
    http.addHeader("Accept-Encoding","gzip,deflate");
    */
    http.addHeader("Content-Type","application/json");
    httpCode = http.GET();
    
    Serial.println("[HTTP] GET... code:" + (String)httpCode );
    Serial.println(http.getString());
    
    http.end();
    
    five_min =millis();
  }
  
  if (Str_ret != '\0'){
    //Str_ret = String('\0');
    //Serial.println(Str_ret);
  }
  
}

