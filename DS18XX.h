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
// DS family codes
#define FAMILY_DS1820       0x10

//---------------------------------------------------
// One-Wire bus set macros

/*
 usage: ow_set_bus(&PIND,&PORTD,&DDRD,PD6);
 */
uint8_t OW_PIN_MASK; 
volatile uint8_t* OW_IN;
volatile uint8_t* OW_OUT;
volatile uint8_t* OW_DDR;

#define OW_GET_IN()   ( *OW_IN & OW_PIN_MASK )
#define OW_OUT_LOW()  ( *OW_OUT &= (uint8_t) ~OW_PIN_MASK )
#define OW_OUT_HIGH() ( *OW_OUT |= (uint8_t)  OW_PIN_MASK )
#define OW_DIR_IN()   ( *OW_DDR &= (uint8_t) ~OW_PIN_MASK )
#define OW_DIR_OUT()  ( *OW_DDR |= (uint8_t)  OW_PIN_MASK )

void ow_set_bus(volatile uint8_t* in,
				volatile uint8_t* out,
				volatile uint8_t* ddr,
				uint8_t pin)
{
	OW_DDR=ddr;
	OW_OUT=out;
	OW_IN=in;
	OW_PIN_MASK = (1 << pin);
	//ow_reset(); handle this manually
}

//---------------------------------------------------
// One-Wire timer set macros (use any 8 bit timer)

// this uses timer/counter 0
#define OW_TCCRA    TCCR0A
#define OW_TCCRB    TCCR0B
#define OW_TCNT     TCNT0
#define OW_OCRA     OCR0A

void ow_timer_init();
//init2ialize and make sure all is okay

void ow_timer_set(uint8_t prescaler, uint8_t compare_match_value);
// Use 8 bit timer, settings as follows
// Assuming F_OSC = 16000000UL (16MHz)
// Normal operation:
//   TCCR0A = 0x02; /* turn off compare match output modules, set to
//                   set to clear on compare register match */
//   TCCR0B = 0x02; /* use clock-source F_OSC/8 : t_clk = 0.5us */
// Reset Pulse:
//   TCCR0B = 0x03 /* clock-source F_OSC/64 : t_clk = 4us */
// Wait 10ms for conversions:
//   use delay library? or not...
// 
// this funct should be called to reset the clock for each communication
// if prescaller != 0, set the prescaler and reset the timer, else leave the clock running
// if compare value is set, update the value as well, if 0 turn it off

//---------------------------------------------------
uint8_t ow_reset();
//use 
//pull low for 480us to reset devices
//return 1 if no presence pulse detected
//else 0 if all okay

//---------------------------------------------------
#ifndef NUMBER_OF_SENSORS
#define NUMBER_OF_SENSORS        1
#endif

uint8_t ow_search_roms(uint8_t n, uint8_t ee_store[]);
//execute SEARCH_ROM -> store memory in EEPROM @ ee_store
//if n = 1 use READ_ROM command 
//if n > 1 : search again, if rom is unique -> store to next EEPROM address
//if n > 2 ...
//needs to call crc on ROM values
//return 0 if all sensors found, 1 if error

//---------------------------------------------------
void ow_write_byte(uint8_t ow_command);
//start 8 bit loop
//add pulldown 1us init
//add hold nth bit for 60-120us
//add 1 us (min) recovery time
//loop back for next bit

//---------------------------------------------------
uint8_t ow_read_byte();
// uint8_t read_byte;
// start 8 bit loop
// add master pull low 1us min to init
// release and sample for/within 15us 
// keep released for rest of the 60 us
// add 1 us (min) recovery time
// loop back for next bit
// return read_byte;

//---------------------------------------------------
uint8_t ow_read_scratchpad(uint8_t* r_byte);
//use pointer and write 8 bits at a time, avoids having to alloc two 64-bit blocks of RAM
//use (r_byte++)* = ow_read_byte(), check sd card stuff for syntax
//return 0 if successful, 1 otherwise

//---------------------------------------------------
float ow_ds18xx_convert_temp(uint16_t raw_temp);
// take the temperature, write it out in deg C accurate to resolution of device 

//---------------------------------------------------
float ow_read_temp();
// reset pulse + wait for presence pulse
// ow_write(MATCH_ROM);
// for byte in rom_code(&ow_write byte) //figure out how to point to EEPROM
// uint64_t = ow_read_scratchpad() - should handle CRC on it's own, reread if bad
// return ds18xx_convert_temp()


