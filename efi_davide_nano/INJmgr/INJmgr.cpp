#ifndef INJmgr_cpp
#define INJmgr_cpp

#include "INJmgr.h"
#include "Tempo/Tempo.h"

// GLOBAL VARIABLES

volatile uint16_t injection_counter=0; // counts the combustion cycles
volatile uint16_t inj_start_tick_last=0; // time at which injection was started last time
volatile uint16_t delta_time_tick=0; // distanza tra due iniezioni (2rpm)
volatile uint16_t inj_end_tick=0; // tempo di fine iniezione da ECU
volatile uint16_t delta_inj_tick=0; // tempo di iniezione input
volatile uint16_t extension_time_ticks=0; // extension time Timer1 ticks (0.5us)
volatile uint16_t perc_inc=0; // percentuale di incremento (letta dalla mappa a seconda di RPM, THR, TIM)
uint8_t incrementi_rpm[INJ_INCR_RPM_MAPS_SIZE]; // increments, based on rpm, used by SW
uint8_t incrementi_thr[INJ_INCR_THR_MAPS_SIZE]; // increments depending on throttle
uint8_t incrementi_tim[INJ_INCR_TIM_MAPS_SIZE]; // increments depending on injection time
uint16_t incrementi_rpm_brkpts[] = {2500, 2756, 3012, 3524, 4548, 6596, 10692, 18884}; // breakpoints, size should be INJ_INCR_RPM_MAPS_SIZE
uint8_t incrementi_rpm_shifts[] = {8, 8, 9, 10, 11, 12, 13}; // breakpoints, size should be (INJ_INCR_RPM_MAPS_SIZE-1)
volatile unsigned int throttle=0; // voltage on throttle pin 0-1023
volatile unsigned int lambda=0;  // voltage on lambda sensor pin
volatile bool INJ_safety_inj_turned_off = false; // Injector was not turned OFF since the last function call

// Buffer variables, to store data before sending on Serial or storing on SD
volatile uint16_t injection_counter_buffer=0; // counts the combustion cycles
volatile uint16_t delta_time_tick_buffer=0; // distanza tra due iniezioni (2rpm) buffer
volatile uint16_t delta_inj_tick_buffer=0; // tempo di iniezione input buffer
volatile uint16_t throttle_buffer=0; // tensione del throttle pin 0-1023 buffer
volatile uint16_t lambda_buffer=0; // lambda sensor voltage
volatile uint16_t extension_time_ticks_buffer=0;
volatile uint16_t INJ_exec_time_1_buffer = 0; // tempo di esecuzione
volatile uint16_t INJ_exec_time_2_buffer = 0; // tempo di esecuzione
volatile uint8_t buffer_busy=0; // viene attivato quando sta mandando fuori i dati in seriale

extern volatile unsigned long timer0_overflow_count; // needed to read the microseconds counter

/*
enum INJ_signal_trigger_mode_enum{
	WAIT_FOR_LOW = 0,
	WAIT_FOR_HIGH
};
volatile INJ_signal_trigger_mode_enum INJ_signal_trigger_mode = WAIT_FOR_LOW; // can be one or the other, as you wish
*/
volatile uint8_t INJ_signal_status = 0; // Injector Input Signal logic status. 0 = injector OFF, 1= injector ON. This value might be opposite than electric signal

volatile uint16_t INJ_exec_time_1 = 0;
volatile uint16_t INJ_exec_time_2 = 0;

// FUNCTIONS

void INJmgr_class::begin(){
	
	// INJ pin input output settings
	pinMode(IN_INJ_PIN, INPUT);
	pinMode(OUT_INJ_PIN, OUTPUT);
	digitalWrite(OUT_INJ_PIN, LOW);
	
	// INJ pin input interrupt settings (INT0 and Timer1)
	PCICR |=  1 << PCIE2; // PCIE2 (Port D)
	#if FUELINO_HW_VERSION == 1
		PCMSK2 |= 1 << PCINT18; // PCINT18 (Port D Pin 2)
	#elif FUELINO_HW_VERSION == 2
		PCMSK2 |= 1 << PCINT22; // PCINT22 (Port D Pin 6)
	#endif
	PCIFR |= 1 << PCIF2; // PCIF2 (Port D)
	Timer1.initialize(); // Initializes Timer1 for injection time management
	
	INJ_safety_err_counter = 0; // Safety error counter
	
}


// Returns the number of tick counts of Timer0 (1 tick = 4 us, since f=16MHz and prescaler = 64). 1 Overflow happens each 256 * 4 us = 1024 us
// The returned data size is 2 bytes (16 bit), allowing to count up to 262144 us
uint16_t INJmgr_class::Timer0_tick_counts() {
	
	uint8_t m;
	uint8_t t;
	
	uint8_t oldSREG = SREG;
	cli();
	m = (uint8_t)(timer0_overflow_count & 0xFF);
	t = TCNT0;
	if ((TIFR0 & _BV(TOV0)) && (t < 255)) m++;
	SREG = oldSREG;
	
	return ((m << 8) + t);
	
}


uint8_t INJmgr_class::interpolate_map(uint16_t* breakpoints_array, bool brkpts_incr, uint8_t* calib_array, uint8_t arrays_size, uint16_t x_value){
	
	// Search for index
	if (x_value <= breakpoints_array[0]) return calib_array[0]; // return array lowest value (the x value was lower than all the elements)
	uint8_t index; // index found	
	for (index=1; index<arrays_size; index++){ // scan all breakpoints
		if (x_value < breakpoints_array[index]){ // x lower is higher than next element
			index--; // previous index
			break; // exit cycle
		}
	}
	if (index == arrays_size) return calib_array[arrays_size-1]; // return array highest value (the x value was higher than all the elements)
	
	// Performs subtraction and multiplication
	uint16_t rem_x = x_value - breakpoints_array[index]; // remaining part of x value
	uint8_t y0 = calib_array[index];
	uint8_t y1 = calib_array[index+1];
	uint8_t dy;
	bool positive;
	if (y1 >= y0){
		dy = y1 - y0; // y increment positive
		positive=true;
	}else{
		dy = y0 - y1; // y increment negative
		positive=false;
	}
	uint16_t mult_tmp = (uint16_t)((uint32_t)rem_x * (uint32_t)dy);
	
	// Performs division
	uint8_t div_shifts = incrementi_rpm_shifts[index]; // number of shifts on the right
	mult_tmp = mult_tmp >> div_shifts; 
	/*for (uint8_t i=0; i<div_shifts; i++){ // shift right "div_shift" times
		mult_tmp = mult_tmp >> 1; 
	}*/
	
	// Performs addition
	uint8_t add_tmp;
	if (positive){
		add_tmp = y0 + (uint8_t)mult_tmp;
	}else{
		add_tmp = y0 - (uint8_t)mult_tmp;
	}
	
	return add_tmp;

}


// Updates the info buffer for logger / serial communication
void INJmgr_class::update_info_for_logger() {
  if (!buffer_busy) { // the ECU is not using the buffer to send data to the logger
    injection_counter_buffer = injection_counter; // injection counter is increased
	delta_time_tick_buffer = delta_time_tick; // distanza tra due iniezioni (2rpm) buffer
    delta_inj_tick_buffer = delta_inj_tick; // tempo di iniezione input buffer
	INJ_exec_time_1_buffer = INJ_exec_time_1; // execution time for the interrupt
	INJ_exec_time_2_buffer = INJ_exec_time_2; // execution time for the interrupt
    throttle_buffer = throttle; // voltage on throttle pin 0-1023 buffer
	lambda_buffer = lambda; // voltage on lambda sensor pin 
	extension_time_ticks_buffer=extension_time_ticks;
  }
}


// Deactivates the injector when Timer1 interrupt happens
void deactivate_inj(){
	Timer1.stop(); // stops the timer
	digitalWrite(OUT_INJ_PIN, LOW); // shuts down injector
	INJ_safety_inj_turned_off = true; // Injector turned OFF (this flag is used for Safety check)
	Timer1.detachInterrupt(); // Removes Timer 1 interrupt
	INJmgr.update_info_for_logger(); // Updates info buffer for logger / serial
}


// Acquires signal from throttle sensor
void INJmgr_class::analog_digital_signals_acquisition(){
	
	uint16_t temp_read = analogRead(THROTTLE_PIN); // reads throttle voltage
	uint16_t temp_read2 = analogRead(LAMBDA_PIN); // reads lambda voltage

	// Atomic write
	uint8_t oldSREG = SREG;				
	cli(); // Disable interrupts for 16 bit register access
	throttle = temp_read; // atomic write
	lambda = temp_read2; // atomic write
	SREG = oldSREG;
}


// This function has the purpose of checking if the injector has been properly deactivated
void INJmgr_class::safety_check(){
	// Checks if, from last function call, the interrupt properly turned on the flag
	if (INJ_safety_inj_turned_off == false){ // If this flag is active, interrupt did not set the injector to OFF
		INJ_safety_err_counter++; // Increase error counter
		
	}else{
		INJ_safety_err_counter = 0; // No error, reset the counter
	}
	if(INJ_safety_err_counter > INJ_SAFETY_MAX_ERR){ // If maximum error number is reached
		digitalWrite(OUT_INJ_PIN, LOW); // shuts down injector
		INJ_safety_err_counter = 0;
	}
	INJ_safety_inj_turned_off = false; // re-triggers the flag
}


// Interrupt pin status changes (Original ECU injector command pin)
ISR(PCINT2_vect){
	
	// For execution time measurement
	uint16_t INJ_exec_time_start = INJmgr.Timer0_tick_counts(); // 1 tick = 4us

	INJ_signal_status = !digitalRead(IN_INJ_PIN);
	
	// Original ECU is enabling injector (Injector valve will open and gasoline flows)
	if (INJ_signal_status) {
		digitalWrite(OUT_INJ_PIN, HIGH); // activates injector
		//inj_start_tick = INJ_exec_time_start; // 1 tick = 4us
		delta_time_tick = INJ_exec_time_start - inj_start_tick_last; // calculates Timer0 ticks (4us) between 2 consecutive injections (2 rpm)
		perc_inc = INJmgr.interpolate_map(incrementi_rpm_brkpts, true, incrementi_rpm, INJ_INCR_RPM_MAPS_SIZE, delta_time_tick); // returns the interpolated increment based on rpm
		//if (perc_inc > MAX_PERCENTAGE_INJ) perc_inc = MAX_PERCENTAGE_INJ; // saturates the maximum injection percentage, for safety
		inj_start_tick_last = INJ_exec_time_start; // saves old time stamp
		injection_counter++; // increases the combustion cycles
	}

	// Original ECU is disabling injector (Injector valve will close and gasoline stops, after Timer1 expires)
	else{
		delta_inj_tick = INJ_exec_time_start - inj_start_tick_last; // delta injetion time, in Timer0 tiks (4 us)
		if (delta_inj_tick > INJ_OPENING_TIME_TICKS) { // remove the opening time from the time measured
			delta_inj_tick -= INJ_OPENING_TIME_TICKS; // real injection time (removed the opening time)
		}
		else {
			delta_inj_tick = 0; // if time is lower than estimated injector opening time, real injection time is considered 0
		}
		bool extension_timer_activated = false;
		if ((delta_inj_tick >= INJ_TIME_TICKS_MIN) && (delta_inj_tick <= INJ_TIME_TICKS_MAX)) { // injection range check
			uint8_t index_thr_tmp = (uint8_t)((throttle >> 6)) & 0x0F; // 0x0F -> to make sure that the index is not over 15 (INJ_INCR_THR_MAPS_SIZE - 1)
			perc_inc+=incrementi_thr[index_thr_tmp];
			extension_time_ticks = (uint16_t)(((uint32_t)delta_inj_tick * (uint32_t)perc_inc) >> (8 + 1 - 3)); // calculates extension time, in Timer1 ticks (0.5us)
			if ((extension_time_ticks >= MIN_EXTENSION_TIME_TICKS) && (extension_time_ticks <= MAX_EXTENSION_TIME_TICKS)){
				Timer1.setPeriod(extension_time_ticks); // setta il timer
				Timer1.attachInterrupt(deactivate_inj); // attacca l'interrupt
				Timer1.start(); // fa partire il timer
				extension_timer_activated = true; // timer has been activated
			}
		}
		if (extension_timer_activated == false){ // timer was not activated, so need to disable the output now
			digitalWrite(OUT_INJ_PIN, LOW); // shuts down injector
			INJ_safety_inj_turned_off = true; // Injector turned OFF (this flag is used for Safety check)
			INJmgr.update_info_for_logger();
		}
	}
	
	// For execution time measurement
	uint16_t INJ_exec_time_end = INJmgr.Timer0_tick_counts(); // 1 tick = 4us
	uint16_t INJ_exec_time = INJ_exec_time_end - INJ_exec_time_start; // Measure interrupt execution time
	if (INJ_signal_status){
		INJ_exec_time_1 = INJ_exec_time; // ON
	}else{
		INJ_exec_time_2 = INJ_exec_time; // OFF
	}
}

INJmgr_class INJmgr;

#endif