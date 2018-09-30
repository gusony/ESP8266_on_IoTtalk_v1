

#include "MyEsp8266.h"

String url = "";
long cycleTimestamp;
int continue_error_quota = 5;

const char* df_list[] = DF_LIST;
StaticJsonBuffer<256> JB_TS;//JsonBuffer Timestamp
JsonObject& JO_TS = JB_TS.createObject();


//extern HTTPClient httpclient;
//extern char ServerIP[50];
//extern byte mac[6];
extern char deviceid[37];



void init_ODFtimestamp(void){
  for(int i = 0; i < (sizeof(df_list)/4);i++)
    JO_TS[df_list[i]]="";
}
String  getProfile(void){
  String result;
  StaticJsonBuffer<512> JB_root;
  JsonObject& JO_root = JB_root.createObject();
  JsonObject& JO_profile = JO_root.createNestedObject("profile");
  JO_profile["d_name"] =  String(DM_NAME) + "." + String(deviceid).substring(7);
  JO_profile["dm_name"] = DM_NAME;
  JO_profile["is_sim"] = false;
  JsonArray& JO_df_list = JO_profile.createNestedArray("df_list");
  for(int i = 0; i < sizeof(df_list)/4; i++)
    JO_df_list.add( String(df_list[i]) );
#ifdef debug_mode
  Serial.print("[d_name]");
  Serial.println(JO_profile["d_name"].as<String>());
#endif
  JO_root.printTo(result);
  JB_root.clear();
  return result;
}
int Register(void){ // retrun httpcode
  Serial.println("[Register]start");
  int httpstatuscode = POST(getProfile().c_str()).HTTPStatusCode;
  while ( httpstatuscode != 200){
      Serial.println("[Register]Fail, code"+String(httpstatuscode));
      delay(1000);
      httpstatuscode = POST(getProfile().c_str()).HTTPStatusCode;
  }
  return (httpstatuscode);
}
int push(char *df_name, String value){  //return httpcode
/*
  httpclient.begin( url + String(df_name));
  httpclient.addHeader("Content-Type", "application/json");
  String data = "{\"data\":[" + value + "]}";
  int httpCode = httpclient.PUT(data);
*/
#ifdef debug_mode
  Serial.println("[PUSH]" + String(df_name)+":"+String(value));
#endif

  int httpCode = PUT(value.c_str(), df_name).HTTPStatusCode;
  // get response
  if (httpCode != 200) {
    Serial.println("[PUSH] \""+String(df_name)+"\":" +value+"..." + (String)httpCode );
    continue_error_quota--;
  }
  return httpCode;
}
String pull(char *df_name){
  httpresp resp_package = GET(df_name);
  
  if (resp_package.HTTPStatusCode != 200) {
    Serial.println("[PULL] \"" + String(df_name) + "\"..." + String(resp_package.HTTPStatusCode) +"\n"+String(resp_package.payload));
    continue_error_quota--;
  }
  else {
    StaticJsonBuffer<512> JB_resp;
    JsonObject& root = JB_resp.parseObject(String(resp_package.payload));
    if( root["samples"][0][0].as<String>() !=  JO_TS[df_name].as<String>()) {//if( (timestamp = root["samples"][0][0].as<String>()) != "") {//if (JO_TS[df_name].as<String>() != timestamp) {
      JO_TS[df_name] = root["samples"][0][0].as<String>();
      String last_data = root["samples"][0][1][0].as<String>();
#ifdef debug_mode
      Serial.println("[PULL]"+String(df_name)+":"+last_data);
#endif
      return root["samples"][0][1][0].as<String>();
      //}
    }
  }
  return "___NULL_DATA___";
}

/*
void test_v1_latency(void){
  String rep;
  float average;
  long send_timestamp, sum = 0;
  long push_time, time1;
  int i, j, k; //number of packets
  long latency[Nofp_time * NofP];

  Serial.println("Start test latency, please waiting");
  //OLED_print("Start test \nlatency");

  for (k = 0; k < Nofp_time; k++)
    for (i = 0; i < NofP; i) {
      //push
      send_timestamp = millis();
      push("ESP12F_IDF", String(send_timestamp) );
      push_time = millis() - send_timestamp;
      //Serial.println("push_time = " + (String)push_time);

      //pull
      while (millis() - send_timestamp < 500) {
        rep = pull("ESP12F_ODF");
        if (rep != "___NULL_DATA___" && rep.toInt() == send_timestamp && millis() - send_timestamp < 500) {
          latency[i++] = millis() - send_timestamp - push_time;
          break;
        }
      }
    }
  for (i = 0; i < NofP; i++) {
    Serial.println(latency[i]);
    sum += latency[i];
  }

  //  average = sum/NofP;
  //  Serial.println("Average = "+(String)average);
  Serial.println("Test finish");
  //OLED_print("Test finish");

}
*/
void setup(void){
  pinMode(CLEAREEPROM, INPUT_PULLUP); //GPIO13: clear eeprom button
  randomSeed(analogRead(0));
  EEPROM.begin(512);
  Serial.begin(115200);
  
  SetDeviceID();
  WIFI_init();
  Register();
  init_ODFtimestamp();
  cycleTimestamp = millis();
}
void loop(void){
  String result;
  if (digitalRead(CLEAREEPROM) == LOW) 
    clr_eeprom(0);

  if (continue_error_quota <= 0) {
    Serial.println("[Loop] Try to register");
    continue_error_quota = 5;
    while (Register() != 200)
      delay(1000);
  }
  

  if (millis() - cycleTimestamp > 500) {
    push("ESP12F_IDF", String(ESP8266TrueRandom.random() % 1000 + 1));
    delay(100);

    result = pull("ESP12F_ODF");
   
    /*if (result != "___NULL_DATA___") {
      if (result.toInt() == 0) {
        test_v1_latency();
      }
    }*/

    cycleTimestamp = millis();
  }

}
