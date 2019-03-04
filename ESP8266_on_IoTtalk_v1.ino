#include "csmapi.h"

//long cycleTimestamp;
extern int continue_error_quota;
#define TEST_DATA_NUM 1000
#define TEST_DATA_INTERVAL 1000


void test_v1_latency(){
  int i = 0;
  Serial.println("test start");
  
  for (i = 0; i<TEST_DATA_NUM; i){
    
    String push_data = String(random(100));
    push("ESP12F_IDF", push_data); //15~17 ms
    long start_time = millis();
    while(millis() - start_time<TEST_DATA_INTERVAL){
      String Pull_result = pull("ESP12F_ODF");//18ms
      
      if(Pull_result != "___NULL_DATA___" && Pull_result == push_data && millis() - start_time < TEST_DATA_INTERVAL){
        Serial.println((String)(millis() - start_time) );
        i++;
        break;
      }
    }
    delay(100); // take a break
  }
  
  
  Serial.println("test finish");
}
void setup(){
  #warning test warning
  Init();
  Register();
  init_ODFtimestamp();
  //cycleTimestamp = millis();
  //test_v1_latency();
}
void loop(){
#ifdef USE_WIFI
  if (digitalRead(CLEAREEPROM) == LOW)
    clr_eeprom(0);
#endif

  // if error happens too much times, try register
  if (continue_error_quota <= 0) { 
    Serial.println("[Loop] Try to register");
    continue_error_quota = 5;
    Register();
  }

  if( pull("ESP12F_testlatency") == "1"){
    test_v1_latency();
  }

//  if (millis() - cycleTimestamp > 300) { //每一秒push 一次資料
//    long start_time = millis();
//    push("ESP12F_IDF", String(random(100))); //15~17 ms
//    
//    String Pull_result = pull("ESP12F_ODF");//18ms
//    
//    if(Pull_result != "___NULL_DATA___"){
//      Serial.println(millis() - start_time);
//    }
//    cycleTimestamp = millis();
//    delay(10);
//  }

  
  

}
