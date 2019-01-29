#include "csmapi.h"

long cycleTimestamp;
extern int continue_error_quota;


void test_v1_latency(){
  int i = 0;
  int j = 1;
  Serial.println("test start");
  
  for (i = 0; i<1000; i){
    if (millis() - cycleTimestamp > 1000) { //每一秒push 一次資料
      long start_time = millis();
      String push_data = String(random(100));
      push("ESP12F_IDF", push_data); //15~17 ms
      delay(10);
      while(millis() - start_time<200){
        String Pull_result = pull("ESP12F_ODF");//18ms
        
        
        //Serial.print("["+push_data+","+Pull_result+"] ");
        if(millis() - start_time >=200 )
          break;
        else if(Pull_result != "___NULL_DATA___" && Pull_result == push_data){
          Serial.println(millis() - start_time-10*j);
          i++;
          j=1;
          break;
        }
        delay(10);
        j++;
      }
      cycleTimestamp = millis();
      delay(10);
    }
  }
  
  Serial.println("test finish");
}
void setup(){
  #warning test warning
  Init();
  Register();
  init_ODFtimestamp();
  cycleTimestamp = millis();
  test_v1_latency();
}
void loop(){
#ifdef USE_WIFI
  if (digitalRead(CLEAREEPROM) == LOW)
    clr_eeprom(0);
#endif

  if (continue_error_quota <= 0) { // if error happens too much times, try register  
    Serial.println("[Loop] Try to register");
    continue_error_quota = 5;
    Register();
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
