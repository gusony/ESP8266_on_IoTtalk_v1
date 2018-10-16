#include "csmapi.h"

long cycleTimestamp;
extern int continue_error_quota;

void setup(){
  Init();
  Register();
  init_ODFtimestamp();
  cycleTimestamp = millis();
}
void loop(){
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
