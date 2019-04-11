#include "csmapi.h"


#define TEST_DATA_NUM 1000
#define TEST_DATA_INTERVAL 200
//#define TEST_V1

extern int continue_error_quota;
extern String IDF_topic;
extern PubSubClient MQTTclient;
extern bool new_message;
unsigned long timestamp=0;


#ifdef V2
extern long lastMsg, now;

#endif
#ifdef TEST_V1
void test_v1_latency(){
  int i = 0;
  unsigned long start_time = 0; 
  String Pull_result = "";
  String push_data = "";
  Serial.println("test start");
  
  for (i = 0; i<TEST_DATA_NUM; i){
    //push_data = String(random(100));
    push_data = String(i);
    push("ESP12F_IDF", push_data); //15~17 ms
    start_time = millis();
    
    while(millis() - start_time < TEST_DATA_INTERVAL){
      Pull_result = pull("ESP12F_ODF");
      // esp wifi 18ms on v1 
      // mega with ethernet need 38~40 ms on v1
      if(Pull_result == push_data && millis() - start_time < TEST_DATA_INTERVAL){
        Serial.println(millis() - start_time);
        i++;
        break;
      }
    }
    push_data = "";
    Pull_result = "";
    delay(300); // take a break
  }
  Serial.println("test finish");
}
#endif

void setup(){
  #warning test warning
  Init();
  Register();
#ifdef V1
  init_ODFtimestamp();
#endif
  delay(3000);
  timestamp = millis();
}
void loop(){
#ifdef USE_WIFI
  if (digitalRead(CLEAREEPROM) == LOW)
    clr_eeprom(0);
#endif

#ifdef USE_ETHERNET
  Ethernet.maintain();
#endif

#ifdef V2
  if(!MQTTclient.loop()) // like mqtt ping ,to make sure the connection between server
    Register();
    
  if(new_message)
    CtrlHandle();
  
  if (millis() - lastMsg > 1000 && IDF_topic != "" ) {
    lastMsg = millis();
    MQTTclient.publish(IDF_topic.c_str(), ("["+(String)lastMsg+"]").c_str());
    now = millis();
  }
#endif

#ifdef V1
  // if error happens too much times, try register
  if (continue_error_quota <= 0) { 
    Serial.println("[Loop] Try to register");
    continue_error_quota = 5;
    Register();
  }
#endif

#ifdef TEST_V1
  if( pull("ESP12F_testlatency") == "1" && millis() - timestamp >=1000){
    test_v1_latency();
    timestamp = millis();
  }
#endif
  

}
