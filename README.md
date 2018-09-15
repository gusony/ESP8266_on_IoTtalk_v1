# ESP12F CONNECT pinout
|ESP12F |UART to USB|
|:-----:|:-----:|
|Vcc|3.3v|
|GND|GND|
|CH_PD|3.3V|
|TX|RX|
|RX|TX|
|GPIO|LOW(when upload)|

# Arduino IDE setting
Type|Parameter
:---:|:---:
Board|Generic ESP8266 Module
Flash Mode|QIO
Flash Frequency|40MHz
CPU Frequency|80MHz
Flash size|4M(1M SPIFFS)
Debug port|disabled
Debug Level|None
Reset Method|ck
Upload Speed|115200

# Arduino Library:
[pubsubclient v2.6.0](https://github.com/knolleary/pubsubclient)  ( [API](https://pubsubclient.knolleary.net/) )  
[UIPEthernet v2.0.6](https://github.com/UIPEthernet/UIPEthernet)  
[ArduinoJson v5.13.1](https://arduinojson.org/?utm_source=meta&utm_medium=library.properties)  

# Set Library Parameter
```
[UIPEthernet/utility/uipethernet-conf.h] 
	#define UIP_SOCKET_NUMPACKETS    4
	#define UIP_CONF_MAX_CONNECTIONS 3 
	#define UIP_CONF_UDP_CONNS 1 //reduce memory usage 

[pubsubclient/src/PubSubClient.h]
	#define MQTT_MAX_PACKET_SIZE 512
```
  
# Feature Work
1.用bs170替換
