  pool lap counter

   This is a battery-powered poolside device for counting swim laps. It is a self-contained
   waterproof (well, water-resistant) box with a big button that the swimmer hits, and a
   2-digit display that shows the lap count. The box has suction cups on the bottom, and
   you would typically set it on the edge of the pool deck.

   When you hit the button, the display shows the new lap count for 3 seconds, then goes dark.
   To reset the count to zero, press the button while the count is displayed.
   To show the battery charge percent, press and hold the button.
   
   The circuit is designed to maximize the time between battery charging.  One charge of
   the 0.8 Ah lead-acid battery should last for a year of casual use.

   Major components:
 - a waterproof 4.7" x 4.7" x 3.2" project box with a clear lid
       https://www.mouser.com/ProductDetail/546-1554P2GYCL
   - a big (2") diameter pushbutton with light
       https://smile.amazon.com/gp/product/B01M7PNCO9
   - two 1" super bright red 7-segment LCD digit displays (Kingbright SA10-21SRWA)
   - a constant-current LED driver (STMicroelectronics STP16CPC05)
   - a Teensy LC processor board
   - a 0.8 Ah rechargeable lead-acid battery
   - a 7805 5V regulator
   - miscellaneous transistors, capacitors, resistors, and connectors

L. Shustek, February 2019
Updated April 21, 2019 for printed circuit board version
Updated September 31, 2020 to include battery charge display and circuit tweaks