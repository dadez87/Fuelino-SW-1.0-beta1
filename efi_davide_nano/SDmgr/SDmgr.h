#ifndef SDmgr_h
#define SDmgr_h

#include <SD.h> // needed to manage SD card
#include "../EEPROMmgr/EEPROMmgr.h" // Need to manage reading from EEPROM (file name, ...)
#include "../COMMmgr/COMMmgr.h" // Communication manager (to send serial messages, ...)
#include "../INJmgr/INJmgr.h" // INJ manager. To have access to variables (inj time, ...)

#define SD_CS_PIN_NUM 10 // CS pin for SD card (SPI) - Slave Select
#define SD_WRITE_BUFFER_SIZE 32 // Size of buffer for SD writing
#define SIMULATION_SIGNAL 0 // enables signal simulation

class SDmgr_class
{
  public:
    String file_name; // SD file name where to log data
	bool SD_init_OK; // if SD card was initialized correctly
	uint8_t SD_writing_buffer[SD_WRITE_BUFFER_SIZE]; // buffer for SD writing
	uint8_t SD_writing_buffer_size;
	
	SDmgr_class();
	bool begin();
	bool log_engine_info();

};

extern SDmgr_class SDmgr;

#endif