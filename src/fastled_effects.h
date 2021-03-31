/* rainbow_march_demo
 *
 * By: Andrew Tuline
 *
 * Date: March, 2015
 *
 * Rainbow marching up the strand. Pretty basic, but oh so popular, and we get a few options as well. No 'wheel' routine required.
 *
 */

#include "FastLED.h"                                          // FastLED library.

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif
 
// Fixed definitions cannot change on the fly.
#define LED_DT 14                                         // Data pin to connect to the strip.
// #define LED_CK 11
#define COLOR_ORDER BGR                                       // It's GRB for WS2812B and BGR for APA102
#define LED_TYPE WS2812                                       // What kind of strip are you using (WS2801, WS2812B or APA102)?
#define NUM_LEDS 30                                           // Number of LED's.

// Initialize changeable global variables.
uint8_t max_bright = 128;                                     // Overall brightness definition. It can be changed on the fly.
struct CRGB leds[NUM_LEDS];      

void fastled_setup(){
  LEDS.addLeds<LED_TYPE, LED_DT, COLOR_ORDER>(leds, NUM_LEDS);  // Use this for WS2801 or APA102
  FastLED.setBrightness(max_bright);
  FastLED.setMaxPowerInVoltsAndMilliamps(12, 500);                // FastLED 2.1 Power management set at 5V, 500mA
}

                             // Initialize our LED array.

// Initialize global variables for sequences
uint8_t thisdelay = 40;                                       // A delay value for the sequence(s)
uint8_t thishue = 0;                                          // Starting hue value.
int8_t thisrot = 5;                                           // Hue rotation speed. Includes direction.
uint8_t deltahue = NUM_LEDS;                                         // Hue change between pixels.
bool thisdir = 0;                                             // I use a direction variable instead of signed math so I can use it in multiple routines.

void ChangeRainbowLoopLenth() {                                             // A time (rather than loop) based demo sequencer. This gives us full control over the length of each sequence.
  
  uint8_t secondHand = (millis() / 1000) % 60;                // Change '60' to a different value to change length of the loop.
  static uint8_t lastSecond = 99;                             // Static variable, means it's only defined once. This is our 'debounce' variable.

  if (lastSecond != secondHand) {                             // Debounce to make sure we're not repeating an assignment.
    lastSecond = secondHand;
    switch(secondHand) {
      case  0: thisrot=1; deltahue=5; break;
      case  5: thisdir=-1; deltahue=10; break;
      case 10: thisrot=5; break;
      case 15: thisrot=5; thisdir=-1; deltahue=20; break;
      case 20: deltahue=30; break;
      case 25: deltahue=2; thisrot=5; break;
      case 30: break;
    }
  }
}

void rainbow_effect_loop(){
  //fastled loop
  // ChangeRainbowLoopLenth();

  EVERY_N_MILLISECONDS(thisdelay) {                           // FastLED based non-blocking delay to update/display the sequence.
    if (thisdir == 0) thishue += thisrot; else thishue-= thisrot;  // I could use signed math, but 'thisdir' works with other routines.
    fill_rainbow(leds, NUM_LEDS, thishue, deltahue);            // I don't change deltahue on the fly as it's too fast near the end of the strip.
  }
  FastLED.show();
}// ChangeMe()


//*********************Palette effect*******************//
CRGBPalette16 currentPalette(CRGB::Black);
CRGBPalette16 targetPalette(PartyColors_p );
TBlendType    currentBlending;       

void FillLEDsFromPaletteColors(uint8_t colorIndex) {
  
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(currentPalette, colorIndex + sin8(i*16), 255);
    colorIndex += 3;
  }

} // FillLEDsFromPaletteColors()

void ChangePalettePeriodically() {
  
  uint8_t secondHand = (millis() / 1000) % 60;
  static uint8_t lastSecond = 99;
  
  if(lastSecond != secondHand) {
    lastSecond = secondHand;
    CRGB p = CHSV(HUE_PURPLE, 255, 255);
    CRGB g = CHSV(HUE_GREEN, 255, 255);
    CRGB b = CRGB::Black;
    CRGB w = CRGB::White;
    if(secondHand ==  0)  { targetPalette = RainbowColors_p; }
    if(secondHand == 10)  { targetPalette = CRGBPalette16(g,g,b,b, p,p,b,b, g,g,b,b, p,p,b,b); }
    if(secondHand == 20)  { targetPalette = CRGBPalette16(b,b,b,w, b,b,b,w, b,b,b,w, b,b,b,w); }
    if(secondHand == 30)  { targetPalette = LavaColors_p; }
    if(secondHand == 40)  { targetPalette = CloudColors_p; }
    if(secondHand == 50)  { targetPalette = PartyColors_p; }
  }
  
} // ChangePalettePeriodically()

void palette_effect_loop(){
  ChangePalettePeriodically();
  // nblendPaletteTowardPalette() will crossfade current palette slowly toward the target palette.
  //
  // Each time that nblendPaletteTowardPalette is called, small changes
  // are made to currentPalette to bring it closer to matching targetPalette.
  // You can control how many changes are made in each call:
  //   - the default of 24 is a good balance
  //   - meaningful values are 1-48.  1=veeeeeeeery slow, 48=quickest
  //   - "0" means do not change the currentPalette at all; freeze
  EVERY_N_MILLISECONDS(100) {
    uint8_t maxChanges = 24; 
    nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);
  }
  EVERY_N_MILLISECONDS(thisdelay) {
    static uint8_t startIndex = 0;
    startIndex += 1;                                 // motion speed
    FillLEDsFromPaletteColors(startIndex);
  }
  FastLED.show();
}

void black_effect_loop(){
  fill_solid(leds, NUM_LEDS, CRGB::Black);                    // Just to be sure, let's really make it BLACK.
  FastLED.show();                         // Power managed display
}
//*********************Palette effect*******************//


//********************dot beat***********************//

uint8_t bpm = 30;
uint8_t fadeval = 224;  

void dot_beat_loop() {

  uint8_t inner = beatsin8(bpm, NUM_LEDS/4, NUM_LEDS/4*3);    // Move 1/4 to 3/4
  uint8_t outer = beatsin8(bpm, 0, NUM_LEDS-1);               // Move entire length
  uint8_t middle = beatsin8(bpm, NUM_LEDS/3, NUM_LEDS/3*2);   // Move 1/3 to 2/3

  leds[middle] = CRGB::Purple;
  leds[inner] = CRGB::Blue;
  leds[outer] = CRGB::Aqua;

  EVERY_N_MILLISECONDS(thisdelay){
    nscale8(leds,NUM_LEDS,fadeval);                             // Fade the entire array. Or for just a few LED's, use  nscale8(&leds[2], 5, fadeval);
    FastLED.show();
  }
} // dot_beat()
//*********************dot beat*************************//

//************************blur***********************//
void blur_loop() {

  uint8_t blurAmount = dim8_raw( beatsin8(3,64, 192) );       // A sinewave at 3 Hz with values ranging from 64 to 192.
  uint8_t  i = beatsin8(  9, 0, NUM_LEDS);
  uint8_t  j = beatsin8( 7, 0, NUM_LEDS);
  uint8_t  k = beatsin8(  5, 0, NUM_LEDS);
  
  // The color of each point shifts over time, each at a different speed.
  uint16_t ms = millis();  
  leds[(i+j)/2] = CHSV( ms / 29, 200, 255);
  leds[(j+k)/2] = CHSV( ms / 41, 200, 255);
  leds[(k+i)/2] = CHSV( ms / 73, 200, 255);
  leds[(k+i+j)/3] = CHSV( ms / 53, 200, 255);

  EVERY_N_MILLISECONDS(thisdelay){
    blur1d( leds, NUM_LEDS, blurAmount);                        // Apply some blurring to whatever's already on the strip, which will eventually go black.
    FastLED.show();
  }
} // loop()
//***************************blur*************************//