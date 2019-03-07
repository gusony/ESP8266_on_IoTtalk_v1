#include "csmapi.h"

extern int continue_error_quota;
extern long tcp_connect_time;
extern EthernetClient TCPclient;
#define TEST_DATA_NUM 1000
#define TEST_DATA_INTERVAL 1000
unsigned long timestamp=0;

void test_v1_latency(){
  int i = 0;
  Serial.println("test start");
  
  for (i = 0; i<TEST_DATA_NUM; i){
    Ethernet.maintain();
    String push_data = String(random(100));
    push("ESP12F_IDF", push_data); //15~17 ms
    Serial.println("[Test]push");
    unsigned long start_time = millis();
    //delay(30);
    while(millis() - start_time<TEST_DATA_INTERVAL){
      Serial.println("[Test]Pull");
      String Pull_result = pull("ESP12F_ODF");//18ms
      
      
      if(Pull_result != "___NULL_DATA___" && Pull_result == push_data && millis() - start_time < TEST_DATA_INTERVAL){
        Serial.println("[Test]"+(String)(millis() - start_time)+","+(String)(millis() - start_time - tcp_connect_time) );
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
  test_v1_latency();
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

  // if error happens too much times, try register
  if (continue_error_quota <= 0) { 
    Serial.println("[Loop] Try to register");
    continue_error_quota = 5;
    Register();
  }
  
  if( pull("ESP12F_testlatency") == "1" && millis - timestamp >=1000){
    test_v1_latency();
    timestamp = millis();
  }
  

}
