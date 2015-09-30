#include <Flash.h>
#include <avr/boot.h>
#include <EEPROM.h>
#include "cpuname.h"

/*
* SIGRD is a "must be zero" bit on most Arduino CPUs; can we read the sig or not?
*/
#if (!defined(SIGRD))
#define SIGRD 5
#endif

unsigned char fuse_bits_low;
byte fuse_bits_extended;
byte fuse_bits_high;
byte lock_bits;
byte sig1, sig2, sig3;
byte oscal;

void setup()
{
 unsigned char xxx = fuse_bits_low>>4;
 Serial.begin(115200);
 Serial.print(xxx);
}

void space() {
 Serial.print(' ');
}

void print_serno(void)
{
 int i;
 int unoSerial[6];
 int startAddr=1018;
 unsigned long serno = 0;

 for (i = 0; i < 6; i++) {
   unoSerial[i] = EEPROM.read(startAddr + i);
 }
 if (unoSerial[0] == 'U' && unoSerial[1] == 'N' && unoSerial[2] == 'O') {

   Serial << F("Your Serial Number is: UNO");

   for (i = 3; i < 6; i = i + 1) {
     serno = serno*256 + unoSerial[i];
     Serial.print(" ");
     Serial.print(unoSerial[i], HEX);
   }
   Serial << F(" (") << serno << F(")");
 } 
 else {
   Serial << F("No Serial Number");
 }
 Serial.println();
}

void print_binary(byte b)
{
 for (byte i=0x80; i>0; i>>=1) {
   if (b&i) {
     Serial.print('1');
   } 
   else {
     Serial.print('0');
   }
 }
}

/*
* Note that most fuses are active-low, and the the avr include files
* define them as inverted bitmasks...
*/

void print_fuse_low(void)
{
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__) || \
 defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__) ||  \
   defined(__AVR_ATmega168A__) || defined(__AVR_ATmega168PA__)
   /*
* Fuse Low is same on 48/88/168/328
    */
   Serial << F("Fuse Low = ");
 print_binary(fuse_bits_low);
 Serial << F(" (");
 Serial.print(fuse_bits_low, HEX);
 Serial << F(")\n");
 Serial << F("           ||||++++______"); 
 switch (fuse_bits_low & 0xF) {
 case 0: 
   Serial << F("Reserved");
   break;
 case 1: 
   Serial << F("External Clock");
   break;
 case 2: 
   Serial << F("Calibrated 8MHz Internal Clock");
   break;
 case 3: 
   Serial << F("Internal 128kHz Clock");
   break;
 case 4: 
   Serial << F("LF Crystal, 1K CK Startup");
   break;
 case 5: 
   Serial << F("LF Crystal 32K CK Startup");
   break;
 case 6: 
   Serial << F("Full Swing Crystal ");
   break;
 case 7: 
   Serial << F("Full Swing Crystal");
   break;
 case 8:
 case 9:
   Serial << F("Low Power Crystal 0.4 - 0.8MHz");
   break;
 case 10:
 case 11:
   Serial << F("Low Power Crystal 0.9 - 3.0MHz");
   break;
 case 12:
 case 13:
   Serial << F("Low Power Crystal 3 - 8MHz");
   break;
 case 14:
 case 15:
   Serial << F("Low Power Crystal 8 - 16MHz");    
   break;
 }

 Serial.println();
 Serial << F("           ||++__________"); 
 Serial << F("Start Up Time=");
 Serial.print((fuse_bits_low >> 4) & 3, BIN);

 Serial.println();
 Serial << F("           |+____________"); 
 Serial << F("Clock Output ");
 if (fuse_bits_low & (~FUSE_CKOUT))
   Serial << F("Disabled");
 else
   Serial << F("Enabled");

 Serial.println();
 Serial << F("           +_____________");
 if (fuse_bits_low & (~FUSE_CKDIV8)) {
   Serial << F("(no divide)");
 } 
 else {
   Serial << F("Divide Clock by 8");
 }


#elif defined(__AVR_ATmega8__)
#endif
 Serial.println();
}


void print_fuse_high()
{
 Serial << F("\nFuse High = ");
 print_binary(fuse_bits_high);
 Serial << F(" (");
 Serial.print(fuse_bits_high, HEX);
 Serial << F(")\n");

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
 Serial << F("            |||||||+______");
 if (fuse_bits_high & bit(FUSE_BOOTRST)) {
   Serial << F("Reset to Start of memory\n");
 } 
 else {
   Serial << F("Reset to Bootstrap\n");
 }
 Serial << F("            |||||++_______");
 switch ((byte)(fuse_bits_high & ((~FUSE_BOOTSZ1)+(~FUSE_BOOTSZ0)))) {
   case (byte)((~FUSE_BOOTSZ1)+(~FUSE_BOOTSZ0)):
   Serial << F("256 words (512 bytes)\n"); 
   break;
   case (byte)((~FUSE_BOOTSZ1)):
   Serial << F("512 words (1024 bytes)\n"); 
   break;
   case (byte)((~FUSE_BOOTSZ0)):
   Serial << F("1024 words (2048 bytes)\n"); 
   break;
 case 0:
   Serial << F("2048 words (4096 bytes)\n"); 
   break;
 default:
   Serial.println(fuse_bits_high & ((~FUSE_BOOTSZ1)+(~FUSE_BOOTSZ0)), BIN);
 }
#elif defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__) || \
 defined(__AVR_ATmega168A__) || defined(__AVR_ATmega168PA__)
   Serial << F("            |||||+++______");
 switch ((byte)(fuse_bits_high & 7)) {
 case 7:
   Serial << F("Brownout Disabled\n"); 
   break;
 case 6:
   Serial << F("Brownout at 1.8V\n"); 
   break;
 case 5:
   Serial << F("Brownout at 2.7V\n"); 
   break;
 case 4:
   Serial << F("Brownout at 4.3V\n"); 
   break;
 default:    
   Serial << F("Brownout Reserved value");
   Serial.println(fuse_bits_high& 7, BIN);
   break;
 }

#elif defined(__AVR_ATmega8__)
#endif
 Serial << F("            ||||+_________");
 if (fuse_bits_high & ~(FUSE_EESAVE)) {
   Serial << F("EEPROM Erased on chip erase\n");
 } 
 else {
   Serial << F("EEPROM Preserved on chip erase\n");
 }
 Serial << F("            |||+__________");
 if (fuse_bits_high & ~(FUSE_WDTON)) {
   Serial << F("Watchdog programmable\n");
 } 
 else {
   Serial << F("Watchdog always on\n");
 }
 Serial << F("            ||+___________");
 if (fuse_bits_high & ~(FUSE_SPIEN)) {
   Serial << F("ISP programming disabled\n");
 } 
 else {
   Serial << F("ISP programming enabled\n");
 }
 Serial << F("            |+____________");
 if (fuse_bits_high & ~(FUSE_DWEN)) {
   Serial << F("DebugWire off\n");
 } 
 else {
   Serial << F("DebugWire enabled\n");
 }
 Serial << F("            +_____________");
 if (fuse_bits_high & ~(FUSE_RSTDISBL)) {
   Serial << F("RST enabled\n");
 } 
 else {
   Serial << F("RST disabled\n");
 }
}

void print_lock_bits()
{
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__) || \
 defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__) || \
   defined(__AVR_ATmega168A__) || defined(__AVR_ATmega168PA__)

   Serial << F("\nLock Bits = ");
 print_binary(lock_bits);
 Serial << F(" (");
 Serial.print(lock_bits, HEX);
 Serial << F(")\n");
 Serial << F("            ||||||++______");
 switch ((byte)(lock_bits & 3)) {
 case 3:
   Serial << F("Read/Write to everywhere\n"); 
   break;
 case 2:
   Serial << F("Programming of Flash/EEPROM disabled\n"); 
   break;
 case 0:
   Serial << F("No Read/Write of Flash/EEPROM\n"); 
   break;
 default:
   Serial.println();
 }
 Serial << F("            ||||++________");
 switch ((byte)(lock_bits & 0b1100)) {  //BLB0x
 case 0b1100:
   Serial << F("R/W Application\n"); 
   break;
 case 0b1000:
   Serial << F("No Write to App\n"); 
   break;
 case 0b0000:
   Serial << F("No Write to App, no read from Boot\n"); 
   break;
 case 0b0100:
   Serial << F("No Write to App, no read from Boot, no Ints to App\n"); 
   break;
 }

 Serial << F("            ||++__________");
 switch ((byte)(lock_bits & 0b110000)) {  //BLB0x
 case 0b110000:
   Serial << F("R/W Boot Section\n"); 
   break;
 case 0b100000:
   Serial << F("No Write to Boot Section\n"); 
   break;
 case 0b000000:
   Serial << F("No Write to Boot, no read from App\n"); 
   break;
 case 0b010000:
   Serial << F("No Write to Boot, no read from App, no Ints to Boot\n"); 
   break;
 }
#elif defined(__AVR_ATmega8__)
#endif
}

void print_fuse_extended()
{
 Serial << F("\nFuse Extended = ");
 print_binary(fuse_bits_extended);
 Serial << F(" (");
 Serial.print(fuse_bits_extended, HEX);
 Serial << F(")\n");
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
 Serial << F("                |||||+++______");
 switch ((byte)(fuse_bits_extended & 7)) {
 case 7:
   Serial << F("Brownout Disabled\n"); 
   break;
 case 6:
   Serial << F("Brownout at 1.8V\n"); 
   break;
 case 5:
   Serial << F("Brownout at 2.7V\n"); 
   break;
 case 4:
   Serial << F("Brownout at 4.3V\n"); 
   break;
 default:    
   Serial << F("Brownout Reserved value");
   Serial.println(fuse_bits_extended & 7, BIN);
   break;
 }
#elif defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__)
 Serial << F("                |||||||+______");
 if (fuse_bits_extended & bit(FUSE_BOOTRST)) {
   Serial << F("Reset to Start of memory\n");
 } 
 else {
   Serial << F("Reset to Bootstrap\n");
 }
 Serial << F("                |||||++_______");
 switch ((byte)(fuse_bits_extended & ((~FUSE_BOOTSZ1)+(~FUSE_BOOTSZ0)))) {
   case (byte)((~FUSE_BOOTSZ1)+(~FUSE_BOOTSZ0)):
   Serial << F("128 words (256 bytes)\n"); 
   break;
   case (byte)((~FUSE_BOOTSZ1)):
   Serial << F("256 words (512 bytes)\n"); 
   break;
   case (byte)((~FUSE_BOOTSZ0)):
   Serial << F("512 words (1024 bytes)\n"); 
   break;
 case 0:
   Serial << F("1024 words (2048 bytes)\n"); 
   break;
 default:
   Serial.println(fuse_bits_extended & ((~FUSE_BOOTSZ1)+(~FUSE_BOOTSZ0)), BIN);
 }
#elif defined(__AVR_ATmega8__)
#endif
}

void print_signature()
{
 Serial << F("\nSignature:         ");
 Serial.print(sig1, HEX);
 space();
 Serial.print(sig2, HEX);
 space();
 Serial.print(sig3, HEX);
 if (sig1 == 0x1E) { /* Atmel ? */
   switch (sig2) {
   case 0x92:  /* 4K flash */
     if (sig3 == 0x0A) 
       Serial << F(" (ATmega48P)");
     else if (sig3 == 0x05)
       Serial << F(" (ATmega48A)");
     else if (sig3 == 0x09)
       Serial << F(" (ATmega48)");
     break;
   case 0x93:  /* 8K flash */
     if (sig3 == 0x0F) 
       Serial << F(" (ATmega88P)");
     else if (sig3 == 0x0A)
       Serial << F(" (ATmega88A)");
     else if (sig3 == 0x11)
       Serial << F(" (ATmega88)");
     else if (sig3 == 0x08)
       Serial << F(" (ATmega8)");
     break;
   case 0x94:  /* 16K flash */
     if (sig3 == 0x0B) 
       Serial << F(" (ATmega168P)");
     else if (sig3 == 0x06)
       Serial << F(" (ATmega168A)");
     break;
   case 0x95:  /* 32K flash */
     if (sig3 == 0x0F)
       Serial << F(" (ATmega328P)");
     else if (sig3 == 0x14)
       Serial << F(" (ATmega328)");
     break;
   }
 } 
 else {
#if defined (__AVR_ATmega168__) || defined(__AVR_ATmega8__)
   Serial << F(" (Fuses not readable on non-P AVR)");
#else
   Serial << F("????");
#endif
 }
}

void loop()
{
 delay(2000);
 Serial << F("\nCompiled for " __CPUNAME "\n");
 print_serno();  
 cli();
 fuse_bits_low = boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS);
 fuse_bits_extended = boot_lock_fuse_bits_get(GET_EXTENDED_FUSE_BITS);
 fuse_bits_high = boot_lock_fuse_bits_get(GET_HIGH_FUSE_BITS);
 lock_bits = boot_lock_fuse_bits_get(GET_LOCK_BITS);
 sig1 = boot_signature_byte_get(0);
 sig2 = boot_signature_byte_get(2);
 sig3 = boot_signature_byte_get(4);
 oscal = boot_signature_byte_get(1);
 sei();

 Serial << F("\nFuse bits (L/E/H): ");
 Serial.print(fuse_bits_low, HEX);
 space();
 Serial.print(fuse_bits_extended, HEX);
 space();
 Serial.print(fuse_bits_high, HEX);
 Serial << F("\nLock bits:         ");
 Serial.print(lock_bits, HEX);
 print_signature();
 Serial << F("\nOscal:             ");
 Serial.println(oscal, HEX);
 Serial.println();

 print_fuse_low();
 print_fuse_high();
 print_fuse_extended();
 print_lock_bits();

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
#elif defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__) || \
 defined(__AVR_ATmega168A__) || defined(__AVR_ATmega168PA__)
#elif defined(__AVR_ATmega8__)
#endif
   while (1) {
     if (Serial.read() > 0)
       break;
   }
}

