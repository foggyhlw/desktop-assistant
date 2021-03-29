#define LUX_UPDATE_PERIOD  2000
#define FANS_UPDATE_PERIOD 60000
unsigned long time_now1 = 0;
unsigned long time_now2 = 0;

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
  "foggy_2G",
  "1989Fox228",
  "192.168.0.100",  // MQTT Broker server ip
  "foggy",   // Can be omitted if not needed
  "1989228",   // Can be omitted if not needed
  "TestClient",     // Client name that uniquely identify your device
  1883              // The MQTT port, default to 1883. this line can be omitted
);

#include <menuIO/u8g2Out.h>
#include <menuIO/serialOut.h>
#include <menuIO/serialIn.h>
#define defaultFont u8g2_font_7x13B_tf 
#define bigFont u8g2_font_t0_17b_tr    
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

//used to choose fastled effect
int effect_mode = 0;
CHOOSE(effect_mode, effectMenu, " -EFFECT", doNothing, noEvent, noStyle
       , VALUE(" LED-OFF", 0, doNothing, noEvent)
       , VALUE(" RAINBOW", 1, doNothing, noEvent)
       , VALUE(" PALETTE", 2, doNothing, noEvent)
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
     , FIELD(brightness, " -BRIGHTNESS", "%", BRIGHTNESS_LOW, BRIGHTNESS_HIGH, 10, 1, update_brightness_cmd, updateEvent , noStyle)
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
    //  , SUBMENU(selMenu)
     , SUBMENU(effectMenu)
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

//when menu is suspended (idle state)
result update_bilibili_fans(menuOut &o, idleEvent e) {
  // to ensure wifi is connected
    o.clear();
    u8g2.drawXBMP(15,0,BILIBILI_BMP_width,BILIBILI_BMP_height,BILIBILI_BMP_bits);
    // Serial.print(fans_num);
    // int fans_num = 1900;

    u8g2.setFont(bigFont);
    u8g2.setCursor(70,20);
    // Serial.print(fans_num);
    u8g2.print("Fans:");
    u8g2.setCursor(75,40);
    u8g2.println(fans_num);
    u8g2.setCursor(30,60);
    u8g2.println("foggyhlw");
    u8g2.setFont(defaultFont);
    
    // switch (e) {
    //   case idleStart: o.println("suspending menu!"); break;
    //   case idling: o.println("suspended..."); break;
    //   case idleEnd: o.println("resuming menu."); break;
    // }
  return proceed;
}

result idle(menuOut& o,idleEvent e) {
  o.clear();
  switch(e) {
    case idleStart:o.println("suspending menu!");break;
    case idling:o.println("suspended...");break;
    case idleEnd:o.println("resuming menu.");break;
  }
  return proceed;
}

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
  r.setChangedHandler(rotate);
//  r.setLeftRotationHandler(leftrotatehandler);
//  r.setRightRotationHandler(rightrotatehandler);

  b.setTapHandler(click);
  b.setLongClickHandler(longclick);
  b.setDoubleClickHandler(doubleclick);
  // out put device ssd1306

  SPI.begin();
  u8g2.begin();
  u8g2.setFont(defaultFont);

  nav.idleTask = update_bilibili_fans; //point a function to be used when menu is suspended
  //enable updateEvent to set brightness for monitor
  nav.useUpdateEvent = true;
  //set timeout for menu
  nav.timeOut = 15;
  //disable FIELD CurrentLux to make it read-only
  monitor_subMenu[0].disable();
 
  get_bilibili_fans(fans_num);

  LEDS.addLeds<LED_TYPE, LED_DT, COLOR_ORDER>(leds, NUM_LEDS);  // Use this for WS2801 or APA102

  FastLED.setBrightness(max_bright);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);                // FastLED 2.1 Power management set at 5V, 500mA
  

}

void loop() {
  if(millis() > time_now1 + LUX_UPDATE_PERIOD){
    time_now1 = millis();
    lux = lightMeter.readLightLevel();
    if(monitor_auto_control == BRIGHTNESS_AUTO){
      brightness = convert_lux_to_brightness(lux);
      update_brightness_cmd();
      }
    nav.idleChanged=true;   // trigger idleTask to run repeatedly
  }
  if(millis() > time_now2 + FANS_UPDATE_PERIOD){
    time_now2 = millis();
    get_bilibili_fans(fans_num);
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

switch(effect_mode){
  case 0: black_effect_loop();break;
  case 1: rainbow_effect_loop();break;
  case 2: palette_effect_loop();break;
}
  
}

// on change
void rotate(ESPRotary& r) {
//   Serial.println(r.getPosition());
  if(r.getDirection() == RE_LEFT){
    reIn.registerEvent(RotaryEventIn::EventType::ROTARY_CW);
  }
  if(r.getDirection() == RE_RIGHT){
    reIn.registerEvent(RotaryEventIn::EventType::ROTARY_CCW);
  }
}

// on left or right rotation
void showDirection(ESPRotary& r) {
//  Serial.println(r.directionToString(r.getDirection()));
}

// void leftrotatehandler(ESPRotary& r){
//   reIn.registerEvent(RotaryEventIn::EventType::ROTARY_CW);
// }
// void rightrotatehandler(ESPRotary& r){
//   reIn.registerEvent(RotaryEventIn::EventType::ROTARY_CCW);
// }

// single click
void click(Button2& btn) {
  reIn.registerEvent(RotaryEventIn::EventType::BUTTON_CLICKED);
//  Serial.println("Click!");
}

// long click
void longclick(Button2& btn) {
  // reIn.registerEvent(RotaryEventIn::EventType::BUTTON_LONG_PRESSED);
  nav.doNav(navCmd(escCmd));
}

void doubleclick(Button2& btn) {
  reIn.registerEvent(RotaryEventIn::EventType::BUTTON_DOUBLE_CLICKED);
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
  if(lux > 200){
    return BRIGHTNESS_HIGH;
  }
  else if(lux > 100){
    return int((BRIGHTNESS_HIGH+BRIGHTNESS_LOW)/2+BRIGHTNESS_LOW);
  }
  else if( lux >20){
    return int((BRIGHTNESS_HIGH+BRIGHTNESS_LOW)/4+BRIGHTNESS_LOW);
  }
  else{
    return BRIGHTNESS_LOW;
  }
}
