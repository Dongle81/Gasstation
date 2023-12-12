/*
  -----------------------------------------------------------
            Defaults
  -----------------------------------------------------------
  It is recommended not to change these default values and parameters.
*/

// resistors
enum {
  R1,
  R2
};


// console msg type
enum {
  T_BOOT,
  T_RUN,
  T_ERROR,
  T_WIFI,
  T_UPDATE,
  T_HTTPS,
  T_PARAM
};


// https update
enum {
  PROBE_UPDATE,
  UPDATE_FIRMWARE,
  UPDATE_SPIFFS
};

enum {
 AUTO,
 MANUAL,
 PARAMETER 
};

enum {
  PRE_STOP,
  STOP,
  FADING_PUMP_CALC,
  FADING_PUMP,
  PUMP,
  WAIT,
  DRAIN
};


// EEprom parameter addresses
enum {
  P_FILL_MODE =                         1,
  P_FILL_PWM =                          2, 
  P_DRAIN_PWM =                         P_FILL_PWM + sizeof(float),
  P_SOFTSTART_TIME =                    P_DRAIN_PWM + sizeof(float),
  P_DRAIN_TIME =                        P_SOFTSTART_TIME + sizeof(float),
  P_STOP_TIME =                         P_DRAIN_TIME + sizeof(float), 
  P_BAT_TYPE =                          P_STOP_TIME + sizeof(float),
  P_BATT_CELLS =                        P_BAT_TYPE + 1,
  P_RESISTOR_R1 =                       P_BATT_CELLS + 3,
  P_RESISTOR_R2 =                       P_RESISTOR_R1 + sizeof(float),
  
  P_SSID_STA =                          P_RESISTOR_R2 + sizeof(float),
  P_PASSWORD_STA =                      P_SSID_STA + MAX_SSID_PW_LENGHT + 1,
  P_SSID_AP =                           P_PASSWORD_STA + MAX_SSID_PW_LENGHT + 1,
  P_PASSWORD_AP =                       P_SSID_AP + MAX_SSID_PW_LENGHT + 1,
  
  P_ENABLE_OTA =                         P_PASSWORD_AP + MAX_SSID_PW_LENGHT + 1,  
                              
  P_DEVICENAME =                        P_ENABLE_OTA + 1,
  EEPROM_SIZE =                         P_DEVICENAME + MAX_SSID_PW_LENGHT + 1
};



// battery image 12x6
const unsigned char batteryImage[] U8X8_PROGMEM = {
  0xff, 0x03, 0x01, 0x0e, 0x01, 0x0e, 0x01, 0x0e, 0x01, 0x0e, 0xff, 0x03
};

// Gas image 18x18
const unsigned char Gasimage[] U8X8_PROGMEM = {
  0xF8, 0x07, 0x00, 0x04, 0x08, 0x00, 0xF4, 0x4B, 0x00, 0x0C, 0x8C, 0x00, 
  0x0C, 0xCC, 0x00, 0x0C, 0xBC, 0x00, 0xF4, 0xBB, 0x00, 0x04, 0xF8, 0x00, 
  0x04, 0x48, 0x00, 0x04, 0x88, 0x00, 0x04, 0x18, 0x01, 0x04, 0x38, 0x02, 
  0x04, 0x38, 0x02, 0x04, 0x38, 0x02, 0x04, 0x78, 0x03, 0x04, 0x98, 0x00, 
  0x04, 0x18, 0x00, 0xFF, 0x3F, 0x00, };


// battery types
#if ENABLE_PERCENTLIST
#define NUMBER_BAT_TYPES     6
#else
#define NUMBER_BAT_TYPES     2
#endif

enum {
  B_OFF,
  B_VOLT,
  B_LIPO,
  B_LIION,
  B_LIFEPO,
  B_NIXX
};

const String battTypName[NUMBER_BAT_TYPES] = {
#if ENABLE_PERCENTLIST
  "OFF",
  "Voltage",
  "LiPo",
  "Li-ion",
  "LiFePo",
  "Nixx"
#else
  "OFF",
  "Voltage"
#endif
};

// battery percent lists
#define DATAPOINTS_PERCENTLIST    21

const PROGMEM float percentList[][DATAPOINTS_PERCENTLIST][2] =
{
  {
    {3.000, 0},   // LiPo
    {3.250, 5},
    {3.500, 10},
    {3.675, 15},
    {3.696, 20},
    {3.718, 25},
    {3.737, 30},
    {3.753, 35},
    {3.772, 40},
    {3.789, 45},
    {3.807, 50},
    {3.827, 55},
    {3.850, 60},
    {3.881, 65},
    {3.916, 70},
    {3.948, 75},
    {3.987, 80},
    {4.042, 85},
    {4.085, 90},
    {4.115, 95},
    {4.150, 100}
  },
  {
    {3.250, 0},   // Li-ion
    {3.300, 5},
    {3.327, 10},
    {3.355, 15},
    {3.377, 20},
    {3.395, 25},
    {3.435, 30},
    {3.490, 40},
    {3.630, 60},
    {3.755, 75},
    {3.790, 80},
    {3.840, 85},
    {3.870, 90},
    {3.915, 95},
    {4.050, 100}
  },
  {
    {2.80, 0},    // LiFePo
    {3.06, 5},
    {3.14, 10},
    {3.17, 15},
    {3.19, 20},
    {3.20, 25},
    {3.21, 30},
    {3.22, 35},
    {3.23, 40},
    {3.24, 45},
    {3.25, 50},
    {3.25, 55},
    {3.26, 60},
    {3.26, 65},
    {3.27, 70},
    {3.28, 75},
    {3.28, 80},
    {3.29, 85},
    {3.29, 90},
    {3.29, 95},
    {3.30, 100}
  },
  {
    {0.900, 0},   // Nixx
    {0.970, 5},
    {1.040, 10},
    {1.090, 15},
    {1.120, 20},
    {1.140, 25},
    {1.155, 30},
    {1.175, 40},
    {1.205, 60},
    {1.220, 75},
    {1.230, 80},
    {1.250, 85},
    {1.280, 90},
    {1.330, 95},
    {1.420, 100}
  }
};
