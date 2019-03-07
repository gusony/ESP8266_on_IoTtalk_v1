/* declare */
#ifdef USE_PM25
  SoftwareSerial pms(PMS_RX, PMS_TX);
#endif

#ifdef USE_GPS
  SoftwareSerial GPS(GPS_RX, GPS_TX);
#endif

#ifdef USE_SSD1306
  #ifdef USE_SSD1306_SPI
    Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
  #elif defined SSD1306_IIC  //SSD1306 with IIC
    Adafruit_SSD1306 display(OLED_RESET);
  #endif
#endif

#ifdef USE_DHT
  DHT dht(DHTPIN, DHTTYPE);
#endif


/* function */
#ifdef USE_PM25
String read_pm25(void){ //get pm2.5 data
  unsigned char pms5003[2];//store pms5003 data
  unsigned long read_timeout;
  int i;
  pms.begin(pms_baudrate);
  read_timeout = millis();

  while (millis() - read_timeout <= 10000) {
    pms.readBytes(pms5003, 2);
    if (pms5003[0] == 0x42 || pms5003[1] == 0x4d) { //尋找每段資料的開頭
      for (i = 0; i < 6; i++) {
        pms.readBytes(pms5003, 2);
      }
      pms.end();
      return (String)pms5003[1];
    }
  }
  Serial.println("pm25 no data");
  pms.end();
  return "__no_data__";
}
#endif

#ifdef USE_LCM1602
void lcd_print(String Str,int column,int row){
    lcd.setCursor(column,row);
    lcd.print(Str);
}
#endif

#ifdef USE_SSD1306
void init_ssd1306(void){
    display.begin(SSD1306_SWITCHCAPVCC);
    display.clearDisplay();
    delay(1000);
    display.setTextSize(1); //21 char in one line with Textsize == 1 ,10 char with size 2
    display.setTextColor(WHITE);
    display.display();
}
void OLED_print(String mes){
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(mes);
  display.display();
}
#endif


#ifdef USE_GPS
String get_GPS( String value){
  unsigned long timeout = millis();
  char c;
  String result;
  bool find_flag = 0;
  int i , j , next_comma, count;
  String temp ;
  int i_temp;
  int flag_lon = 0, flag_lat = 0;
  String show_on_OLED = "";

  String Time, Date, Date_Time, Lat/* latitude經度*/ , Lon/*longitude緯度*/;
  GPS.begin(GPS_baudrate);
  while (millis() - timeout < 5000 ) {
  c = GPS.read();
  if(c == '$'){
   find_flag = 1;
   result = "";
  }
  else if (c == '\n')
   find_flag = 0;



  if (find_flag  == 1)
  result += c;
  else if (find_flag == 0 && result.indexOf("GPRMC") != -1) {
  //                1         2 3          4 5           6 7     89      012
  //result = "$GPRMC,053015.00,A,2447.19539,N,12100.06437,E,0.016,,080618,,,A*74";
  Serial.println("result:" + result);

  // Time
  next_comma = result.indexOf(','); // first comma, usually after $GPRMC
  i = next_comma + 1;
  next_comma = result.indexOf(',', next_comma + 1); //2nd comma

  if (next_comma - i == 9) { //gps receive time
   for ( i ; i < next_comma - 3; i++) {
     if (Time.length() == 2 || Time.length() == 5)
       Time += ':';
     Time += result[i];
   }

   //fix the time to Taiwan's time zone
   temp = "";
   temp = (String)Time[0] + (String)Time[1];
   temp = (temp.toInt() + 8 >= 24 ? (temp.toInt() - 17) : (temp.toInt() + 8));
   Time = temp + Time.substring(2);
   temp = "";
  }

  // Lon
  next_comma = result.indexOf(',', next_comma + 1); //3th comma, After A
  i = next_comma + 1;
  next_comma = result.indexOf(',', next_comma + 1); //4th , after Lon

  if (next_comma - i ==  10) { //get current lon data
   flag_lon = 1;
   for (i; i < next_comma ; i++) {
     if (result[i] == '.') {}
     else if (Lon.length() == 2) {
       Lon += '.';
       Lon += result[i];
     }
     else
       Lon += result[i];
   }

   temp = "";
   for (j = 3; j < Lon.length(); j++)
     temp += Lon[j];
   temp = (String)(temp.toFloat() / 60.0);
   Lon = "24." + temp;
   i_temp = Lon.indexOf('.', Lon.indexOf('.') + 1);
   Lon[i_temp] = Lon[i_temp + 1];
   Lon[i_temp + 1] = Lon[i_temp + 2];
   LV_lon = Lon;
  }
  else { //not current lon data
   flag_lon = 0;
   Lon = LV_lon;
  }


  next_comma = result.indexOf(',', next_comma + 1); //5th, after N
  i = next_comma + 1;
  next_comma = result.indexOf(',', next_comma + 1); //6th, after Lat


  //Lat
  if (next_comma - i == 11) {
   flag_lat = 1;
   //take out lat from result
   for (i; i < next_comma ; i++) {
     if (result[i] == '.') {}
     else if (Lat.length() == 3) {
       Lat += '.';
       Lat += result[i];
     }
     else {
       Lat += result[i];
     }
   }

   // convert lat
   temp = "";
   for (j = 4; j < Lat.length(); j++)
     temp += Lat[j];
   temp = (String)(temp.toInt()/ 60.0);

   if(Lat[2] == '1')
     Lat = "121.";
   else if(Lat[2] == '0')
     Lat = "120.";

   if(temp.indexOf('.') < 6){
     for(j = 0; j < 5-temp.indexOf('.'); j++){
       Lat += '0';
     }
   }

   for (j = 0; j < temp.length(); j++)
     if (temp[j] != '.' )
       Lat += temp[j];


   LV_lat = Lat;
  }
  else {
   flag_lat = 0;
   Lat = LV_lat;
  }


  next_comma = result.indexOf(',', next_comma + 1); //7th
  next_comma = result.indexOf(',', next_comma + 1); //8th
  next_comma = result.indexOf(',', next_comma + 1); //9th
  i = next_comma + 1;
  next_comma = result.indexOf(',', next_comma + 1); //10th

  //Date
  if (next_comma - i == 6) {
   for (i; i < next_comma; i++)
     Date += result[i];
   Date_Time = "\"20" + Date.substring(4, 6) + "-" + Date.substring(2, 4) + "-" + Date.substring(0, 2) + " " + Time + "\"";
   LV_datetime = Date_Time;
  }
  else
   Date_Time = LV_datetime;


  break;
  }
  delay(5);
  }
  GPS.end();


  // show date time and lon lat on OLED monitor
  show_on_OLED = LV_datetime + "\n";
  if (flag_lon)
  show_on_OLED += Lon + ",";
  else
  show_on_OLED += "no lon,";

  if (flag_lat)
  show_on_OLED += Lat + "\n";
  else
  show_on_OLED += "no lat";
  OLED_print(show_on_OLED);

  return (Lon + ", " + Lat + ", \"user1\", " + value + "," + LV_datetime + "");
}
#endif
