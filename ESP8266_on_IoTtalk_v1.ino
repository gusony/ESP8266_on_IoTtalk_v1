

#include "MyEsp8266.h"

long cycleTimestamp;
int continue_error_quota = 5;
extern char deviceid[37];
#ifdef USE_SSL
extern String httppw;
#endif


#ifdef V1
StaticJsonBuffer<256> JB_TS;//JsonBuffer Timestamp
JsonObject& JO_TS = JB_TS.createObject();
#endif

void init_ODFtimestamp(void){
  const char* df_list[] = DF_LIST;
  for(int i = 0; i < (sizeof(df_list)/4);i++)
    JO_TS[df_list[i]]="";
}
String  getProfile(void){
  const char* df_list[] = DF_LIST;
  String result;
  StaticJsonBuffer<512> JB_root;
  JsonObject& JO_root = JB_root.createObject();
  JsonObject& JO_profile = JO_root.createNestedObject("profile");

  JO_profile["d_name"] =  String(DM_NAME) + "." + String(deviceid).substring(8);
  JO_profile["dm_name"] = DM_NAME;
  JO_profile["is_sim"] = false;
  JsonArray& JO_df_list = JO_profile.createNestedArray("df_list");
  for(int i = 0; i < sizeof(df_list)/2; i++)
    JO_df_list.add( String(df_list[i]) );

  JO_root.printTo(result);
  JB_root.clear();

#ifdef debug_mode_getprofile
  Serial.println("[getProfile]"+result);
#endif
  return result;
}
int Register(void){ // retrun httpcod
#ifdef debug_mode_register
  Serial.println("[Register]start");
#endif
  CheckNetworkStatus();

  httpresp result;
  result.HTTPStatusCode = 0;
  result.payload = (char*)malloc(HTTP_RESPONSE_PAYLOAD_SIZE);
  memset(result.payload, 0, HTTP_RESPONSE_PAYLOAD_SIZE);
  POST(&result, getProfile().c_str());

  while ( result.HTTPStatusCode != 200){
    Serial.println("[Register]Fail, code"+String(result.HTTPStatusCode));
    delay(1000);
    memset(result.payload, 0, HTTP_RESPONSE_PAYLOAD_SIZE);
    POST(&result, getProfile().c_str());
  }

#ifdef USE_SSL
  StaticJsonBuffer<512> JB_root;
  JsonObject& JO_root = JB_root.parseObject(result.payload);
  httppw = JO_root["password"].as<String>();
  Serial.println("[Register]httppw:"+httppw);
#endif

  if(result.payload != NULL)
    free(result.payload);
  return (result.HTTPStatusCode);
}
int push(char *df_name, String value){  //return httpcode
#ifdef debug_mode
  Serial.println("[PUSH]" + String(df_name)+":"+String(value));
#endif

  httpresp result;
  result.HTTPStatusCode = 0;
  result.payload = (char*)malloc(HTTP_RESPONSE_PAYLOAD_SIZE);
  memset(result.payload, 0, HTTP_RESPONSE_PAYLOAD_SIZE);
  PUT(&result, value.c_str(), df_name);

  if (result.HTTPStatusCode != 200) {
    Serial.println("[PUSH] \""+String(df_name)+"\":" +value+"..." + String(result.HTTPStatusCode) );
    continue_error_quota--;
  }
  else
    continue_error_quota = 5;

  if(result.payload != NULL)
    free(result.payload);
  return result.HTTPStatusCode;
}
String pull(char *df_name){
  httpresp result;
  result.HTTPStatusCode = 0;
  result.payload = (char*)malloc(HTTP_RESPONSE_PAYLOAD_SIZE);
  memset(result.payload, 0, HTTP_RESPONSE_PAYLOAD_SIZE);
  GET(&result, df_name);


  if (result.HTTPStatusCode != 200) {
    Serial.println("[PULL]" + String(df_name) + "," + String(result.HTTPStatusCode) +"\n"+String(result.payload));
    continue_error_quota--;
  }
  else {
    continue_error_quota = 5;
    StaticJsonBuffer<HTTP_RESPONSE_PAYLOAD_SIZE> JB_resp;
    JsonObject& root = JB_resp.parseObject(String(result.payload));
    if( root["samples"][0][0].as<String>() !=  JO_TS[df_name].as<String>()) {
      JO_TS[df_name] = root["samples"][0][0].as<String>();
      String last_data = root["samples"][0][1][0].as<String>();
#ifdef debug_mode
      Serial.println("[PULL]"+String(df_name)+":"+last_data);
#endif
      if(result.payload != NULL)
        free(result.payload);
      return root["samples"][0][1][0].as<String>();
    }
  }
  if(result.payload != NULL)
    free(result.payload);
  return "___NULL_DATA___";
}

void setup(void){
  
  Init();
  Register();
  init_ODFtimestamp();
  cycleTimestamp = millis();
}
void loop(void){
  String result;
#ifdef USE_WIFI
  if (digitalRead(CLEAREEPROM) == LOW)
    clr_eeprom(0);
#endif

  if (continue_error_quota <= 0) {
    Serial.println("[Loop] Try to register");
    continue_error_quota = 5;
    Register();
  }


  if (millis() - cycleTimestamp > 1000) {
    //push("ESP12F_IDF", String(ESP8266TrueRandom.random() % 1000 + 1));
    push("ESP12F_IDF", String(random(100)) );
    delay(100);
    Serial.println("[loop]pull:"+pull("ESP12F_ODF"));
    cycleTimestamp = millis();
  }

}
