#ifndef EEPROMmgr_h
#define EEPROMmgr_h

#include <EEPROM.h>
#include <Arduino.h>
#include "../COMMmgr/COMMmgr.h"
#include "../INJmgr/INJmgr.h"

// EEPROM memory addresses table:
#define INJ_INCR_RPM_MAPS_START 0 // start address (0)
#define INJ_INCR_THR_MAPS_START 2*INJ_INCR_RPM_MAPS_SIZE // start address (16)
#define INJ_INCR_TIM_MAPS_START 2*(INJ_INCR_RPM_MAPS_SIZE+INJ_INCR_THR_MAPS_SIZE) // start address (48)
#define SD_FILE_NUM_ADDR 128 // address of file number (4 bytes used)

// Functions of other modules to be imported
extern void send_8_byte_string_to_serial(String input_string);

// Functions of EEPROMmgr to be exported
extern uint8_t EEPROM_load_calib();
extern uint8_t EEPROM_read();
extern uint8_t EEPROM_write_standard_values(uint8_t data_number);
extern uint16_t EEPROM_SD_file_num_rw();
extern void EEPROM_write_RAM_map_to_EEPROM(uint8_t map_number_req);

#endif