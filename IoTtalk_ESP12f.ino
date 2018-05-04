#define DM_name  "ESP12F" 
#define DF_list  {"ESP12F_IDF", "ESP12F_ODF", "ESP12F_LED", "ESP12F_testlatency"}
#define nODF     10  // The max number of ODFs which the DA can pull.
#include "MyEsp8266.h"

extern Adafruit_SSD1306 display;
extern char IoTtalkServerIP[100];
HTTPClient http;
String url = "";
String df_name_list[nODF];
String df_timestamp[nODF];
long cycleTimestamp;
String result;
bool one_time = 0;
long latency[1000];


int iottalk_register(void)
{
    url = "http://" + String(IoTtalkServerIP) + ":9999/";  
    
    String df_list[] = DF_list;
    int n_of_DF = sizeof(df_list)/sizeof(df_list[0]); // the number of DFs in the DF_list
    String DFlist = ""; 
    for (int i=0; i<n_of_DF; i++){
        DFlist += "\"" + df_list[i] + "\"";  
        if (i<n_of_DF-1) DFlist += ",";
    }
  
    uint8_t MAC_array[6];
    WiFi.macAddress(MAC_array);//get esp12f mac address
    for (int i=0;i<6;i++){
        if( MAC_array[i]<0x10 ) url+="0";
        url+= String(MAC_array[i],HEX);;      //Append the mac address to url string
    }
 
    //send the register packet
    Serial.println("[HTTP] POST..." + url);
    String profile="{\"profile\": {\"d_name\": \"";
    profile += DM_name;
    profile += "." + String(MAC_array[4],HEX) + String(MAC_array[5],HEX);
    profile += "\", \"dm_name\": \"";
    profile += DM_name;
    profile += "\", \"is_sim\": false, \"df_list\": [";
    profile +=  DFlist;
    profile += "]}}";

    http.begin(url);
    http.addHeader("Content-Type","application/json");
    int httpCode = http.POST(profile);

    Serial.println("[HTTP] Register... code: " + (String)httpCode );
    Serial.println(http.getString());
    
    if(httpCode == 200)
      OLED_print("IoTtalk V1 \nRegister Successful!");
    //http.end();
    url +="/";  
    return httpCode;
}

void init_ODFtimestamp(void)
{
  for (int i=0; i<=nODF; i++) 
    df_timestamp[i] = "";
  
  for (int i=0; i<=nODF; i++) 
    df_name_list[i] = "";  
}

int DFindex(char *df_name)
{
    for (int i=0; i<=nODF; i++){
        if (String(df_name) ==  df_name_list[i]) return i;
        else if (df_name_list[i] == ""){
            df_name_list[i] = String(df_name);
            return i;
        }
    }
    return nODF+1;  // df_timestamp is full
}

int push(char *df_name, String value)
{
    http.begin( url + String(df_name));
    http.addHeader("Content-Type","application/json");
    String data = "{\"data\":[" + value + "]}";
    int httpCode = http.PUT(data);
    if (httpCode != 200) Serial.println("[HTTP] PUSH \"" + String(df_name) + "\"... code: " + (String)httpCode + ", retry to register.");
    while (httpCode != 200){
        digitalWrite(LEDPIN, HIGH);
        httpCode = iottalk_register();
        if (httpCode == 200)  http.PUT(data);
        else delay(3000);
    }    
    return httpCode;
}

String pull(char *df_name)
{
  /*
   * pull one times
   * the iottalk server will return a json format String
   * eg. 
   * {
   * "samples": [
   *     [
   *         "2018-05-03 13:34:21.050927",
   *         [
   *             1
   *         ]
   *     ],
   *     [
   *         "2018-05-03 13:34:20.588854",
   *         [
   *             2
   *         ]
   *     ]
   *  ]
   * }
   * so,
   * root["samples"][0][0] is the timestamp
   * root["samples"][0][1] is the data we want
   */
  String get_ret_str;
  int httpCode;
  String temp_timestamp="";
  String last_data = "";  //This last_data is used to fetch the timestamp.
  DynamicJsonBuffer jsonBuffer;

  http.begin( url + String(df_name) );
  http.addHeader("Content-Type","application/json");
  httpCode = http.GET(); //http state code
  if (httpCode != 200) {
    Serial.println("[HTTP] PULL \"" + String(df_name) + "\"... code: " + (String)httpCode + ", retry to register.");
  }
  
  while (httpCode != 200){
      digitalWrite(LEDPIN, HIGH);
      httpCode = iottalk_register();
      if (httpCode == 200) http.GET();
      else delay(3000);
  }
  get_ret_str = http.getString();  //After send GET request , store the return string
  JsonObject& root = jsonBuffer.parseObject(get_ret_str);
  http.end();

  
  if (get_ret_str.indexOf("samples") >=0){ // if not found the string , it will return -1
      temp_timestamp = root["samples"][0][0].as<String>();
      
      if (df_timestamp[DFindex(df_name)] != temp_timestamp){
          df_timestamp[DFindex(df_name)] = temp_timestamp;
          last_data = root["samples"][0][1].as<String>();
          last_data[0] = ' ';
          last_data[last_data.length()-1] = 0;
          OLED_print("PULL '"+(String)df_name+"':\n"+last_data);
          return last_data;   // return the data.
       }
       else return "___NULL_DATA___";
  }
  else return "___NULL_DATA___";
}

void test_v1_latency(void)
{
  String result;
  float average;
  long send_timestamp, mis, sum=0;
  int i, j, NofP= 1000; //number of packets

  for(i=0; i<NofP; i){
    //push
    send_timestamp = millis() ;
    push("ESP12F_IDF", String(send_timestamp) );

    //pull
    while(millis()-send_timestamp < 500){
      result = pull("ESP12F_ODF");
      if (result != "___NULL_DATA___" && result.toInt() == send_timestamp){
        mis = millis();
        latency[i++] = mis - send_timestamp;
        Serial.println( String( latency[i-1] ));
        break;
      }
    }
  }
  
  for(j=0; j<NofP; j++){
    sum += latency[j];
  }
  
  average = sum/NofP;
  Serial.println("Average = "+(String)average);
}

void setup(void) 
{
    pinMode(CLEAREEPROM, INPUT_PULLUP); //GPIO13: clear eeprom button
    pinMode(LEDPIN, OUTPUT);//GPIO2 : on board led
    digitalWrite(LEDPIN,HIGH);
    pinMode(16, OUTPUT);//GPIO16 : relay signal
    digitalWrite(16,LOW);
    randomSeed(analogRead(0));
  
    init_ssd1306();
  
    EEPROM.begin(512);
    Serial.begin(115200);
  
    char wifissid[100]="";
    char wifipass[100]="";
    int statesCode = read_WiFi_AP_Info(wifissid, wifipass, IoTtalkServerIP);
  
    if (!statesCode){
      connect_to_wifi(wifissid, wifipass);
    }
    else{
        Serial.println("Laod setting failed! statesCode: " + String(statesCode)); // StatesCode 1=No data, 2=ServerIP with wrong format
        ap_setting();
    }
    //while(wifimode) server.handleClient(); //waitting for connecting to AP ;
  
    statesCode = 0;
    while (statesCode != 200) {
        statesCode = iottalk_register();
        if (statesCode != 200){
            Serial.println("Retry to register to the IoTtalk server. Suspend 3 seconds.");
            if (digitalRead(CLEAREEPROM) == LOW) clr_eeprom(0);
            delay(3000);
        }
    }
    init_ODFtimestamp();
    cycleTimestamp = millis();
}

void loop(void)
{
    if (digitalRead(CLEAREEPROM) == LOW){
        clr_eeprom(0);
    }
    
      
      
    if (millis() - cycleTimestamp > 100) {
      result = pull("ESP12F_testlatency");
      if (result != "___NULL_DATA___"){
        if (result.toInt() == 0) {
          test_v1_latency();
        }
      }
      
      push("ESP12F_IDF", String(ESP8266TrueRandom.random()%1000+1));
      cycleTimestamp = millis();
    }

}
