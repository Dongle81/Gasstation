/*
  -----------------------------------------------------------
            Settings for ESP8266 based MCUs
  -----------------------------------------------------------
*/
// **** Hardware settings ****
#define ENCODER_PINA     D4
#define ENCODER_PINB     D3
#define ENCODER_BTN      D6
#define ENCODER_STEPS_PER_NOTCH    4   // Change this depending on which encoder is used

#define VNH2SP30_INA    D7
#define VNH2SP30_INB    D8
#define VNH2SP30_PWM    D5

// **** Fuel settings ****
#define FILLMODE                        MANUAL
#define FILL_PWM                        100
#define DRAIN_PWM                       90
#define SOFTSTART_TIME                  1500
#define DRAIN_TIME                      500
#define STOP_TIME                       250

// **** Model memory settings ****

#define MAX_MODELNAME_LENGHT        32               // max chars 
#define DEFAULT_NAME                "Model"          // default model name
#define MODEL_FILE                  "/models.json"   // file to store models
#define JSONDOC_SIZE                20000            // max file size in bytes



// **** PWM settings ****
#define UPDATE_INTERVAL_OLED_MENU     500      // ms
//#define UPDATE_INTERVAL_LOADCELL      100      // ms

#define MAXPWM  255
#define MINPWM  -255

// **** Display settings ****
U8G2_SSD1306_128X64_NONAME_1_HW_I2C oledDisplay(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ D2, /* data=*/ D1);

#define DISPLAY_WIDTH                 128
#define DISPLAY_HIGHT                 64

//OLED Defines
#define ROW1 18
#define ROW2 64

#define LEFTBORDER  0
#define RIGHTBORDER 128
#define CENTRE      0
#define BOTTOM      64
#define RIGHT       112


// **** Voltage measurement settings ****
// analog input pin
#define VOLTAGE_PIN                   A0

// supply voltage
#define V_REF                         3300     // set supply voltage from 1800 to 5500mV

// voltage divider
#define RESISTOR_R1                   10000    // ohm
#define RESISTOR_R2                   3300    // ohm

// calculate voltage to percent
#define ENABLE_PERCENTLIST            true

// Battery type
#define BAT_TYPE                      B_LIPO

// Battery cells
#define BAT_CELLS                     2



// **** Wifi settings ****

#define MAX_SSID_PW_LENGHT          32

// Station mode: connect to available network
#define SSID_STA                    "myWiFi"
#define PASSWORD_STA                ""
#define TIMEOUT_CONNECT             25000   //ms

// Access point mode: create own network
#define SSID_AP                     "Gas station"
#define PASSWORD_AP                 ""
const char ip[4] =                  {192,168,0,1};    // default IP address

#define ENABLE_MDNS                 true          // enable mDNS to reach the webpage with hostname.local
#define ENABLE_OTA                  true          // enable over the air update



// **** https update settings ****
#define ENABLE_UPDATE               false
#define HTTPS_PORT                  443
#define HOST                        "github.com"
#define URL                         "/nightflyer88/CG_scale/releases/latest"

             
