/* Library for use with Dallas / Maxim DS1820, DS1822
 * family of 1-Wire Digital Thermometers
 *
 * ToDos: -many, many things
 *        -consider splitting into seperate .c and .h
 *        -add posibility for the read/write routines to be interruptable
 */

#include <avr/interrupt.h>


//---------------------------------------------------
// ROM Command Definitions:
#define SEARCH_ROM          0xF0
#define READ_ROM            0x33
#define MATCH_ROM           0x55
#define SKIP_ROM            0xCC
#define ALARM_SEARCH        0xEC
// FUNCTION Commands:
#define CONVERT_T           0x44
#define WRITE_SCRATCHPAD    0x4E
#define READ_SCRATCHPAD     0xBE
#define COPY_SCRATCHPAD     0x48
#define RECALL_EE           0xB8
#define READ_POWER_SUPPLY   0xB4

//---------------------------------------------------
// Use 8 bit timer, settings as follows
//TCCT0A =  


//---------------------------------------------------
void reset_pulse(){
//
}

//---------------------------------------------------
void ow_write(uint8_t ow_command){
//start 8 bit loop
//add pulldown 1us init
//add hold nth bit for 60-120us
//add 1 us (min) recovery time
//loop back for next bit
}

//---------------------------------------------------
uint8_t ow_read_byte(){
  uint8_t read_byte;
//start 8 bit loop
//add master pull low 1us min to init
//release and sample for/within 15us 
//keep released for rest of the 60 us
//add 1 us (min) recovery time
//loop back for next bit
  return read_byte;
}

//---------------------------------------------------
float read_temp(uint64_t ROM_code){
//reset pulse + wait for presence pulse
  ow_write(MATCH_ROM);
//for byte in rom_code(ow_write byte)
//uint64_t = read_scratchpad - should handle CRC on it's own, reread if bad
//
} 

