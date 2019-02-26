/**************************************************************************************************

   pool lap counter

   This is a battery-powered poolside device for counting swim laps. It is a self-contained
   waterproof (well, water-resistant) box with a big button that the swimmer hits, and a
   2-digit display that shows the lap count. The box has suction cups on the bottom, and
   you would typically set it on the edge of the pool deck.

   When you hit the button, the display shows the new lap count for 3 seconds, then goes dark.
   To reset the count to zero, press the button while the count is displayed.
   
   The circuit is designed to maximize the time between battery charging.  One charge of
   the 0.8 Ah lead-acid battery should last for a year of casual use.

   Major components:
   - a waterproof 6" x 3.5" x 2.3" project box
       https://smile.amazon.com/dp/B071FKFLKZ
   - a big (2") diameter pushbutton with light
       https://smile.amazon.com/gp/product/B01M7PNCO9
   - two 1" super bright red 7-segment LCD digit displays (Kingbright SA10-21SRWA)
   - a constant-current LED driver (STMicroelectronics STP16CPC05)
   - a Teensy LC processor board
   - a 0.8 Ah rechargeable lead-acid battery
   - a 7805 5V regulator
   - miscellaneous transistors, capacitors, resistors, and connectors

   Battery usage notes:
   - The current draw when idle is about 18 uA.
     (The CPU is powered down, and lap count has been stored in EEPROM.)
   - The current draw while active for 3 seconds ranges from 100ma to 300ma,
     depending on the number of display segments that are lit up.
   - Assuming a 0.8 Ah battery, and derating 50% to 0.4 Ah for 10V minimum output,
     - the battery would last 20,000 hours (2+ years) if not used
     - with continuous use, the battery should last for almost 2000 lap counts
   - When the 12V battery voltage drops below 10V, the display will flash "LO BA"
     before showing the count, which means "The battery should be recharged soon".

   ------------------------------------------------------------------------------------------------
   Copyright (c) 2019, Len Shustek
     The MIT License (MIT)
   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without
   restriction, including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or
   substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
   BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   ------------------------------------------------------------------------------------------------
  *** CHANGE LOG ***

   16 Feb 2019, V1.0, L. Shustek, first version

 ************************************************************************************************/
#define VERSION "1.0"
#define TEST false  // do diagnostic?

#include <Arduino.h>  // Must also use modified pins_teensy.c with no usb_init and corresponding delays.
#include <EEPROM.h>

#define DISPLAY_TIME    (3*1000UL)     // display the lap count for these msec
#define DEBOUNCE_DELAY 25  // debounce delay in msec

// Arduino pin definitions

#define POWER_ON 6       // active high keeps power MOSFET turned on
#define BUTTON_PUSHED 8  // active low: big pushbutton is pressed
#define BUTTON_LED 5     // active high: turn on LED under the big pushbutton
#define BATTERY A1       // battery check analog input
// STP16CPC05 7-segment LED display driver
#define DISPLAY_LE 9     // active high: latch enable
#define DISPLAY_OE 10    // active low: output enable
#define DISPLAY_CLK 11   // rising edge: clock
#define DISPLAY_DATA 12  // data shift input

struct {    // what we save in non-volatile EEPROM memory
   int id;               // tag that shows we have initialized it
#define EEPROM_ID 'LC'
   int lapcount;         // the current lap counter
} eeprom_data;

void display_load_digit(byte digit) {
   /*  --a--
       f   b
       --g--
       e   c
       --d-- p
   */
   static byte digitmask[] = { // segment order is: abcdefgp
      B11111100, // 0
      B01100000, // 1
      B11011010, // 2
      B11110010, // 3
      B01100110, // 4
      B10110110, // 5
      B10111110, // 6
      B11100000, // 7
      B11111110, // 8
      B11110110, // 9
#define DIGIT_L 10
      B00011100, // "L"
#define DIGIT_LFTBKT 11
      B10011100,
#define DIGIT_RGTBKT 12
      B11110000,
#define DIGIT_BLANK 13
      B00000000,
#define DIGIT_ERR 14
      B10010010, // only horizontal segments
#define DIGIT_A 15
      B11101110, // "A"
#define NUM_DIGITS 16
   };
   if (digit >= NUM_DIGITS) digit = NUM_DIGITS - 1;
   byte mask = digitmask[digit];
   for (byte i = 0; i < 8; ++i) {
      digitalWrite(DISPLAY_DATA, mask & 0x01);
      digitalWrite(DISPLAY_CLK, HIGH);
      mask >>= 1;  // shift mask, and delay for pulse width
      digitalWrite(DISPLAY_CLK, LOW); } }

void display_load_2digits(byte leftdigit, byte rightdigit) {
   display_load_digit(rightdigit);  // shift in 2x8 bits of display segments
   display_load_digit(leftdigit);
   digitalWrite(DISPLAY_LE, HIGH);  // latch into the output register
   delayMicroseconds(5);
   digitalWrite(DISPLAY_LE, LOW); }

void display_show_num(int num) {
   if (num < 10)
      display_load_2digits(DIGIT_BLANK, num);
   else
      display_load_2digits(num / 10, num % 10);
   digitalWrite(DISPLAY_OE, LOW); }

void display_flash(byte leftdigit, byte rightdigit) {
   for (byte i = 0; i < 3; ++i) { // flash something three times
      display_load_2digits(leftdigit, rightdigit);
      digitalWrite(DISPLAY_OE, LOW);
      delay(750);
      digitalWrite(DISPLAY_OE, HIGH);
      delay(250); } }

void read_lapcount(void) {
   for (byte i = 0; i < sizeof(eeprom_data); ++i)
      ((byte *)&eeprom_data)[i] = EEPROM.read(i);
   if (eeprom_data.id != EEPROM_ID) { // never was initialized
      eeprom_data.id = EEPROM_ID;
      eeprom_data.lapcount = 0; } }

void write_lapcount(void) {
   for (byte i = 0; i < sizeof(eeprom_data); ++i)
      EEPROM.update(i, ((byte *)&eeprom_data)[i]); }

void check_battery(void) {
#define LOWBAT 10*1000  // low battery warning level in millivolts
#define AREF_MV  3300   // analog reference voltage, in millivolts
#define R_TOP  (205*3)  // voltage divider top resistor, in Kohms
#define R_BOT  205      // voltage divider bottom resistor, in Kohms
   uint16_t aval = analogRead(BATTERY); // 0..1023
   // aval = (V *(BOT/(TOP+BOT) / AREF) * 1024; solve for V in millivolts
   uint16_t Vmv = ((unsigned long)aval * AREF_MV * (R_TOP + R_BOT)) / (1024UL * R_BOT);
   if (TEST) { // show volts, then hundredths of a volt
      delay(500);
      display_show_num(Vmv / 1000);
      delay(1000);
      digitalWrite(DISPLAY_OE, HIGH);
      display_show_num((Vmv % 1000) / 10);
      delay(1000);
      digitalWrite(DISPLAY_OE, HIGH);
      delay(500); }
   if (Vmv < LOWBAT) {
      display_load_2digits(DIGIT_L, 0); // "LO"
      digitalWrite(DISPLAY_OE, LOW);
      delay(1000);
      display_load_2digits(8, DIGIT_A); // "BA" (well, "8A")
      delay(1000);
      digitalWrite(DISPLAY_OE, HIGH);
      delay(1000); } }

void setup(void) {
   pinMode(POWER_ON, OUTPUT);     // turn on the MOSFET as quickly as possible
   digitalWrite(POWER_ON, HIGH);  // to keep power on
   // To allow a short button push to get things started, we use a modified version of
   // pins_teensy.c that doesn't initialize the USB port, because that uses up several
   // hundred milliseconds before we get to execute this code that keeps the power on.

   pinMode(BUTTON_PUSHED, INPUT_PULLUP); // then configure all the other pins
   pinMode(BUTTON_LED, OUTPUT);
   digitalWrite(BUTTON_LED, HIGH); // turn on the button's light
   pinMode(BATTERY, INPUT);
   pinMode(DISPLAY_LE, OUTPUT);
   pinMode(DISPLAY_OE, OUTPUT);
   pinMode(DISPLAY_CLK, OUTPUT);
   pinMode(DISPLAY_DATA, OUTPUT);
   delay(DEBOUNCE_DELAY); // get beyond push bounces

   if (TEST) {
      for (byte i = 0; i < 3; ++i) {
         delay(250);
         digitalWrite(BUTTON_LED, LOW); // flash the button's light
         delay(250);
         digitalWrite(BUTTON_LED, HIGH); }
      for (byte i = 0; i < NUM_DIGITS - 1; ++i) { // display all digits
         display_load_2digits(i, i + 1);
         digitalWrite(DISPLAY_OE, LOW);
         delay(500);
         digitalWrite(DISPLAY_OE, HIGH);
         delay(100); } }

   check_battery();         // warn if  low battery
   read_lapcount();
   ++eeprom_data.lapcount;  // increment and display the lapcount
   display_show_num(eeprom_data.lapcount);
   while (digitalRead(BUTTON_PUSHED) == LOW) ; // wait for button to be released
   delay(DEBOUNCE_DELAY);

   unsigned long start_time = millis();
   while (millis() < start_time + DISPLAY_TIME) { // wait until display expiration time
      if (digitalRead(BUTTON_PUSHED) == LOW) { // if button pushed meanwhile,
         eeprom_data.lapcount = 0;  // zero the lap count
         display_show_num(eeprom_data.lapcount);
         delay(DEBOUNCE_DELAY);
         while (digitalRead(BUTTON_PUSHED) == LOW) ; // wait for button to be released
         delay(DEBOUNCE_DELAY);
         start_time = millis(); // and restart the display time
      } }

   digitalWrite(DISPLAY_OE, HIGH); // turn off the display
   digitalWrite(BUTTON_LED, LOW);  // and the button's light
   write_lapcount();               // update the lapcount in non-volatile memory
   digitalWrite(POWER_ON, LOW);    // shutoff the power
   while (1) ;                     // and wait until the lights go out
}

void loop(void) {
   /* never executed */
}
//*
