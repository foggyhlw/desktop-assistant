#define LUX_UPDATE_PERIOD  2000
#define FANS_UPDATE_PERIOD 60000
unsigned long time_now1 = 0;
unsigned long time_now2 = 0;

#include <Wifi.h>
#include "temp_humi.h"
#include "beep.h"
#include "ntp_time.h"

#include "fastled_effects.h"

#include <Arduino.h>
#include <SPI.h>
#include "parameters.h"
#include <EEPROM.h>

#include <menu.h>
#include <menuIO/rotaryEventIn.h>
#include <menuIO/chainStream.h>
#include "Button2.h"
#include "ESPRotary.h"


ESPRotary r = ESPRotary(ROTARY_PIN1, ROTARY_PIN2, CLICKS_PER_STEP);
Button2 b = Button2(BUTTON_PIN);

#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <Wire.h>
#include <BH1750.h>
BH1750 lightMeter(0x23);

#include "EspMQTTClient.h"
EspMQTTClient client(
  "YOUR_WIFI_SSID",
  "YOUR_WIFI_PASSWD",
  "YOURMQTTIP",  // MQTT Broker server ip
  "MQTT-USERNAME",   // Can be omitted if not needed
  "MQTT_PASSWD",   // Can be omitted if not needed
  "TestClient",     // Client name that uniquely identify your device
  1883              // The MQTT port, default to 1883. this line can be omitted
);

#include <menuIO/u8g2Out.h>
#include <menuIO/serialOut.h>
#include <menuIO/serialIn.h>
#define defaultFont u8g2_font_7x13B_tf 
#define mediumFont u8g2_font_t0_17b_tr    
#define largeFont u8g2_font_timB24_tn    
#define fontX 7
#define fontY 16
#define offsetX 0
#define offsetY 3
#define U8_Width 128
#define U8_Height 64
U8G2_SSD1306_128X64_NONAME_1_4W_HW_SPI u8g2(U8G2_R0, CS_PIN, DC_PIN, RST_PIN);

using namespace Menu;

RotaryEventIn reIn(
  RotaryEventIn::EventType::BUTTON_CLICKED | // select
  RotaryEventIn::EventType::BUTTON_DOUBLE_CLICKED | // back
  RotaryEventIn::EventType::BUTTON_LONG_PRESSED | // also back
  RotaryEventIn::EventType::ROTARY_CCW | // up
  RotaryEventIn::EventType::ROTARY_CW // down
); 
serialIn serial(Serial);
MENU_INPUTS(in,&reIn,&serial);

result doAlert(eventMask e, prompt &item);

result showEvent(eventMask e, navNode& nav, prompt& item) {
  Serial.print("event: ");
  Serial.println(e);
  return proceed;
}

//used to choose screensaver
CHOOSE(screen_saver_mode, screensaverMenu, " -SCREEN", doNothing, noEvent, noStyle
       , VALUE(" CHANGE", 0, doNothing, noEvent)
       , VALUE(" BILIBILI", 1, doNothing, noEvent)
       , VALUE(" DATETIME", 2, doNothing, noEvent)
       , VALUE(" TEMPHUMI", 3, doNothing, noEvent)

      );

//used to choose fastled effect
int effect_mode = 0;
CHOOSE(effect_mode, effectMenu, " -EFFECT", doNothing, noEvent, noStyle
       , VALUE(" LED-OFF", 0, doNothing, noEvent)
       , VALUE(" RAINBOW", 1, doNothing, noEvent)
       , VALUE(" PALETTE", 2, doNothing, noEvent)
       , VALUE(" DOTBEAT", 3, doNothing, noEvent)
       , VALUE(" BLUR", 4, doNothing, noEvent)
      );


//brightness_set: manually control monitor brightness
int brightness = BRIGHTNESS_LOW;
//lux: lux value from bh1750. update interval is set in loop() function
int lux =0;
uint8_t monitor_auto_control = BRIGHTNESS_AUTO;
//valid charactors for EDIT 
char* const validators[] PROGMEM = {" 0123456789%"};
//current_lux_str is updated in loop()
TOGGLE(monitor_auto_control, setMonitorControlMode, " -MODE: ", save_monitor_control_mod, anyEvent, noStyle //,doExit,enterEvent,noStyle
       , VALUE("Auto", BRIGHTNESS_AUTO, doNothing, noEvent)
       , VALUE("Manual", BRIGHTNESS_MANUAL, doNothing, noEvent)
      );

MENU(monitor_subMenu, " -MONITOR", 
doNothing, noEvent, noStyle
     , FIELD(lux, " -LUX", " L", 0, 100, 10, 1, doNothing, noEvent, wrapStyle)
    //  , EDIT("LUX: ",current_lux_str, validators, doNothing, noEvent, noStyle)
     , SUBMENU(setMonitorControlMode)
     //to decide whitch event to use
     , FIELD(brightness, " -BRIGHTNESS", "LEV", BRIGHTNESS_LOW, BRIGHTNESS_HIGH, 1, 1, update_brightness_cmd, updateEvent , noStyle)
    //  , OP("Lux:", showEvent, anyEvent)
    //  , OP("Sub2", showEvent, anyEvent)
    //  , OP("Sub3", showEvent, anyEvent)
     , EXIT(" >BACK")
    );

MENU(mainMenu, " DESKTOP-ASSIS", doNothing, noEvent, wrapStyle
    //  , OP("Op1", action1, anyEvent)
    //  , OP("Op2", action2, enterEvent)
     , SUBMENU(monitor_subMenu)
    //  , SUBMENU(setLed)
    //  , OP("LED On", myLedOn, enterEvent)
    //  , OP("LED Off", myLedOff, enterEvent)
     , SUBMENU(effectMenu)
     , SUBMENU(screensaverMenu)
    //  , altOP(altPrompt, "", showEvent, anyEvent)
    //  , OP("Alert test", doAlert, enterEvent)
     , EXIT(" >Back")
    );
 
#define MAX_DEPTH 3

// define menu colors --------------------------------------------------------
//each color is in the format:
//  {{disabled normal,disabled selected},{enabled normal,enabled selected, enabled editing}}
// this is a monochromatic color table
const colorDef<uint8_t> colors[6] MEMMODE={
  {{0,0},{0,1,1}},//bgColor
  {{1,1},{1,0,0}},//fgColor
  {{1,1},{1,0,0}},//valColor
  {{1,1},{1,0,0}},//unitColor
  {{0,1},{0,0,0}},//cursorColor
  {{1,1},{1,0,0}},//titleColor
};

MENU_OUTPUTS(out,MAX_DEPTH
  ,U8G2_OUT(u8g2,colors,fontX,fontY,offsetX,offsetY,{0,0,U8_Width/fontX,U8_Height/fontY})
  ,SERIAL_OUT(Serial)
);

//macro to create navigation control root object (nav) using mainMenu
NAVROOT(nav, mainMenu, MAX_DEPTH, in,  out);



result alert(menuOut& o, idleEvent e) {
  if (e == idling) {
    o.setCursor(0, 0);
    o.print("alert test");
    o.setCursor(0, 1);
    o.print("press [select]");
    o.setCursor(0, 2);
    o.print("to continue...");
  }
  return proceed;
}

result doAlert(eventMask e, prompt &item) {
  nav.idleOn(alert);
  return proceed;
}

int fans_num = 0;
void get_bilibili_fans(int &fans_num){
  if (WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    http.begin(FANS_URL);
    int httpCode = http.GET();
    if(httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      StaticJsonDocument<300> json_doc; // for json deserializeJson
      DeserializationError error = deserializeJson(json_doc, payload);
      // Test if parsing succeeds.
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
      }
    fans_num = int(json_doc["data"]["follower"]);
    }
  }
}

//when menu is suspended (idle state)
result screen_saver(menuOut &o, idleEvent e) {
  if (screen_saver_mode == BILIBILI_MODE){
    u8g2.drawXBMP(15,0,BILIBILI_BMP_width,BILIBILI_BMP_height,BILIBILI_BMP_bits);
    u8g2.setFont(mediumFont);
    u8g2.setCursor(70,20);
    u8g2.print("Fans:");
    u8g2.setCursor(75,40);
    u8g2.println(fans_num);
    u8g2.setCursor(30,60);
    u8g2.println("foggyhlw");
    u8g2.setFont(defaultFont);
  }
  if (screen_saver_mode == TIME_MODE){
    u8g2.setCursor(30,40);
    u8g2.setFont(largeFont);
    if(timeinfo.tm_hour < 10){
      u8g2.print(0);
    }
    u8g2.print(timeinfo.tm_hour);
    u8g2.print(":");
    if(timeinfo.tm_min < 10){
      u8g2.print(0);
    }
    u8g2.print(timeinfo.tm_min);
    u8g2.setCursor(12,57);
    u8g2.setFont(mediumFont);
    // u8g2.print(timeinfo.tm_year+1900);
    // u8g2.print("-");
    u8g2.print(timeinfo.tm_mon+1);
    u8g2.print("-");
    u8g2.print(timeinfo.tm_mday);
    u8g2.print(" ");
    u8g2.println(&timeinfo, "%A");
    u8g2.setFont(defaultFont);
  }
  if (screen_saver_mode == TEMP_HUMI_MODE){
    u8g2.setFont(mediumFont);
    u8g2.setCursor(0,20);
    u8g2.print(" Environment");
    u8g2.setCursor(0,40);
    u8g2.print("Temp: ");
    u8g2.print(temperature);
    u8g2.println(" C");
    u8g2.setCursor(0,60);
    u8g2.print("Humi:   ");
    u8g2.println(humidity);
    u8g2.println(" %");
    u8g2.setFont(defaultFont);
  }
    // switch (e) {
    //   case idleStart: o.println("suspending menu!"); break;
    //   case idling: o.println("suspended..."); break;
    //   case idleEnd: o.println("resuming menu."); break;
    // }
  return proceed;
}

//used to change screen_saver when idling
void cycle_choose_screen_saver(bool rotate_direction){
  if(screen_saver_mode < MODE_NUM && screen_saver_mode > 0){
    if(rotate_direction){
      screen_saver_mode++;
    }
    else{
      screen_saver_mode--;
    }
  }
  if(screen_saver_mode >= MODE_NUM){
    screen_saver_mode = 1;
  }
  if(screen_saver_mode <= 0){
    screen_saver_mode = MODE_NUM-1;
  }
  // Serial.print(screen_saver_mode);
}
// result idle(menuOut& o,idleEvent e) {
//   o.clear();
//   switch(e) {
//     case idleStart:o.println("suspending menu!");break;
//     case idling:o.println("suspended...");break;
//     case idleEnd:o.println("resuming menu.");break;
//   }
//   return proceed;
// }

void setup() {
  Serial.begin(115200);

  // BH1750
  Wire.begin();
  lightMeter.begin();
// EEPROM
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(BRIGHTNESS_SET_ADDR,brightness);
  EEPROM.get(MONITOR_AUTO_CONTROL_ADDR,monitor_auto_control);

//rotary and button cb
  r.setChangedHandler(rotate_callback);
  b.setTapHandler(click_callback);
  b.setLongClickHandler(longclick_callback);
  b.setDoubleClickHandler(doubleclick_callback);

  // out put device ssd1306
  SPI.begin();
  u8g2.begin();
  u8g2.setFont(defaultFont);

  nav.idleTask = screen_saver; //point a function to be used when menu is suspended
  //enable updateEvent to set brightness for monitor
  nav.useUpdateEvent = true;
  //set timeout for menu
  nav.timeOut = 15;
  //disable FIELD CurrentLux to make it read-only
  monitor_subMenu[0].disable();
  // while(!WiFi.isConnected());
  buzzer_setup();
   //ntp 
  dht_setup();
  ntp_setup();

  get_bilibili_fans(fans_num);

  fastled_setup();
  updateLocalTime();

}

void loop() {
  if(millis() > time_now1 + LUX_UPDATE_PERIOD){
    time_now1 = millis();
    lux = lightMeter.readLightLevel();
    if(monitor_auto_control == BRIGHTNESS_AUTO){
        brightness = convert_lux_to_brightness(lux);
        update_brightness_cmd();
      }
  //dht update period is the same as lux
    dht_loop();
  }
  if(millis() > time_now2 + FANS_UPDATE_PERIOD){
    time_now2 = millis();
    get_bilibili_fans(fans_num);
    updateLocalTime();
  }
  //if (!digitalRead(A0)) action1(enterEvent);
  r.loop(); // rotary loop 
  b.loop(); // button loop
  client.loop(); // mqtt client loop
  // nav.poll();  //
  nav.doInput();
  if (nav.changed(0)) {//only draw if menu changed for gfx device
    //change checking leaves more time for other tasks
    u8g2.firstPage();
    do nav.doOutput(); while(u8g2.nextPage());
  }
  buzzer_loop();

  switch(effect_mode){
    case 0: black_effect_loop();break;
    case 1: rainbow_effect_loop();break;
    case 2: palette_effect_loop();break;
    case 3: dot_beat_loop();break;
    case 4: blur_loop();break;
  }
  nav.idleChanged=true;   // trigger idleTask to run repeatedly
}

// on change
void rotate_callback(ESPRotary& r) {
//   Serial.println(r.getPosition());
  buzzer_beep_rotate();
  if(r.getDirection() == RE_LEFT){
    reIn.registerEvent(RotaryEventIn::EventType::ROTARY_CW);
    if(nav.sleepTask){
      //used to change screen_saver when idling
      cycle_choose_screen_saver(false);
    }
  }
  if(r.getDirection() == RE_RIGHT){
    reIn.registerEvent(RotaryEventIn::EventType::ROTARY_CCW);
    if(nav.sleepTask){
      //used to change screen_saver when idling
      cycle_choose_screen_saver(true);
    }
  }
}

// single click
void click_callback(Button2& btn) {
  buzzer_beep_click();

  reIn.registerEvent(RotaryEventIn::EventType::BUTTON_CLICKED);
//  Serial.println("Click!");
}

// long click
void longclick_callback(Button2& btn) {
  // reIn.registerEvent(RotaryEventIn::EventType::BUTTON_LONG_PRESSED);
  nav.doNav(navCmd(escCmd));
  buzzer_beep_double_click();
}

void doubleclick_callback(Button2& btn) {
  reIn.registerEvent(RotaryEventIn::EventType::BUTTON_DOUBLE_CLICKED);
  buzzer_beep_double_click();
}

void onConnectionEstablished()
{
  // Subscribe to "mytopic/test" and display received message to Serial
  client.subscribe("/desktop-assistant/cmd", [](const String & payload) {
    Serial.println(payload);
  });
  Serial.println("Connected!");
  // Subscribe to "mytopic/wildcardtest/#" and display received message to Serial
  client.subscribe("mytopic/wildcardtest/#", [](const String & topic, const String & payload) {
    Serial.println("(From wildcard) topic: " + topic + ", payload: " + payload);
  });

  // Publish a message to "mytopic/test"
  client.publish("mytopic/test", "This is a message"); // You can activate the retain flag by setting the third parameter to true

  // Execute delayed instructions
  client.executeDelayed(5 * 1000, []() {
    client.publish("mytopic/wildcardtest/test123", "This is a message sent 5 seconds later");
  });
}

void update_brightness_cmd(){
  if(client.isMqttConnected()){
    if(monitor_auto_control != BRIGHTNESS_AUTO){
      client.publish("/desktop-assistant/monitor/brightness_manual", String(brightness));
      EEPROM.put(BRIGHTNESS_SET_ADDR, brightness);
      EEPROM.commit();
    }
    else{
      client.publish("/desktop-assistant/monitor/brightness_auto", String(brightness));
    }
  }
}

void save_monitor_control_mod(){
  Serial.print("eeprom put");
  EEPROM.put(MONITOR_AUTO_CONTROL_ADDR, monitor_auto_control);
  EEPROM.commit();
}

int convert_lux_to_brightness(int lux){
  if(lux > 300){
    return BRIGHTNESS_LEVEL4;
  }
  else if(lux > 100){
    return BRIGHTNESS_LEVEL3;
  }
  else if(lux > 40){
    return BRIGHTNESS_LEVEL2;
  }
  else{
    return BRIGHTNESS_LEVEL1;
  }
}
