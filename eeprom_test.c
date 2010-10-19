#include <avr/eeprom.h>
#include <avr/io.h>

uint8_t   ee8  EEMEM = 0x77;
uint64_t  ee64 EEMEM = 0x88;

void setup() {

    uint8_t eebuffer;

    Serial.begin(9600);

    Serial.println("Testing 8-bit EEPROM pointer");

    for(i=0; i<8; i++){
        eebuffer = eeprom_read_byte(&(ee8+i));
        Serial.print("0x");
        eeprom_busy_wait();
        Serial.print(eebuffer,HEX);	
    }

    eebuffer = 0x10;    

    Serial.println("Testing write-to 8bit EEPROM pointer");

    for(i=0; i<8; i++){
        eeprom_write_byte(&(ee8+i), eebuffer);
        eebuffer++;
        eeprom_busy_wait();
    }

    for(i=0; i<8; i++){
        foreebuffer = eeprom_read_byte(&
        
}
