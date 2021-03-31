#include "EasyBuzzer.h"
unsigned int frequency = 4000;
unsigned int rotate_duration = 5;
unsigned int click_duration = 10;
unsigned int buzzer_pin = 12;

void buzzer_setup(){
    EasyBuzzer.setPin(buzzer_pin);
}

void buzzer_beep_click(){
    EasyBuzzer.singleBeep(
		frequency, 	// Frequency in hertz(HZ).  
		click_duration 	// Duration of the beep in milliseconds(ms). 
	);
}

void buzzer_beep_rotate(){
    EasyBuzzer.singleBeep(
		frequency, 	// Frequency in hertz(HZ).  
		rotate_duration 	// Duration of the beep in milliseconds(ms). 
	);
}

void buzzer_beep_double_click(){
    EasyBuzzer.singleBeep(
		frequency, 	// Frequency in hertz(HZ).  
		click_duration 	// Duration of the beep in milliseconds(ms). 
	);

}
void buzzer_loop(){
    EasyBuzzer.update();
}
