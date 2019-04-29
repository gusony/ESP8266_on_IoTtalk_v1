
/* main config */
#define V1
//#define V2

#define USE_WIFI
//#define USE_SSL
//#define USE_ETHERNET

#define DF_LIST {"ESP12F_IDF", "ESP12F_ODF","ESP12F_testlatency"} //Device Feature list
#define DF_NUM 3                                                  // must be eaual to the element numbers of DF_LIST 
#define IDF_LIST {"ESP12F_IDF"}
#define ODF_LIST {"ESP12F_ODF","ESP12F_testlatency"}
#define DM_NAME  "ESP12F"                                         // Device Module name


//#define FORCE_CONNECT                                           //if you don't want into ap_setting(),enable it
#define HTTP_RESPONSE_PAYLOAD_SIZE 512                            // the size of payload of 'httpresp' struct 



/* set server ip */
#ifdef V1
  #ifdef USE_SSL
    #define DEFAULT_SERVER_IP "test.iottalk.tw"
  #else
    #define DEFAULT_SERVER_IP "140.113.215.2"
  #endif
#elif defined V2
  #define DEFAULT_SERVER_IP "140.113.199.198"
#endif

/* set server port*/
#ifdef USE_SSL
  #define ServerPort 443
#else
  #define ServerPort 9992
#endif


/*  which part you wnat to debug ,just uncomment */
#define debug_mode
#ifdef debug_mode
//  #define debug_prepare_http_package
//  #define debug_SEND
//  #define debug_GET
//  #define debug_mode_PUT
//  #define debug_POST
//  #define debug_getprofile
//  #define debug_register
//  #define debug_checknetstatus
//  #define debug_getProfile
//  #define debug_pull
//  #define debug_push
//  #define debug_ETH_TCP
#endif

// for test v1
#define TEST_DATA_NUM 1000
#define TEST_DATA_INTERVAL 200


/*  If use Ethernet, i suggest you using the static ip to saving memory usage. 
 *  If you want to use DHCP , you only need mac address. 
 */
#ifdef USE_ETHERNET
  #define MYIPADDR 192,168,1,6
  #define MYIPMASK 255,255,255,0
  #define MYDNS 192,168,1,1
  #define MYGW 192,168,1,1
  #define MACADDRESS 0x00,0x01,0x02,0x03,0x04,0x05
#endif


/* Pin usage */
/* record which pin has been used */
#define UPLOAD       0  // when you want to upload code to esp8266, this pin must be LOW, will not be used on Nodemcu
#define LEDPIN       2  // on board led
#define CLEAREEPROM  4 //  hold for 5 second , it will erease the contain in eeprom
#define TX           "TXD"
#define RX           "RXD"
#ifdef USE_ETHERNET
  #define ETHERNET_SO  12
  #define ETHERNET_SI  13
  #define ETHERNET_SCK 14
  #define ETHERNET_CS  15
#endif


//     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//                       _oo0oo_
//                      o8888888o
//                      88" . "88
//                      (| -_- |)
//                      0\  =  /0
//                    ___/`---'\___
//                  .' \\|     |// '.
//                 / \\|||  :  |||// \
//                / _||||| -:- |||||- \
//               |   | \\\  -  /// |   |
//               | \_|  ''\---/''  |_/ |
//               \  .-\__  '-'  ___/-. /
//             ___'. .'  /--.--\  `. .'___
//          ."" '<  `.___\_<|>_/___.' >' "".
//         | | :  `- \`.;`\ _ /`;.`/ - ` : | |
//         \  \ `_.   \_ __\ /__ _/   .-` /  /
//     =====`-.____`.___ \_____/___.-`___.-'=====
//                       `=---='
//     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//               佛祖保佑         永无BUG

