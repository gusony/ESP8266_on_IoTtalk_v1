

#include "MyEsp8266.h"

long cycleTimestamp;
int continue_error_quota = 5;
const char* df_list[] = DF_LIST;
#ifdef V1
StaticJsonBuffer<256> JB_TS;//JsonBuffer Timestamp
JsonObject& JO_TS = JB_TS.createObject();
#endif

extern char deviceid[37];
#ifdef USE_SSL
extern String httppw;
#endif

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
  JO_root.printTo(result);
  JB_root.clear();
  
#ifdef debug_mode
  Serial.println("[getProfile]"+result);
#endif
  return result;
}
int Register(void){ // retrun httpcode
  Serial.println("[Register]start");
  if( WiFi.status() != WL_CONNECTED )
    connect_to_wifi();
  
  httpresp httpresponse = POST(getProfile().c_str());
  while ( httpresponse.HTTPStatusCode != 200){
      Serial.println("[Register]Fail, code"+String(httpresponse.HTTPStatusCode));
      delay(1000);
      httpresponse = POST(getProfile().c_str());
  }
  StaticJsonBuffer<512> JB_root;
  JsonObject& JO_root = JB_root.parseObject(httpresponse.payload);
  httppw = JO_root["password"].as<String>();
  Serial.println("[Register]httppw:"+httppw);
  
  return (httpresponse.HTTPStatusCode);
}
int push(char *df_name, String value){  //return httpcode
#ifdef debug_mode
  Serial.println("[PUSH]" + String(df_name)+":"+String(value));
#endif

  int httpCode = PUT(value.c_str(), df_name).HTTPStatusCode;
  if (httpCode != 200) {
    Serial.println("[PUSH] \""+String(df_name)+"\":" +value+"..." + (String)httpCode );
    continue_error_quota--;
  }
  else
    continue_error_quota = 5;
  return httpCode;
}
String pull(char *df_name){
  httpresp resp_package = GET(df_name);
#ifdef debug_mode
  Serial.println("[PULL]" + String(df_name) + "," + String(resp_package.HTTPStatusCode) +"\n"+String(resp_package.payload)+"----------------------");  
#endif
  if (resp_package.HTTPStatusCode != 200) {
    Serial.println("[PULL]" + String(df_name) + "," + String(resp_package.HTTPStatusCode) +"\n"+String(resp_package.payload)+"----------------------");  
    continue_error_quota--;
  }
  else {
    continue_error_quota = 5;
    StaticJsonBuffer<512> JB_resp;
    JsonObject& root = JB_resp.parseObject(String(resp_package.payload));
    if( root["samples"][0][0].as<String>() !=  JO_TS[df_name].as<String>()) {
      JO_TS[df_name] = root["samples"][0][0].as<String>();
      String last_data = root["samples"][0][1][0].as<String>();
#ifdef debug_mode
      Serial.println("[PULL]"+String(df_name)+":"+last_data);
#endif
      return root["samples"][0][1][0].as<String>();
    }
  }
  return "___NULL_DATA___";
}

void setup(void){
  pinMode(CLEAREEPROM, INPUT_PULLUP); //GPIO13: clear eeprom button
  randomSeed(analogRead(0));
  EEPROM.begin(512);
  Serial.begin(115200);
  //clr_eeprom(1);
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
    Register();
  }
  

  if (millis() - cycleTimestamp > 1000) {
    push("ESP12F_IDF", String(ESP8266TrueRandom.random() % 1000 + 1));
    delay(100);
    Serial.println("[loop]pull"+pull("ESP12F_ODF"));
    cycleTimestamp = millis();
  }

}
