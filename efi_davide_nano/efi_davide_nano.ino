// Created by Davide Cavaliere
// E-mail: dadez87@gmail.com
// Website: www.monocilindro.com
// 26 October 2016
// Fuelino SW1.00 beta1
// Compiles successfully with Arduino IDE 1.6.9

#include "EEPROMmgr/EEPROMmgr.h" // EEPROM memory management
#include "INJmgr/INJmgr.h" // Injection and physical input/outputs management
#include "COMMmgr/COMMmgr.h" // Communication management
#include "SDmgr/SDmgr.h" // SD memory management

void setup(){
  COMM_begin(); // Initializes communication
  EEPROM_load_calib(); // EEPROM memory check
  SDmgr.begin(); // Initializes SD memory
  INJmgr.begin(); // Injection and signal IO management
}

void loop() {

  INJmgr.analog_digital_signals_acquisition(); // Acquires throttle position sensor signal
  INJmgr.safety_check(); // Safety checks (checks if, from last function call, the injector has been deactivated at least one time)

  SDmgr.log_engine_info();
  COMM_receive_check();
   
  delay(100); // delay ms

}
