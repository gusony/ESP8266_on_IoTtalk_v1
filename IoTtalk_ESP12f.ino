#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include "ESP8266HTTPClient.h"

#define Serial Serial
const char* ssid     = "dlink DIR-632";
const char* password = "035731924";
String url = "http://140.113.131.75:9999/";
HTTPClient http;

unsigned long five_min;

void setup() {
  uint8_t MAC_array[6];
  int i;
  
  Serial.begin(115200);
  //Serial.println("----------Start----------");

  //connect to wifi
  Serial.println("--Connect o Wi-Fi--------");
  WiFi.begin(ssid, password);//進void setup 一開始就要先執行 後執行會連不上
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  //Append the mac address to url string
  WiFi.macAddress(MAC_array);//get esp12f mac address
  for (i=0;i<6;i++)
    url+=String(MAC_array[i],HEX);

  //send the register packet
  Serial.println("[HTTP] begin...");
  Serial.println("[HTTP] POST..." + url);
  
  http.begin(url);
  http.setUserAgent("python-requests/2.18.1");
  http.addHeader("Connection","keep-alive");
  http.useHTTP10(true);
  http.addHeader("Accept-Encoding","gzip,deflate");
  http.addHeader("Content-Type","application/json");
  int httpCode = http.POST("{\"profile\": {\"d_name\": \"87.Dummy_Device\", \"dm_name\": \"Dummy_Device\", \"is_sim\": false, \"df_list\": [\"Dummy_Sensor\", \"Dummy_Control\"]}}");

  Serial.println("[HTTP] POST... code:" + (String)httpCode );
  Serial.println(http.getString());

  http.end();

  url +="/Dummy_Sensor";
  Serial.println("[HTTP] URL: "+url);
  
  Serial.println("\n[HTTP] PUT..." + url);
  http.begin(url);
  http.setUserAgent("python-requests/2.18.1");
  http.addHeader("Connection","keep-alive");
  http.useHTTP10(true);
  http.addHeader("Accept-Encoding","gzip,deflate");
  http.addHeader("Content-Type","application/json");
  httpCode = http.PUT("{\"data\":["+String(random(0,10))+"]}");
  
  Serial.println("[HTTP] POST... code:" + (String)httpCode );
  Serial.println(http.getString());

  http.end();
  
  
  
  
  five_min = millis();
}

void loop() {
  String Str_ret = http.getString();
  
  if(millis() - five_min >5000){
    
    //http.begin(url);
    //http.useHTTP10(1);
    //http.addHeader("Connection","keep-alive");
    //http.addHeader("Accept-Encoding","gzip,deflate");
    //http.addHeader("Accept","*/*");
    //http.setUserAgent("python-requests/2.18.1");
    //http.addHeader("Content-Type","application/json");
    
    //int httpCode = http.PUT("{\"data\":["+String(random(0,10))+"]}");
  
    //Serial.println("[HTTP] POST... code:" + (String)httpCode );
    //Serial.println(http.getString());
  
    //http.end();
    //five_min =millis();
  }
  
  if (Str_ret != '\0'){
    //Str_ret = String('\0');
    //Serial.println(Str_ret);
  }
  
}

