/* Library for use with Dallas / Maxim DS1820, DS1822
 * family of 1-Wire Digital Thermometers
 *
 * ToDos: -many, many things
 *        -consider splitting into seperate .c and .h
 *        -set up interrupt servidce routines 
 *        -set up periodical temp check on other timer?
 *        -set up CRC function
 *        -set up wdt to prevent hangups, need a special routine for handling WDT resets?
 *  
 */


#include <avr/interrupt.h>


//---------------------------------------------------
// ROM Command Definitions:
#define OW_SEARCH_ROM          0xF0
#define OW_READ_ROM            0x33
#define OW_MATCH_ROM           0x55
#define OW_SKIP_ROM            0xCC
#define OW_ALARM_SEARCH        0xEC
// FUNCTION Commands:
#define OW_CONVERT_T           0x44
#define OW_WRITE_SCRATCHPAD    0x4E
#define OW_READ_SCRATCHPAD     0xBE
#define OW_COPY_SCRATCHPAD     0x48
#define OW_RECALL_EE           0xB8
#define OW_READ_POWER_SUPPLY   0xB4
// DS family codes
#define FAMILY_DS1820       0x10

//---------------------------------------------------
// One-Wire bus set macros

/* usage: ow_set_bus(&PIND,&PORTD,&DDRD,PD6); */
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
// one-wire interrupt and routine flags buffer byte
volatile uint8_t ow_flags = 0x00;

#define OW_TOV  0
#define OW_OCF  1
#define OW_RESET_FLAG 2
#define OW_READ_FLAG 3

#define OW_GET_FLAG(x) ((ow_flags>>x) & 0x01)
#define OW_SET_FLAG(x) ow_flags |= ((uint8_t)  (1<<x))
#define OW_CLR_FLAG(x) ow_flags &= ((uint8_t) ~(1<<x))

#define OW_WAIT_UNTIL_SET(x)     while(!OW_GET_FLAG(x)) ; \
                                 OW_CLR_FLAG(x)

//---------------------------------------------------
// One-Wire timer set macros (use any 8 bit timer)

#ifndef OW_TIMER
#define OW_TIMER 0
#endif

// might need a timer initialization proper

//add more support for different xtals as it occurs
#ifndef F_OSC
#define F_OSC 16000000UL
#endif


#if OW_TIMER == 2
// this uses timer/counter 2
#define OW_TCCRA    TCCR2A
#define OW_TCCRB    TCCR2B
#define OW_TCNT     TCNT2
#define OW_OCRA     OCR2A
#define OW_TIMSK    TIMSK2

#else
// this uses timer/counter 0
#define OW_TCCRA    TCCR0A
#define OW_TCCRB    TCCR0B
#define OW_TCNT     TCNT0
#define OW_OCRA     OCR0A
#define OW_TIMSK    TIMSK0
#endif

#define OW_RESET_T (uint8_t)(480/((1000000UL*64)/F_OSC)) //480 us reset pulse
#define OW_PRESENCE_WAIT_T (uint8_t)(20/((1000000UL*64)/F_OSC)) //20 us wait before read presence pulse
#define OW_PRESENCE_SAMPLE_T (uint8_t)(480/((1000000UL*64)/F_OSC)) //20 us wait before read presence pulse
#define OW_1US_T (uint8_t)(1/((1000000UL*8)/F_OSC)) // 1 us delay for init read or pull-up settle time
#define OW_WRITE_SLOT_T (uint8_t)(60/((1000000UL*8)/F_OSC)) //60 us timer for write slot
#define OW_READ_SAMPLE_T (uint8_t)(15/((1000000UL*8)/F_OSC)) //15 us timer for read sample time
#define OW_READ_SLOT_T (uint8_t)(60/((1000000UL*8)/F_OSC)) //60 us timer for read slot
#define OW_WAIT_T (uint8_t)(10/((1000UL*1024)/F_OSC)) //wait 10 ms for the ds1820 to convert the temp


//stop the timer and reset,do this in the overflow interrupt routines, let another macro start it up when it's ready
#define OW_TIMER_STOP() \
          OW_TCCRB = 0x00;\
          OW_TCNT = 0x00; \
          OW_TIMSK = 0x01
//Keeps the timer running, resets back to CTC functionality with overflow at x
//***should only be used if the prescaler remains unchanged***
#define OW_TIMER_SPLIT(x) \
          OW_TIMSK = 0x01; \
          OW_TCCRA = 0x02; \
          OW_OCRA  = x 
// turn off compare match output modules, set to set to clear(overflow) on compare register match, clock-source F_OSC/64 : t_clk = 4us
#define OW_TIMER_RESET_PULSE() \
		  OW_TCCRA = 0x02; \
          OW_OCRA = OW_RESET_T; \
		  OW_TCCRB = 0x03
//similar to ow_timer_read_slot, use split
#define OW_TIMER_PRESENCE_WAIT() \
		  OW_TCCRA = 0x00; \
          OW_OCRA = OW_PRESENCE_WAIT_T; \
		  OW_TCCRB = 0x03; \
          OW_TIMSK = 0x03

// turn off compare match output modules, set to set to clear(overflow) on compare register match, use clock-source F_OSC/8 : t_clk = 0.5us
#define OW_TIMER_1US_DELAY() \
          OW_TCCRA = 0x02; \
          OW_OCRA = OW_1US_T; \
          OW_TCCRB = 0x02
//write value will be held on output for the duration of the write slot
#define OW_TIMER_WRITE_SLOT() \
          OW_TCCRA = 0x02; \
          OW_OCRA = OW_WRITE_SLOT_T; \
          OW_TCCRB = 0x02
//use normal operation, but enable interrupts on OCR match, timer keeps running on OCR interrupt, but call OW_TIMER_SPLIT(hex(120))
#define OW_TIMER_READ_SLOT() \
          OW_TCCRA = 0x00; \
          OW_OCRA = OW_READ_SLOT_T; \
          OW_TCCRB = 0x02; \
          OW_TIMSK = 0x03

#define OW_TIMER_SLEEP()

#define OW_TIMER_WAIT() \
          OW_TCCRA = 0x02; \
          OW_OCRA = OW_WAIT_T; \
		  OW_TCCRB = 0x05

// this funct should be called to reset the clock for each communication
// if prescaller != 0, set the prescaler and reset the timer, else leave the clock running
// if compare value is set, update the value as well, if 0 turn it off


//---------------------------------------------------
//one-wire reset function, pulls line low 480us, releases and waits for presence
//returns 0 is presence pulse detected, returns 1 otherwise
uint8_t ow_reset()
{
    uint8_t presence = 0;
    OW_SET_FLAG(OW_RESET_FLAG);
    OW_OUT_LOW();
    OW_DIR_OUT();
    OW_TIMER_RESET_PULSE();
    OW_WAIT_UNTIL_SET(OW_TOV) ; //wait for the reset pulse to complete
    OW_DIR_IN();
    OW_TIMER_PRESENCE_WAIT();
    OW_WAIT_UNTIL_SET(OW_OCF); //wait for the presence pulse sample time
    while(!OW_GET_FLAG(OW_TOV))
        if(!presence) presence = !OW_GET_IN();
    ow_flags &= ~(0x03); //tidy up the flags when you're done
    if(presence) return 0; //got the pulse
    return 1;
}

//---------------------------------------------------
uint8_t ow_compute_crc(uint8_t r_byte[], uint8_t num_bytes);
//figure out the 

//---------------------------------------------------
#ifndef NUMBER_OF_SENSORS
#define NUMBER_OF_SENSORS        1
#endif

#define MAX_SENSORS              4    // 8 byte ROM code * MAX_SENSORS <= EEPROM size 

uint8_t ow_search_roms(uint8_t n, uint8_t ee_store[]);
//execute SEARCH_ROM -> store memory in EEPROM @ ee_store
//if n = 1 use READ_ROM command 
//if n > 1 : search again, if rom is unique -> store to next EEPROM address
//if n > 2 ...
//feed read bytes into 7x8bit array and seperate read_crc
//needs to check crc(7x8bit array) == readcrc
//then store in EEPROM is unique and crc checks out
//return 0 if all sensors found, 1 if error

//---------------------------------------------------
void ow_write_byte(uint8_t put_byte){
    for(unit8_t bit = 8; bit > 0; bit--){
        
    } 
}

//start 8 bit loop
//add pulldown 1us init
//add hold nth bit for 60-120us
//add 1 us (min) recovery time
//loop back for next bit

//---------------------------------------------------

//sequence for reading a byte from a one-wire device, returns a read byte
//reset pulse must be called prior, call as needed to read the whole scratchpad

uint8_t ow_read_byte(){
    uint8_t read_byte = 0x00;
    uint8_t read_bit = 1;
    for(unit8_t bit = 0; bit < 8; bit--){
        OW_OUT_LOW();
        OW_DIR_OUT(); //pull line low
        OW_TIMER_1US_DELAY();
        OW_WAIT_UNTIL_SET(OW_TOV);
        OW_DIR_IN();  //let go of the line
        OW_SET_FLAG(OW_READ_FLAG); //make sure ISR(OCR_OV) clears this flag
        OW_TIMER_READ_SLOT();
        while(!OW_GET_FLAG(OW_OCF))
            read_bit = OW_GET_IN();
        read_byte = readbyte | read_bit<<bit;
        OW_WAIT_UNTIL_SET(OW_TOV);
        OW_CLR_FLAG(OW_OCF); 
    }
    return read_byte; 
}

//---------------------------------------------------

uint8_t ow_read_scratchpad(uint8_t* r_byte);
//use pointer and write 8 bits at a time, avoids having to alloc two 64-bit blocks of RAM
//use (r_byte++)* = ow_read_byte(), check sd card stuff for syntax
//return 0 if successful, 1 otherwise

//---------------------------------------------------

float ow_ds18xx_convert_temp(uint16_t raw_temp){
    // take the temperature, write it out in deg C accurate to resolution of device 
    // only accurate for DS1820 at the moment
    
    if(raw_temp & 0xF000)
        //number is negative
        return (((1<<8)-(raw_temp & 0x00FF)) * (-0.5)) ;
    else
        return ((raw_temp & 0x00FF) * (-0.5)) ;

//---------------------------------------------------

#define OW_ALL_SENSORS    0x00
#define OW_SENSOR_1       1
#define OW_SENSOR_2       (1<<1)
#define OW_SENSOR_3       (1<<2)
#define OW_SENSOR_4       (1<<3)

//Read the temperature from specified sensors, returns 0 if successful, writes to a float array of size 4
// Usage: ow_read_temp(OW_SENSOR_1|OW_SENSOR_3, &float_array[]) 

unit8_t ow_read_temp(uint8_t sensor_mask, float* store_temp[]){ 
  //receive number of sensors to convert using the mask, return temps to pointer array 
    while(!ow_reset()) ; //wait for reset to return without an error
    
    sensor_mask &= 0x0F;
    
    if(sensor_mask == OW_ALL_SENSORS || sensor_mask != OW_SENSOR_1 && sensor_mask != OW_SENSOR_2 && sensor_mask != OW_SENSOR_3 && sensor_mask != OW_SENSOR_4)
        ow_write_byte(OW_SKIP_ROM);
    else
        ow_write_byte(OW_MATCH_ROM);
        //write the ROM code stored in EEPROM
    
    ow_write_byte(OW_CONVERT_T);
    
    OW_TIMER_WAIT();
    OW_WAIT_UNTIL_SET(OW_TOV);

    uint8_t done_yet = 0;
    while(!done_yet) //poll devices to see if done
    {
        OW_OUT_LOW();
        OW_DIR_OUT(); //pull line low
        OW_TIMER_1US_DELAY();
        OW_WAIT_UNTIL_SET(OW_TOV);
        OW_DIR_IN();  //let go of the line
        OW_TIMER_READ_SLOT();
        while(!OW_GET_FLAG(OW_OCF))
            done_yet = OW_GET_IN(); //ds1820 will return a 1 when the temp is converted
        OW_WAIT_UNTIL_SET(OW_TOV);
        OW_CLR_FLAG(OW_OCF); 
    }
    //read the scratchpads one at a time, store a 0 in the array if the sensor was not called.
}



