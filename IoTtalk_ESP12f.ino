#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

#define Serial Serial

ESP8266WiFiMulti WiFiMulti;


void setup() {
  String url = "http://140.113.131.75:9999/AC220B8B1C51";

  Serial.begin(115200);
  Serial.println("----------Start----------");

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }
  Serial.println("--Connect o Wi-Fi--------");
  WiFiMulti.addAP("dlink DIR-632", "035731924");

  if ((WiFiMulti.run() == WL_CONNECTED)) {
    Serial.print("[HTTP] begin...\n");
    
    //Register Device
    HTTPClient http;
    Serial.println("[HTTP] POST..." + url);
    
    http.begin(url);
    http.setUserAgent("python-requests/2.18.1");
    http.addHeader("Connection","keep-alive");
    http.useHTTP10(1);
    http.addHeader("Accept-Encoding","gzip,deflate");
    http.addHeader("Content-Type","application/json");
    int httpCode = http.POST("{\"profile\": {\"d_name\": \"10.Dummy_Device\", \"dm_name\": \"Dummy_Device\", \"is_sim\": false, \"df_list\": [\"Dummy_Sensor\", \"Dummy_Control\"]}}");

    Serial.println("[HTTP] POST... code:" + (String)httpCode );
    Serial.println(http.getString());

    http.end();
  }
}

void loop() {
}

