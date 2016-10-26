#ifndef INJmgr_h
#define INJmgr_h

#include <Arduino.h>
#include "../compile_options.h"

#define INJ_TIME_TICKS_MIN (uint16_t)100 // minima iniezione 400us (1 Timer0 tick = 4us)
#define INJ_TIME_TICKS_MAX (uint16_t)2000 // massima iniezione 8ms = 8000us (1 Timer0 tick = 4us)

#if FUELINO_HW_VERSION == 1
	#define IN_INJ_PIN 2 // input pin injector from ECU
	#define OUT_INJ_PIN 9 // output pin injector to physical injector
	#define THROTTLE_PIN A0 // throttle analog signal input pin
	#define LAMBDA_PIN A0 // no lambda sensor present on Fuelino V1
#elif FUELINO_HW_VERSION == 2
	#define IN_INJ_PIN 6 // input pin injector from ECU
	#define OUT_INJ_PIN 8 // output pin injector to physical injector
	#define THROTTLE_PIN A2 // throttle analog signal input pin
	#define LAMBDA_PIN A3 // lambda sensor signal input pin
#endif

#define MAX_PERCENTAGE_INJ (uint16_t)205 // maximum increment percentage (for safety) [205 corresponds to about 40% increment]
#define MIN_EXTENSION_TIME_TICKS (uint16_t)100 // minimum time of Timer1 extension time ticks (0.5us), for injection time extension [100 = 50us]
#define MAX_EXTENSION_TIME_TICKS (uint16_t)6000 // minimum time of Timer1 extension time ticks (0.5us), for injection time extension [6000 = 3000us]
#define INJ_OPENING_TIME_TICKS (uint16_t)0 // time ticks (4us) required for the injector to open
#define INJ_INCREMENT_RPM_STD (uint8_t)128 // Injection increment standard (50/256 %)
#define INJ_INCREMENT_THR_STD (uint8_t)0 // Injection increment standard (50/256 %)
#define INJ_INCREMENT_TIM_STD (uint8_t)0 // Injection increment standard (50/256 %)
#define INJ_INCR_RPM_MAPS_SIZE 8 // Engine Speed compensation map size
#define INJ_INCR_THR_MAPS_SIZE 16 // Throttle compensation map size [if you change this, you should also change "index_thr_tmp" calculation]
#define INJ_INCR_TIM_MAPS_SIZE 8 // Injection Time compensation map size
#define INJ_MAPS_TOTAL_NUM 3 // total number of calibration maps
#define INJ_SAFETY_MAX_ERR (uint8_t)2 // maximum number of tolerable Safety errors, then turn OFF the injector

// Variables to be exported
extern uint8_t incrementi_rpm[]; // increments depending on rpm
extern uint8_t incrementi_thr[]; // increments depending on throttle
extern uint8_t incrementi_tim[]; // increments depending on injection time
extern volatile uint16_t injection_counter_buffer; // counts the combustion cycles
extern volatile uint16_t delta_time_tick_buffer; // time between 2 consecutive injections (2rpm) buffer [1 tick = 4us]
extern volatile uint16_t delta_inj_tick_buffer; // injection time buffer [1 tick = 4us]
extern volatile uint16_t throttle_buffer; // voltage on throttle pin 0-1023 buffer
extern volatile uint16_t lambda_buffer; // lambda sensor voltage
extern volatile uint16_t extension_time_ticks_buffer; // extension time Timer1 ticks (0.5us)
extern volatile uint16_t INJ_exec_time_1_buffer; // execution time for the interrupt
extern volatile uint16_t INJ_exec_time_2_buffer; // execution time for the interrupt
extern volatile uint8_t buffer_busy; // activated when SD packet (and serial packet) are built, to avoid corruption

class INJmgr_class{
	
	public:
		void begin(); // Called in MAIN Setup
		uint16_t Timer0_tick_counts();
		uint8_t interpolate_map(uint16_t* breakpoints_array, bool brkpts_incr, uint8_t* calib_array, uint8_t arrays_size, uint16_t x_value);
		void update_info_for_logger();
		void analog_digital_signals_acquisition();
		void safety_check();
		
		uint8_t INJ_safety_err_counter; // Safety error counter
		
};

extern INJmgr_class INJmgr;

#endif