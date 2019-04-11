#include "csmapi.h"
int continue_error_quota = 5;
extern char deviceid[37];
extern String ctrl_i, ctrl_o, d_name, rev;
#ifdef USE_SSL
extern String httppw;
#endif

#ifdef V1
String TS[DF_NUM];
// Keep comment
// (X) StaticJsonBuffer<256> JB_TS;
// (X) JsonObject& JO_TS = JB_TS.createObject();
// should not use 'global' jsonobject to store or write data, the details are on https://arduinojson.org/v5/faq/why-shouldnt-i-use-a-global-jsonbuffer/
// google search : "Why shouldn't I use a global JsonBuffer?"
// and see the official example :  JsonConfigFile.ino
#endif


#ifdef V1
void init_ODFtimestamp(void){
  for(int i = 0; i <= DF_NUM; i++)
    TS[i]="";
}
String pull(char *df_name){
  String old_time, new_time;
  String data;
  unsigned long get_time = millis();

  httpresp result;
  result.HTTPStatusCode = 0;
  result.payload = (char*)malloc(HTTP_RESPONSE_PAYLOAD_SIZE);
  memset(result.payload, 0, HTTP_RESPONSE_PAYLOAD_SIZE);
  GET(&result, df_name,0);


  if (result.HTTPStatusCode != 200) {
    Serial.println("[PULL]ERROR, " + String(df_name) + "," + String(result.HTTPStatusCode) +"\n"+String(result.payload));
    continue_error_quota--;
  }
  else {
    continue_error_quota = 5;
    StaticJsonBuffer<HTTP_RESPONSE_PAYLOAD_SIZE> JB_resp;
    JsonObject& JO_resp = JB_resp.parseObject(String(result.payload));

    int index = get_DF_index(String(df_name));

    if( TS[index] != JO_resp["samples"][0][0].as<String>() ){
      TS[index]    = JO_resp["samples"][0][0].as<String>(); //update timestamp
      String last_data = JO_resp["samples"][0][1][0].as<String>();
#ifdef debug_pull
      Serial.println("[PULL]"+String(df_name)+":"+last_data);
#endif
      if(result.payload != NULL)
        free(result.payload);
      return last_data;
    }
  }
  if(result.payload != NULL)
    free(result.payload);
  return "___NULL_DATA___"; // if HTTP code !=200 or no new data
}
#endif

String getProfile(void){
  const char* df_list[] = DF_LIST;
  String result;
  int i = 0;
  StaticJsonBuffer<512> JB_root;
  JsonObject& JO_root = JB_root.createObject();

#ifdef V1
  JsonObject& JO_profile = JO_root.createNestedObject("profile");
  JO_profile["d_name"] =  String(DM_NAME) + "." + String(deviceid).substring(8);
  JO_profile["dm_name"] = DM_NAME;
  JO_profile["is_sim"] = false;
  JsonArray& JO_df_list = JO_profile.createNestedArray("df_list");
  for(int i = 0; i < sizeof(df_list)/sizeof(char*); i++)// ArduinoMega point size = 2
    JO_df_list.add( String(df_list[i]) );

#elif defined V2
  JsonArray& odf_list = JO_root.createNestedArray("odf_list").createNestedArray();
  odf_list.add("ESP12F_ODF"); // some day , i will use for-loop to add it
  odf_list.createNestedArray();

  JsonArray& idf_list = JO_root.createNestedArray("idf_list").createNestedArray();
  idf_list.add("ESP12F_IDF");
  idf_list.createNestedArray();

  JsonObject& profile = JO_root.createNestedObject("profile");
  profile["model"] = "ESP12F";
  profile["u_name"] = "null";

  JsonArray& accept_protos = JO_root.createNestedArray("accept_protos");
  accept_protos.add("mqtt");
#endif

  JO_root.printTo(result);
  JB_root.clear();

#ifdef debug_getprofile
  Serial.println("[getProfile]"+result);
#endif
  return result;
}
int Register(void){ // retrun httpcod
#ifdef debug_register
  Serial.println("[Register]start");
#endif

  CheckNetworkStatus();

  httpresp result;
  result.HTTPStatusCode = 0;
  result.payload = (char*)malloc(HTTP_RESPONSE_PAYLOAD_SIZE);
  memset(result.payload, 0, HTTP_RESPONSE_PAYLOAD_SIZE);
#ifdef V1
  POST(&result, getProfile().c_str());
  while ( result.HTTPStatusCode != 200){
    Serial.println("[Register]Fail, code"+String(result.HTTPStatusCode));
    Serial.println(result.payload);
    delay(1000);
    memset(result.payload, 0, HTTP_RESPONSE_PAYLOAD_SIZE);
    POST(&result, getProfile().c_str());
  }
#elif defined V2
  while(result.HTTPStatusCode != 200){
    V2_PUT(&result, "140.113.215.7", "9992", String(deviceid), getProfile() );
  }
  get_ctrl_chan(String(result.payload));
  MQTT_Conn();
#endif

  Serial.println("[Register] Successful");

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
#ifdef debug_push
  Serial.println("[PUSH]" + String(df_name)+":"+String(value));
#endif

  httpresp result;
  result.HTTPStatusCode = 0;
  result.payload = (char*)malloc(sizeof(char*)*HTTP_RESPONSE_PAYLOAD_SIZE);
  memset(result.payload, 0, sizeof(char*)*HTTP_RESPONSE_PAYLOAD_SIZE);
  PUT(&result, value.c_str(), df_name);

  if (result.HTTPStatusCode == 200)
    continue_error_quota = 5;
  else{
    Serial.println("[PUSH] \""+String(df_name)+"\":" +value+", error:" + String(result.HTTPStatusCode) );
    continue_error_quota--;
  }

  if(result.payload != NULL)
    free(result.payload);

  return result.HTTPStatusCode;
}
