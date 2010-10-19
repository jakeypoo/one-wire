#include <EEPROM.h>
#include <avr/eeprom.h>
#include <avr/io.h>
#include <avr/interrupt.h>

uint8_t   ee8  EEMEM = 0x77;
uint64_t  ee64 EEMEM = 0x88;

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
   
//    Serial.println("Testing 64-bit EEPROM pointer");
//
//    for(uint8_t i=0; i<8; i++){
//        eebuffer = eeprom_read_byte(&ee64+i);
//        Serial.print("0x");
//        eeprom_busy_wait();
//        Serial.print(eebuffer,HEX);	
//    }
//
//    eebuffer = 0x10;    
//
//    Serial.println("Testing write-to 64bit EEPROM pointer");
//
//    for(uint8_t i=0; i<8; i++){
//        eeprom_write_byte(&ee64+i, eebuffer);
//        eebuffer++;
//        eeprom_busy_wait();
//    }
//
//    for(uint8_t i=0; i<8; i++){
//        eebuffer = eeprom_read_byte(&ee64+i);
//        Serial.print("0x");
//        eeprom_busy_wait();
//        Serial.print(eebuffer,HEX);	
//    }    
}

void loop(){
 ; 
}
