#include "MyEsp8266.h"

#define Nofp_time 1
#define NofP 1000 // number of test packets
  

String url = "";
String df_name_list[nODF];
String df_timestamp[nODF];
long cycleTimestamp;
String result;
int continue_error_quota = 5;
extern const char* df_list[nODF];
extern HTTPClient http;
extern char ServerIP;


int     DFindex(char *df_name){
  for (int i = 0; i <= nODF; i++) {
    if (String(df_name) ==  df_name_list[i]){
      return i;
    }
    else if (df_name_list[i] == "") {
      df_name_list[i] = String(df_name);
      return i;
    }
  }
  return nODF + 1; // df_timestamp is full
}
void    init_ODFtimestamp(void){
  for (int i = 0; i <= nODF; i++){
    df_name_list[i] = "";
    df_timestamp[i] = "";
  }
}
String  getProfile(void){
  String result;
  StaticJsonBuffer<512> JB_root;
  JsonObject& JO_root = JB_root.createObject();
  JsonObject& JO_profile = JO_root.createNestedObject("profile");
  JO_profile["d_name"] =  String(DM_NAME) + "." + (mac[3] < 0x10 ? "0"+String(mac[3],HEX) : String(mac[3],HEX)) \
                                                + (mac[4] < 0x10 ? "0"+String(mac[4],HEX) : String(mac[4],HEX)) \
                                                + (mac[5] < 0x10 ? "0"+String(mac[5],HEX) : String(mac[5],HEX));
  JO_profile["dm_name"] = DM_NAME;
  JO_profile["is_sim"] = false;
  JsonArray& JO_df_list = JO_profile.createNestedArray("df_list");
  for(int i = 0; i < nODF; i++)
    JO_df_list.add( String(df_list[i]) );

  JO_root.printTo(result);
  JB_root.clear();
  return result;
}
int iottalk_register(void){
  url = "http://" + String(ServerIP) + ":9999/";

  

  uint8_t MAC_array[6];
  WiFi.macAddress(MAC_array);//get esp12f mac address
  for (int i = 0; i < 6; i++) {
    if ( MAC_array[i] < 0x10 ) url += "0";
    url += String(MAC_array[i], HEX);;    //Append the mac address to url string
  }

  //send the register packet
  Serial.println("[HTTP] POST..." + url);
  String profile = getProfile();

  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(profile);

  Serial.println("[HTTP] Register... code: " + (String)httpCode );
  Serial.println(http.getString());

  url += "/";
  return httpCode;
}

int push(char *df_name, String value){
  http.begin( url + String(df_name));
  http.addHeader("Content-Type", "application/json");
  String data = "{\"data\":[" + value + "]}";
  int httpCode = http.PUT(data);


  if (httpCode != 200) {
    continue_error_quota--;
    Serial.print("[HTTP] PUSH \"" + String(df_name) + "\"... code: " + (String)httpCode );

    if (httpCode == 400 ) {
      Serial.print(", Bad Request, format error");
    }

    if (continue_error_quota <= 0) {
      Serial.println(" retry to register");
      continue_error_quota = 5;
      while (httpCode != 200) {
        httpCode = iottalk_register();

        if (httpCode == 200)
          http.PUT(data);
        else
          delay(3000);
      }
    }
    Serial.println();
  }
  else {
    continue_error_quota = 10;
  }


  return httpCode;
}

/*
     pull one times
     the iottalk server will return a json format String
     eg.
     {
     "samples": [
         [
             "2018-05-03 13:34:21.050927",
             [
                 1
             ]
         ],
         [
             "2018-05-03 13:34:20.588854",
             [
                 2
             ]
         ]
      ]
     }
     so,
     root["samples"][0][0] is the timestamp
     root["samples"][0][1] is the data we want
*/
String pull(char *df_name){
  String get_ret_str;
  int httpCode;
  String temp_timestamp = "";
  String last_data = "";  //This last_data is used to fetch the timestamp.
  DynamicJsonBuffer jsonBuffer;

  http.begin( url + String(df_name) );
  http.addHeader("Content-Type", "application/json");
  httpCode = http.GET(); //http state code
  if (httpCode != 200) {
    Serial.println("[HTTP] PULL \"" + String(df_name) + "\"... code: " + (String)httpCode + ", retry to register.");
    return "___NULL_DATA___";
  }
  else {
    get_ret_str = http.getString();  //After send GET request , store the return string
    JsonObject& root = jsonBuffer.parseObject(get_ret_str);
    if (get_ret_str.indexOf("samples") >= 0) { // if not found the string , it will return -1
      temp_timestamp = root["samples"][0][0].as<String>();

      if (df_timestamp[DFindex(df_name)] != temp_timestamp) {
        df_timestamp[DFindex(df_name)] = temp_timestamp;
        last_data = root["samples"][0][1].as<String>();
        last_data[0] = ' ';
        last_data[last_data.length() - 1] = 0;
        return last_data;   // return the data.
      }
      else return "___NULL_DATA___";
    }
    else return "___NULL_DATA___";
  }
  http.end();
  /*
    while (httpCode != 200){
        digitalWrite(LEDPIN, HIGH);
        httpCode = iottalk_register();
        if (httpCode == 200) http.GET();
        else delay(3000);
    }
  */




}

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

void setup(void){
  pinMode(CLEAREEPROM, INPUT_PULLUP); //GPIO13: clear eeprom button
  randomSeed(analogRead(0));

  
  EEPROM.begin(512);
  Serial.begin(115200);
  uint8_t statesCode ;
  

  statesCode = 0;
  while (statesCode != 200) {
    statesCode = iottalk_register();
    if (statesCode != 200) {
      Serial.println("Retry to register to the IoTtalk server. Suspend 3 seconds.");
      
      if (digitalRead(CLEAREEPROM) == LOW) 
        clr_eeprom(0);
      
      delay(3000);
    }
  }

  
  init_ODFtimestamp();
  cycleTimestamp = millis();
}
void loop(void){
  if (digitalRead(CLEAREEPROM) == LOW) {
    clr_eeprom(0);
  }

  if (millis() - cycleTimestamp > 1000) {
/*    
    Temperature = (String)dht.readTemperature() != "nan" ? (String)dht.readTemperature() : Temperature;
    String push_data = get_GPS(Temperature);
    Serial.println("[ESP12F_Temp]" + push_data);
    push("ESP12F_Temp", push_data);
    delay(500);

    Humidity = (String)dht.readHumidity() != "nan" ? (String)dht.readHumidity() : Humidity;
    push_data = get_GPS(Humidity);
    Serial.println("[ESP12F_Humi]" + push_data);
    push("ESP12F_Humi", push_data);
    delay(500);

    pm25 = read_pm25();
    if (pm25 != "__no_data__") {
      push_data = get_GPS(pm25);
      Serial.println("[ESP12F_PM2.5]" + push_data);
      push("ESP12F_PM2.5", push_data);
    }
    delay(500);
*/
    push("ESP12F_IDF", String(ESP8266TrueRandom.random() % 1000 + 1));
    delay(500);

    result = pull("ESP12F_testlatency");
    if (result != "___NULL_DATA___") {
      if (result.toInt() == 0) {
        test_v1_latency();
      }
    }

    cycleTimestamp = millis();
  }

}
