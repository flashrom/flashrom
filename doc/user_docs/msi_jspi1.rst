=========
MSI JSPI1
=========

JSPI1 is a 5x2 or 6x2 2.0mm pitch pin header on many MSI motherboards.
It is used to recover from bad boot ROM images. Specifically,
it appears to be used to connect an alternate ROM with a working image.
Pull the #HOLD line low to deselect the onboard SPI ROM, allowing another
SPI ROM to take its place on the bus. Pull the #WP line high to disable write-protection.
Some boards use 1.8V flash chips, while others use 3.3V flash chips;
Check the flash chip datasheet to determine the correct value.

**JSPI1 (5x2)**

======== ======== ======== ====
name     pin      pin      name
======== ======== ======== ====
VCC      1        2 	   VCC
MISO     3        4	   MOSI
#SS      5        6	   SCLK
GND      7        8	   GND
#HOLD    9        10 	   NC
======== ======== ======== ====

**JSPI1 (6x2)**

======== ======== ======== ============
name     pin      pin      name
======== ======== ======== ============
VCC      1	  2	   VCC
SO       3        4	   SI
#SS      5	  6	   CLK
GND      7        8	   GND
NC       9        10	   NC (no pin)
#WP      11       12	   #HOLD
======== ======== ======== ============

======== =====================================
name	 function
======== =====================================
VCC	 Voltage (See flash chip datasheet)
MISO	 SPI Master In/Slave Out
MOSI	 SPI Master Out/Slave In
#SS	 SPI Slave (Chip) Select (active low)
SCLK	 SPI Clock
GND	 ground/common
#HOLD	 SPI hold (active low)
#WP	 SPI write-protect (active low)
NC	 Not Connected (or no pin)
======== =====================================
