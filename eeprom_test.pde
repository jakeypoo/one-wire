#include <avr/eeprom.h>
#include <avr/io.h>
#include <avr/interrupt.h>

uint8_t   ee8  EEMEM = 0x77;
uint32_t  ee32 EEMEM = 0x88;
uint8_t   ee8g EEMEM = 0x88;

void setup() {

    uint8_t eebuffer;

    Serial.begin(9600);

    Serial.println("Testing 8-bit EEPROM pointer");

    for(uint8_t i=0; i<8; i++){
        eebuffer = eeprom_read_byte(&ee8+i);
        Serial.print("0x");
        eeprom_busy_wait();
        Serial.println(eebuffer,HEX);	
    }

    eebuffer = 0x10;    

    Serial.println("Testing write-to 8bit EEPROM pointer");

    for(uint8_t i=0; i<8; i++){
        eeprom_write_byte(&ee8+i, eebuffer);
        eebuffer++;
        eeprom_busy_wait();
    }

    for(uint8_t i=0; i<8; i++){
        eebuffer = eeprom_read_byte(&ee8+i);
        Serial.print("0x");
        eeprom_busy_wait();
        Serial.println(eebuffer,HEX);	
    }    
    
    for(uint8_t i=0; i<8; i++){
        eeprom_write_byte(&ee8+i, 0xFF);
       	eeprom_busy_wait();
    }  
   
    Serial.println("Testing 32-bit EEPROM pointer");
    
    uint32_t hello_eeprom = 0x11223344;

    eeprom_update_block(&hello_eeprom, &ee32, 4);

    hello_eeprom = 0;

    eeprom_busy_wait();
    eeprom_read_block(&ee32, &hello_eeprom, 4);

    Serial.print("0x");
    Serial.println(hello_eeprom, HEX);
     
    Serial.println("Testing read/write 64-bit alloc w/ 8-bit pointer");
    
    eebuffer = 0xA1;    

    for(uint8_t i=1; i<4; i+=2){
        eeprom_write_byte(&ee8+i, eebuffer);
        eebuffer++;
        eeprom_busy_wait();
    }   
    eeprom_read_block(&ee32, &hello_eeprom, 4);

    Serial.print("0x");
    Serial.println(hello_eeprom, HEX);
     
}


void loop(){
 ; 
}
