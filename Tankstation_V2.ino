/*
  ------------------------------------------------------------------
                            Gas station
                      (c) 2019-2023 by T. Ott
  ------------------------------------------------------------------
*/
#define GASSTATION_VERSION "1.0"
/*

  ******************************************************************
  history:
  V1.0    03.12.23      first release


  ******************************************************************

  Software License Agreement (BSD License)

  Copyright (c) 2019-2023, Tobias Ott
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  1. Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
  3. Neither the name of the copyright holders nor the
  names of its contributors may be used to endorse or promote products
  derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

// Required libraries, can be installed from the library manager
#include <U8g2lib.h>        // Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)

// built-in libraries
#include <EEPROM.h>+++
//#include <Wire.h>

// libraries for ESP8266
#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ElegantOTA.h>
#include "settings_ESP8266.h"
#include <ClickEncoder.h>
#include <Vnh2sp30_tott.h>

// webserver constructor
ESP8266WebServer server(80);
IPAddress apIP(ip[0], ip[1], ip[2], ip[3]);
WiFiClientSecure httpsClient;
File fsUploadFile;              // a File object to temporarily store the received file

ClickEncoder encoder = ClickEncoder(ENCODER_PINA, ENCODER_PINB, ENCODER_BTN, ENCODER_STEPS_PER_NOTCH);
//              ENA A   B   PWM   CS    inv
Vnh2sp30  pump(  VNH2SP30_INA, VNH2SP30_INB,  VNH2SP30_PWM,   1);

#include "defaults.h"

struct Param {
  uint8_t fillmode = FILLMODE;
  float fillPWM = FILL_PWM;
  float drainPWM = DRAIN_PWM;
  float softstart_time = SOFTSTART_TIME;
  float drain_time = DRAIN_TIME;
  float stop_time = STOP_TIME;
};

Param param;



// load default values
float resistor[] = {RESISTOR_R1, RESISTOR_R2};
uint8_t batType = BAT_TYPE;
uint8_t batCells = BAT_CELLS;
//Website
char device_Name[MAX_SSID_PW_LENGHT + 1] = SSID_AP;
char ssid_STA[MAX_SSID_PW_LENGHT + 1] = SSID_STA;
char password_STA[MAX_SSID_PW_LENGHT + 1] = PASSWORD_STA;
char ssid_AP[MAX_SSID_PW_LENGHT + 1] = SSID_AP;
char password_AP[MAX_SSID_PW_LENGHT + 1] = PASSWORD_AP;
//bool enableUpdate = ENABLE_UPDATE;
bool enableOTA = ENABLE_OTA;
bool enableUpdate = false;
int16_t ClientPWM = 0;

int16_t wait = 0;
int16_t last_enc, value, rawvalue, rawpara, lastpara, percent, fading_step, fading_value;
uint8_t modecnt;
bool ev_buttonclicked, ev_modechanged, ev_buttonheld;
bool clientconnected = false;
bool clientconnected_old = false;
bool ev_clientClicked = 0;



// declare variables
uint8_t state;
uint8_t mode, clientMode;

float batVolt = 0;
unsigned long lastTimeMenu = 0;
const uint8_t *oledFontBig;
const uint8_t *oledFontLarge;
const uint8_t *oledFontNormal;
const uint8_t *oledFontSmall;

char VarBuf[4];
char OLEDbuffer1[16]; // Zeile 1
char OLEDbuffer2[16]; // Zeile 2

//Website
String updateMsg = "";
bool wifiSTAmode = true;
float gitVersion = -1;





// Restart CPU
void resetCPU() {}


// convert time to string
char * TimeToString(unsigned long t)
{
  static char str[13];
  int h = t / 3600000;
  t = t % 3600000;
  int m = t / 60000;
  t = t % 60000;
  int s = t / 1000;
  int ms = t - (s * 1000);
  sprintf(str, "%02ld:%02d:%02d.%03d", h, m, s, ms);
  return str;
}


// Count percentage from cell voltage
int percentBat(float cellVoltage) {

  int result = 0;
  int elementCount = DATAPOINTS_PERCENTLIST;
  byte batTypeArray = batType - 2;

  for (int i = 0; i < elementCount; i++) {
    if (pgm_read_float( &percentList[batTypeArray][i][1]) == 100 ) {
      elementCount = i;
      break;
    }
  }

  float cellempty = pgm_read_float( &percentList[batTypeArray][0][0]);
  float cellfull = pgm_read_float( &percentList[batTypeArray][elementCount][0]);

  if (cellVoltage >= cellfull) {
    result = 100;
  } else if (cellVoltage <= cellempty) {
    result = 0;
  } else {
    for (int i = 0; i <= elementCount; i++) {
      float curVolt = pgm_read_float(&percentList[batTypeArray][i][0]);
      if (curVolt >= cellVoltage && i > 0) {
        float lastVolt = pgm_read_float(&percentList[batTypeArray][i - 1][0]);
        float curPercent = pgm_read_float(&percentList[batTypeArray][i][1]);
        float lastPercent = pgm_read_float(&percentList[batTypeArray][i - 1][1]);
        result = float((cellVoltage - lastVolt) / (curVolt - lastVolt)) * (curPercent - lastPercent) + lastPercent;
        break;
      }
    }
  }

  return result;
}


void printConsole(int t, String msg) {
  Serial.print(TimeToString(millis()));
  Serial.print(" [");
  switch (t) {
    case T_BOOT:
      Serial.print("BOOT");
      break;
    case T_RUN:
      Serial.print("RUN");
      break;
    case T_ERROR:
      Serial.print("ERROR");
      break;
    case T_WIFI:
      Serial.print("WIFI");
      break;
    case T_UPDATE:
      Serial.print("UPDATE");
      break;
    case T_HTTPS:
      Serial.print("HTTPS");
      break;
    case T_PARAM:
      Serial.print("PARAMETER");
      break;
  }
  Serial.print("] ");
  Serial.println(msg);
}


void initOLED() {
  oledDisplay.begin();
  printConsole(T_BOOT, "init OLED display: " + String(DISPLAY_WIDTH) + String("x") + String(DISPLAY_HIGHT));

  oledFontBig    = u8g2_font_helvR18_tn;
  oledFontLarge  = u8g2_font_helvR12_tr;
  oledFontNormal = u8g2_font_helvR10_tr;
  oledFontSmall  = u8g2_font_5x7_tr;

  int ylineHeight = DISPLAY_HIGHT / 3;

  oledDisplay.firstPage();
  do {
    oledDisplay.setFont(oledFontLarge);
    oledDisplay.drawXBMP(20, 12, 18, 18, Gasimage);
    oledDisplay.setFont(oledFontLarge);
    oledDisplay.setCursor(45, 28);
    oledDisplay.print(F("Gas station"));
    oledDisplay.setFont(oledFontSmall);
    oledDisplay.setCursor(35, 55);
    oledDisplay.print(F("Version: "));
    oledDisplay.print(GASSTATION_VERSION);
    oledDisplay.setCursor(20, 64);
    oledDisplay.print(F("(c) 2023 T.Ott"));

  } while ( oledDisplay.nextPage() );
}


void printOLED(String aLine1, String aLine2, String aLine3) {
  int ylineHeight = DISPLAY_HIGHT / 3;

  oledDisplay.firstPage();
  do {
    oledDisplay.setFont(oledFontNormal);
    oledDisplay.setCursor(0, ylineHeight * 1);
    oledDisplay.print(aLine1);
    oledDisplay.setCursor(0, ylineHeight * 2);
    oledDisplay.print(aLine2);
    oledDisplay.setCursor(0, DISPLAY_HIGHT);
    oledDisplay.print(aLine3);
  } while ( oledDisplay.nextPage() );
}

void printPWMOLED() {
  // print to display
  char buff[8];

  float pos_weightTotal = 17;
  float pos_CG_length = 45;

  oledDisplay.firstPage();
  do {
    // print battery
    if (batType > B_OFF) {
      oledDisplay.drawXBMP(88, 1, 12, 6, batteryImage);
      if (batType == B_VOLT) {
        dtostrf(batVolt, 2, 2, buff);
      } else {
        dtostrf(batVolt, 3, 0, buff);
        oledDisplay.drawBox(89, 2, (batVolt / (100 / 8)), 4);
      }
      oledDisplay.setFont(oledFontSmall);
      oledDisplay.setCursor(123 - oledDisplay.getStrWidth(buff), 7);
      oledDisplay.print(buff);
      if (batType == B_VOLT) {
        oledDisplay.print(F("V"));
      } else {
        oledDisplay.print(F("%"));
      }
    }
    oledDisplay.setFont(oledFontNormal);
    oledDisplay.setFontPosTop();
    oledDisplay.drawStr(LEFTBORDER, ROW1, OLEDbuffer1);
    oledDisplay.setFont(oledFontLarge);
    oledDisplay.setFontPosCenter();
    oledDisplay.setFontPosBottom();
    oledDisplay.drawStr(CENTRE, BOTTOM, OLEDbuffer2);
  } while ( oledDisplay.nextPage() );
}

void automatic_sequence(void) {
  int16_t neg_value;

  if (ev_buttonclicked) {
    if (state == FADING_PUMP || state == FADING_PUMP || state == PUMP ) {
      state = PRE_STOP;
    }
    else if (state == DRAIN) {
      state = STOP;
    }
    else {
      state = FADING_PUMP_CALC;
    }
    ev_buttonclicked = false;
  }

   if (ev_clientClicked) {
    if (state == FADING_PUMP || state == FADING_PUMP || state == PUMP ) {
      state = PRE_STOP;
    }
    else if (state == DRAIN) {
      state = STOP;
    }
    else {
      state = FADING_PUMP_CALC;
    }
    ev_clientClicked = false;
  } 

  switch (state) {
    case PRE_STOP:
      fading_value = 0;
      pump.coast();
      state = DRAIN;
      wait = param.stop_time;
      printConsole(T_RUN, "Automatic PRE_STOP");
      break;

    case DRAIN:
      if (wait <= 0) {
        if (value >= 0) {
          neg_value = param.drainPWM * -1;
        }
        else {
          neg_value = param.drainPWM;
        }
        pump.run(neg_value);
        wait = param.drain_time;
        state = STOP;
        printConsole(T_RUN, "Automatic DRAIN");
      }
      break;

    case STOP:
      fading_value = 0;
      if (wait <= 0) {
        pump.coast();
        state = WAIT;
        printConsole(T_RUN, "Automatic STOP");
      }
      break;

    case FADING_PUMP_CALC:
      printConsole(T_RUN, "Automatic FADING_PUMP_CALC");
      fading_step = param.softstart_time / value;
      state = FADING_PUMP;
      if (clientconnected == 0){
        sprintf(OLEDbuffer1, "Pump active");
      }
      break;

    case FADING_PUMP:
      printConsole(T_RUN, "Automatic FADING_PUMP");
      if (wait <= 0) {
        pump.run(fading_value++);
        wait = fading_step;
        if (fading_value >= value) {
          state = PUMP;
        }
      }
      break;

    case PUMP:
      fading_value = 0;
      pump.run(value);
      break;

    case WAIT:
      if (clientconnected == 0){
        sprintf(OLEDbuffer1, "Automatic Mode");
      }
      clientMode=NOTHING;
      break;

    default:
      state = STOP;
  }
}

void manual_sequence(void) {
  if (ev_buttonclicked) {
    if (state == PUMP) {
      state = STOP;
    }
    else {
      state = PUMP;
    }
    ev_buttonclicked = false;
  }

  if (ev_clientClicked) {
    if (state == PUMP) {
      state = STOP;
    }
    else {
      state = PUMP;
    }
    ev_clientClicked = false;
  }  

  switch (state) {
    case STOP:
      pump.coast();
      state = WAIT;
      printConsole(T_RUN, "MANUAL STOP");
      break;

    case PUMP:
      pump.run(value);
      if (clientconnected == 0){
      sprintf(OLEDbuffer1, "Pump active");
      }
      break;

    case WAIT:
      // nothing to do here
      if (clientconnected == 0){
      sprintf(OLEDbuffer1, "Manual Mode");
      }
      clientMode=NOTHING;
      break;

    default:
      state = STOP;
  }

}

void process_encoder_button(void) {
  // encoder
  if (clientconnected == 0){
  if (mode != PARAMETER && state != FADING_PUMP) {
    rawvalue += encoder.getValue();
    if (rawvalue != last_enc) {
      if (rawvalue >= MAXPWM) rawvalue = MAXPWM;
      else if (rawvalue <= MINPWM) rawvalue = MINPWM;
      last_enc = rawvalue;
      value = rawvalue;
    }
    percent = map(value, MINPWM, MAXPWM, -100, 100);
    dtostrf(percent, 4, 0, VarBuf);
    sprintf(OLEDbuffer2, "PWM %s %%", VarBuf);
  }
  }else{
    if (state != FADING_PUMP) {
      value = map(ClientPWM, -100, 100, MINPWM, MAXPWM);
    }
  }
  // button
  if (clientconnected == 0){
  ClickEncoder::Button b = encoder.getButton();
  if (b != ClickEncoder::Open) {
    switch (b) {
      case ClickEncoder::Pressed:
        printConsole(T_RUN, "ClickEncoder::Pressed");
        break;
      case ClickEncoder::Held:
        printConsole(T_RUN, "ClickEncoder::Held");
        break;
      case ClickEncoder::Released:
        printConsole(T_RUN, "ClickEncoder::Released");
        modecnt++;
        break;
      case ClickEncoder::Clicked:
        printConsole(T_RUN, "ClickEncoder::Clicked");
        if (ev_buttonclicked == true) {
          ev_buttonclicked = false;
        }
        else {
          ev_buttonclicked = true;
        }
        break;
      case ClickEncoder::DoubleClicked:
        printConsole(T_RUN, "ClickEncoder::DoubleClicked");
        if (ev_modechanged == true) {
          ev_modechanged = false;
        }
        else {
          ev_modechanged = true;
        }
        if (mode == AUTO && state == WAIT) {
          mode = MANUAL;
          printConsole(T_RUN, "Set MANUAL Mode");
        }
        else if (mode == MANUAL && state == WAIT) {
          mode = AUTO;
          printConsole(T_RUN, "Set AUTOMATIC Mode");
        }
        break;
    }
  }
} else{
  sprintf(OLEDbuffer1, "Client");
  sprintf(OLEDbuffer2, "connected");
  mode = PARAMETER;
}
}

// send headvalues to client
void getHead() {
  String response = ssid_AP;
  response += "&";
  response += GASSTATION_VERSION;
  response += "&";
  response += gitVersion;
  server.send(200, "text/html", response);
}


// get values from client
void setPWM() {
  if (server.hasArg("pwm")) ClientPWM = server.arg("pwm").toInt();

  server.send(200, "text/plain", "saved");
  printConsole(T_RUN, "PWM set via Client");
}

void manualFill() {
  printConsole(T_RUN, "manualFill");
  server.send(200, "text/html", "saved");
  clientMode=MANUAL;
  ev_clientClicked = true;
}

void autoFill() {
  printConsole(T_RUN, "autoFill");
  server.send(200, "text/html", "saved");
  clientMode=AUTO;
  ev_clientClicked = true;
}

// send values to client
void getValue() {
  char buff[8];
  String response = "";
  if (batType == B_VOLT) {
    dtostrf(batVolt, 5, 2, buff);
    response += buff;
    response += "V";
  } else {
    dtostrf(batVolt, 5, 0, buff);
    response += buff;
    response += "%";
  }
  server.send(200, "text/html", response);
}


// send parameters to client
void getParameter() {
  char buff[8];
  String response = "";

  // parameter list
  response += param.fillmode;               //0
  response += "&";
  response += param.fillPWM;                //1
  response += "&";
  response += param.drainPWM;               //2
  response += "&";
  response += param.softstart_time;         //3
  response += "&";
  response += param.drain_time;             //4
  response += "&";
  response += param.stop_time;             //5
  response += "&";
  for (int i = R1; i <= R2; i++) {
    response += resistor[i];            //6-7
    response += "&";
  }
  response += batType;                  //8
  response += "&";
  response += batCells;                 //9
  response += "&";
  response += ssid_STA;                 //10
  response += "&";
  response += password_STA;             //11
  response += "&";
  response += ssid_AP;                  //12
  response += "&";
  response += password_AP;              //13
  response += "&";
  response += enableOTA;                //14
  response += "&";
  response += device_Name;              //15
  server.send(200, "text/html", response);
}



// send available WiFi networks to client
void getWiFiNetworks() {
  bool ssidSTAavailable = false;
  String response = "";
  int n = WiFi.scanNetworks();

  if (n > 0) {
    for (int i = 0; i < n; ++i) {
      response += WiFi.SSID(i);
      if (WiFi.SSID(i) == ssid_STA) ssidSTAavailable = true;
      if (i < n - 1) response += "&";
    }
    if (!ssidSTAavailable) {
      response += "&";
      response += ssid_STA;
    }
  }
  server.send(200, "text/html", response);
}


// save parameters
void saveParameter() {
  if (server.hasArg("nFillMode")) param.fillmode = server.arg("nFillMode").toInt();
  if (server.hasArg("fillPWM")) param.fillPWM = server.arg("fillPWM").toFloat();
  if (server.hasArg("drainPWM")) param.drainPWM = server.arg("drainPWM").toFloat();
  if (server.hasArg("softstart_time")) param.softstart_time = server.arg("softstart_time").toFloat();
  if (server.hasArg("drain_time")) param.drain_time = server.arg("drain_time").toFloat();
  if (server.hasArg("stop_time")) param.stop_time = server.arg("stop_time").toFloat(); 
  if (server.hasArg("resistorR1")) resistor[R1] = server.arg("resistorR1").toFloat();
  if (server.hasArg("resistorR2")) resistor[R2] = server.arg("resistorR2").toFloat();
  if (server.hasArg("batType")) batType = server.arg("batType").toInt();
  if (server.hasArg("batCells")) batCells = server.arg("batCells").toInt();
  if (server.hasArg("ssid_STA")) server.arg("ssid_STA").toCharArray(ssid_STA, MAX_SSID_PW_LENGHT + 1);
  if (server.hasArg("password_STA")) server.arg("password_STA").toCharArray(password_STA, MAX_SSID_PW_LENGHT + 1);
  if (server.hasArg("ssid_AP")) server.arg("ssid_AP").toCharArray(ssid_AP, MAX_SSID_PW_LENGHT + 1);
  if (server.hasArg("password_AP")) server.arg("password_AP").toCharArray(password_AP, MAX_SSID_PW_LENGHT + 1);
  if (server.hasArg("enableOTA")) enableOTA = server.arg("enableOTA").toInt();
  if (server.hasArg("device_Name")) server.arg("device_Name").toCharArray(device_Name, MAX_SSID_PW_LENGHT + 1);


  EEPROM.put(P_FILL_MODE, param.fillmode);
  EEPROM.put(P_FILL_PWM, param.fillPWM);
  EEPROM.put(P_DRAIN_PWM, param.drainPWM);
  EEPROM.put(P_SOFTSTART_TIME, param.softstart_time);
  EEPROM.put(P_DRAIN_TIME, param.drain_time);
  EEPROM.put(P_STOP_TIME, param.drain_time);
  EEPROM.put(P_BAT_TYPE, batType);
  EEPROM.put(P_BATT_CELLS, batCells);
  for (int i = R1; i <= R2; i++) {
    EEPROM.put(P_RESISTOR_R1 + (i * sizeof(float)), resistor[i]);
  }
  EEPROM.put(P_SSID_STA, ssid_STA);
  EEPROM.put(P_PASSWORD_STA, password_STA);
  EEPROM.put(P_SSID_AP, ssid_AP);
  EEPROM.put(P_PASSWORD_AP, password_AP);
  EEPROM.put(P_ENABLE_OTA, enableOTA);
  EEPROM.put(P_DEVICENAME, device_Name);
  EEPROM.commit();

  server.send(200, "text/plain", "saved");
  printConsole(T_WIFI, "save parameters");
}



// convert the file extension to the MIME type
String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".png")) return "text/css";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".map")) return "application/json";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}


// send file to the client (if it exists)
bool handleFileRead(String path) {
  // If a folder is requested, send the index file
  if (path.endsWith("/")) path += "index.html";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";

  // If the file exists, either as a compressed archive, or normal
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if (SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }

  return false;
}


// upload a new file to the SPIFFS
void handleFileUpload() {

  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    if (filename != MODEL_FILE ) server.send(500, "text/plain", "wrong file !");
    // Open the file for writing in SPIFFS (create if it doesn't exist)
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    // Write the received bytes to the file
    fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    // If the file was successfully created
    if (fsUploadFile) {
      fsUploadFile.close();
      // Redirect the client to the success page
      server.sendHeader("Location", "/settings.html");
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }

}


// print update progress screen
void printUpdateProgress(unsigned int progress, unsigned int total) {
  printConsole(T_UPDATE, updateMsg);

  oledDisplay.firstPage();
  do {
    oledDisplay.setFont(oledFontSmall);
    oledDisplay.setCursor(0, 12);
    oledDisplay.print(updateMsg);

    oledDisplay.setCursor(107, 35);
    oledDisplay.printf("%u%%\r", (progress / (total / 100)));

    oledDisplay.drawFrame(0, 40, 128, 10);
    oledDisplay.drawBox(0, 40, (progress / (total / 128)), 10);

  } while ( oledDisplay.nextPage() );
}


// https update
bool httpsUpdate(uint8_t command) {
  if (!httpsClient.connect(HOST, HTTPS_PORT)) {
    printConsole(T_ERROR, "Wifi: connection to GIT failed");
    return false;
  }

  const char * headerKeys[] = {"Location"} ;
  const size_t numberOfHeaders = 1;

  HTTPClient https;
  https.setUserAgent("cgscale");
  https.setRedirectLimit(0);
  https.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

  String url = "https://" + String(HOST) + String(URL);
  if (https.begin(httpsClient, url)) {
    https.collectHeaders(headerKeys, numberOfHeaders);

    printConsole(T_HTTPS, "GET: " + url);
    int httpCode = https.GET();
    if (httpCode > 0) {
      // response
      if (httpCode == HTTP_CODE_FOUND) {
        String newUrl = https.header("Location");
        gitVersion = newUrl.substring(newUrl.lastIndexOf('/') + 2).toFloat();
        if (gitVersion > String(GASSTATION_VERSION).toFloat()) {
          printConsole(T_UPDATE, "Firmware update available: V" + String(gitVersion));
        } else {
          printConsole(T_UPDATE, "Firmware version found on GitHub: V" + String(gitVersion) + " - current firmware is up to date");
        }
      } else if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        //Serial.println(https.getString());
      } else {
        printConsole(T_ERROR, "HTTPS: GET... failed, " + https.errorToString(httpCode));
        https.end();
        return false;
      }
    } else {
      return false;
    }
    https.end();
  } else {
    printConsole(T_ERROR, "Wifi: Unable to connect");
    return false;
  }

  return true;
}


void waitWiFiconnected() {
  long timeoutWiFi = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (WiFi.status() == WL_NO_SSID_AVAIL) {
      printConsole(T_ERROR, "\nWifi: No SSID available");
      break;
    } else if (WiFi.status() == WL_CONNECT_FAILED) {
      printConsole(T_ERROR, "\nWifi: Connection failed");
      break;
    } else if ((millis() - timeoutWiFi) > TIMEOUT_CONNECT) {
      printConsole(T_ERROR, "\nWifi: Timeout");
      break;
    }
  }
}


void checkClientConnected() {
  if (WiFi.softAPgetStationNum() > 0) {
    clientconnected = true;
  } else {
    clientconnected = false;
    clientMode=NOTHING;
  }
  if (clientconnected) {
    if (clientconnected != clientconnected_old) {
      clientconnected_old = clientconnected;
      printConsole(T_RUN, "Client connected");
    }
  } else {
    if (clientconnected != clientconnected_old) {
      clientconnected_old = clientconnected;
      mode = param.fillmode;
      printConsole(T_RUN, "Client disconnected");
    }
  }
}

void setup() {
  last_enc = -1;
  rawvalue = MAXPWM;
  state = STOP;
  ev_buttonclicked = false;
  ev_modechanged = false;
  ev_clientClicked = false;
  clientMode = NOTHING;

  // init serial
  Serial.begin(115200);
  Serial.println();
  delay(1000);

  printConsole(T_BOOT, "startup Gas station V" + String(GASSTATION_VERSION));

  // init filesystem
  SPIFFS.begin();
  EEPROM.begin(EEPROM_SIZE);

  printConsole(T_BOOT, "init filesystem");


  // read settings from eeprom
  if (EEPROM.read(P_FILL_MODE) != 0xFF) {
    EEPROM.get(P_FILL_MODE, param.fillmode);
    mode = param.fillmode;
  }

  if (EEPROM.read(P_FILL_PWM) != 0xFF) {
    EEPROM.get(P_FILL_PWM, param.fillPWM);
    value = param.fillPWM;
  }

  if (EEPROM.read(P_DRAIN_PWM) != 0xFF) {
    EEPROM.get(P_DRAIN_PWM, param.drainPWM);
  }
  
  if (EEPROM.read(P_SOFTSTART_TIME) != 0xFF) {
    EEPROM.get(P_SOFTSTART_TIME, param.softstart_time);
  }

  if (EEPROM.read(P_DRAIN_TIME) != 0xFF) {
    EEPROM.get(P_DRAIN_TIME, param.drain_time);
  }

  if (EEPROM.read(P_STOP_TIME) != 0xFF) {
    EEPROM.get(P_STOP_TIME, param.stop_time);
  }

  if (EEPROM.read(P_BAT_TYPE) != 0xFF) {
    batType = EEPROM.read(P_BAT_TYPE);
  }

  if (EEPROM.read(P_BATT_CELLS) != 0xFF) {
    batCells = EEPROM.read(P_BATT_CELLS);
  }

  for (int i = R1; i <= R2; i++) {
    if (EEPROM.read(P_RESISTOR_R1 + (i * sizeof(float))) != 0xFF) {
      EEPROM.get(P_RESISTOR_R1 + (i * sizeof(float)), resistor[i]);
    }
  }

  if (EEPROM.read(P_SSID_STA) != 0xFF) {
    EEPROM.get(P_SSID_STA, ssid_STA);
  }

  if (EEPROM.read(P_PASSWORD_STA) != 0xFF) {
    EEPROM.get(P_PASSWORD_STA, password_STA);
  }

  if (EEPROM.read(P_SSID_AP) != 0xFF) {
    EEPROM.get(P_SSID_AP, ssid_AP);
  }

  if (EEPROM.read(P_PASSWORD_AP) != 0xFF) {
    EEPROM.get(P_PASSWORD_AP, password_AP);
  }

  if (EEPROM.read(P_DEVICENAME) != 0xFF) {
    EEPROM.get(P_DEVICENAME, device_Name);
  } else {
    strcpy(device_Name, ssid_AP);
  }

  // init OLED display
  initOLED();

  printConsole(T_BOOT, "Wifi: STA mode - connecing with: " + String(ssid_STA));

  encoder.setButtonHeldEnabled(true);
  encoder.setDoubleClickEnabled(true);
  printConsole(T_BOOT, "encoder configured");

  // Start by connecting to a WiFi network
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_STA, password_STA);

  waitWiFiconnected();

  if (WiFi.status() != WL_CONNECTED) {
    // if WiFi not connected, switch to access point mode
    wifiSTAmode = false;
    printConsole(T_BOOT, "Wifi: AP mode - create access point: " + String(ssid_AP));
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(ssid_AP, password_AP);
    printConsole(T_RUN, "Wifi: Connected, IP: " + String(WiFi.softAPIP().toString()));
  } else {
    printConsole(T_RUN, "Wifi: Connected, IP: " + String(WiFi.localIP().toString()));
  }


  // Set Hostname
  String hostname = "disabled";
#if ENABLE_MDNS
  hostname = device_Name;
  hostname.replace(" ", "");
  hostname.toLowerCase();
  if (!MDNS.begin(hostname, WiFi.localIP())) {
    hostname = "mDNS failed";
    printConsole(T_ERROR, "Wifi: " + hostname);
  } else {
    hostname += ".local";
    printConsole(T_RUN, "Wifi hostname: " + hostname);
  }
#endif

  if (wifiSTAmode) {
    printOLED("WiFi: " + String(ssid_STA),
              "Host: " + String(hostname),
              "IP: " + WiFi.localIP().toString());
  } else {
    printOLED("WiFi: " + String(ssid_AP),
              "Host: " + String(hostname),
              "IP: " + WiFi.softAPIP().toString());
  }

  delay(3000);



  // When the client requests data
  server.on("/getHead", getHead);
  server.on("/getValue", getValue);
  server.on("/getParameter", getParameter);
  server.on("/getWiFiNetworks", getWiFiNetworks);
  server.on("/saveParameter", saveParameter);
  server.on("/setPWM", setPWM);
  server.on("/manualFill", manualFill);
  server.on("/autoFill", autoFill);

  // When the client  file
  server.on("/settings.html", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleFileUpload);

  // If the client requests any URI
  server.onNotFound([]() {
    if (!handleFileRead(server.uri()))
      server.send(404, "text/plain", "Gas station Error: 404\n File or URL not Found !");
  });

  // init ElegantOTA
  ElegantOTA.begin(&server);

  // init webserver
  server.begin();
  printConsole(T_RUN, "Webserver is up and running");

  // init OTA (over the air update)
  if (enableOTA) {
    ArduinoOTA.setHostname(ssid_AP);
    ArduinoOTA.setPassword(password_AP);

    ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "firmware";
      } else { // U_SPIFFS
        type = "SPIFFS";
      }
      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      updateMsg = "Updating " + type;
      printConsole(T_UPDATE, type);
    });

    ArduinoOTA.onEnd([]() {
      updateMsg = "successful..";
      printUpdateProgress(100, 100);
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      printUpdateProgress(progress, total);
    });

    ArduinoOTA.onError([](ota_error_t error) {
      if (error == OTA_AUTH_ERROR) {
        updateMsg = "Auth Failed";
      } else if (error == OTA_BEGIN_ERROR) {
        updateMsg = "Begin Failed";
      } else if (error == OTA_CONNECT_ERROR) {
        updateMsg = "Connect Failed";
      } else if (error == OTA_RECEIVE_ERROR) {
        updateMsg = "Receive Failed";
      } else if (error == OTA_END_ERROR) {
        updateMsg = "End Failed";
      }
      printUpdateProgress(0, 100);
    });

    ArduinoOTA.begin();
    printConsole(T_RUN, "OTA is up and running");
  }

  // https update
  httpsClient.setInsecure();
  if (enableUpdate) {
    // check for update
    httpsUpdate(PROBE_UPDATE);
  }

}


void loop() {
  static uint32_t lastService = 0;
  if (lastService + 1000 < micros()) {
    lastService = micros();
    encoder.service();
    if (wait > 0) wait--;
  }

#if ENABLE_MDNS
  MDNS.update();
#endif

  if (enableOTA) {
    ArduinoOTA.handle();
  }
  server.handleClient();
  checkClientConnected();
  process_encoder_button();

  switch (mode) {
    case AUTO:
      automatic_sequence();
      break;
    case PARAMETER:
      switch (clientMode){
        case AUTO:
          automatic_sequence();
        break;
        case MANUAL:
          manual_sequence();
        break;
        default:
        // do nothing
        break;
      }
      break;
    default:
      manual_sequence();
      break;
  }

  // update display
  if ((millis() - lastTimeMenu) > UPDATE_INTERVAL_OLED_MENU) {
    lastTimeMenu = millis();
    printPWMOLED();

    if (batType > B_OFF) {
      batVolt = (analogRead(VOLTAGE_PIN) / 1024.0) * V_REF * ((resistor[R1] + resistor[R2]) / resistor[R2]) / 1000.0;
      if (batType > B_VOLT) {
        batVolt = percentBat(batVolt / batCells);
      }
    }
  }
}
