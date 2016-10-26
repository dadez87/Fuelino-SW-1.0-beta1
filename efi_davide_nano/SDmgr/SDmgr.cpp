#ifndef SDmgr_cpp
#define SDmgr_cpp

#include "SDmgr.h"


SDmgr_class::SDmgr_class(){
	
	SD_init_OK = false; // NG
	SD_writing_buffer_size = 0;
	
}


bool SDmgr_class::begin(){
	
#if SD_MODULE_PRESENT
			
	// SD INITIALIZATION
	if (!SD.begin(SD_CS_PIN_NUM)) { // CS pin for SD card is pin #10
		SD_init_OK = false; // NG
		return false;
	}
	SD_init_OK = true; // OK

	// Read file name from EEPROM and stores file name as string (8.3 format)
	uint16_t file_number = EEPROM_SD_file_num_rw();
	file_name="fln";
	if (file_number < 10) file_name+='0';
	if (file_number < 100) file_name+='0';
	if (file_number < 1000) file_name+='0';
	if (file_number < 10000) file_name+='0';
	file_name+=(unsigned int)file_number;
	file_name+=".log";
	
#endif
	
	return true;
	
}


bool SDmgr_class::log_engine_info(){

	// Preparation of data array
	uint8_t i=0; // bytes counter
	SD_writing_buffer[i++] = 'd';
	SD_writing_buffer[i++] = '1'; // next bytes (payload data)
	unsigned long time_stamp_temp = millis(); // read time stamp
	SD_writing_buffer[i++] = (uint8_t)(time_stamp_temp & 0xff); // LSB
    SD_writing_buffer[i++] = (uint8_t)((time_stamp_temp >> 8) & 0xff);
    SD_writing_buffer[i++] = (uint8_t)((time_stamp_temp >> 16) & 0xff);
    SD_writing_buffer[i++] = (uint8_t)((time_stamp_temp >> 24) & 0xff); // MSB
	buffer_busy = 1; // Locks the buffer access (semaphore)
	if (SIMULATION_SIGNAL) { // Signals are simulated
		injection_counter_buffer++;
		delta_time_tick_buffer = 50000;
		delta_inj_tick_buffer = 5000;
		throttle_buffer = 300;
    }
	SD_writing_buffer[i++] = (uint8_t)(injection_counter_buffer & 0xff); // LSB
	SD_writing_buffer[i++] = (uint8_t)((injection_counter_buffer >> 8) & 0xff); // MSB
    SD_writing_buffer[i++] = (uint8_t)(delta_time_tick_buffer & 0xff); // LSB
    SD_writing_buffer[i++] = (uint8_t)((delta_time_tick_buffer >> 8) & 0xff); // MSB
    SD_writing_buffer[i++] = (uint8_t)(delta_inj_tick_buffer & 0xff); // LSB
    SD_writing_buffer[i++] = (uint8_t)((delta_inj_tick_buffer >> 8) & 0xff); // MSB
    SD_writing_buffer[i++] = (uint8_t)(throttle_buffer & 0xff); // LSB
    SD_writing_buffer[i++] = (uint8_t)((throttle_buffer >> 8) & 0xff); // MSB
	SD_writing_buffer[i++] = (uint8_t)(lambda_buffer & 0xff); // LSB
    SD_writing_buffer[i++] = (uint8_t)((lambda_buffer >> 8) & 0xff); // MSB
	SD_writing_buffer[i++] = (uint8_t)(extension_time_ticks_buffer & 0xff); // LSB
	SD_writing_buffer[i++] = (uint8_t)((extension_time_ticks_buffer >> 8) & 0xff); // MSB
	SD_writing_buffer[i++] = (uint8_t)(INJ_exec_time_1_buffer & 0xff); // LSB
	SD_writing_buffer[i++] = (uint8_t)((INJ_exec_time_1_buffer >> 8) & 0xff); // MSB
	SD_writing_buffer[i++] = (uint8_t)(INJ_exec_time_2_buffer & 0xff); // LSB
	SD_writing_buffer[i++] = (uint8_t)((INJ_exec_time_2_buffer >> 8) & 0xff); // MSB
	//delta_time_tick_buffer = 0; // reset, to understand if ECU has updated this field or not (example: when braking, this field is not updated)
    //delta_inj_tick_buffer = 0; // reset, to understand if ECU has updated this field or not (example: when braking, this field is not updated)
    //throttle_buffer = 0; // reset, to understand if ECU has updated this field or not (example: when braking, this field is not updated)
	//lambda_buffer = 0; // reset 
	//extension_time_ticks_buffer=0; // reset
	//INJ_exec_time_1_buffer = 0; // reset
	//INJ_exec_time_2_buffer = 0; // reset
	buffer_busy = 0; // Opens the buffer again
    uint16_t CK_SUM = COMM_calculate_checksum(SD_writing_buffer, 0, i);
    SD_writing_buffer[i++] = (uint8_t)(CK_SUM >> 8);
    SD_writing_buffer[i++] = (uint8_t)(CK_SUM & 0xFF);
	SD_writing_buffer_size = i;

#if SD_MODULE_PRESENT
	// Writing the data on SD memory
	if (SD_init_OK == false) return false; // SD was not initialized
	File dataFile = SD.open(file_name, FILE_WRITE);
	if (!dataFile) return false; // not possible to perform the log
	dataFile.write(SD_writing_buffer, SD_writing_buffer_size); // SD_writing_buffer_size
	dataFile.close(); // finished to write the file
#endif
	
	return true;
}

SDmgr_class SDmgr;

#endif