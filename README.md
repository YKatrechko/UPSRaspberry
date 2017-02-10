# UPSRaspberry

###Description
This is project of UPS for Raspberry Pi 2 based on TP4056 charging module and ATtiny13.

Idea from __hardlock.org.ua__ [1S li-ion indicator](http://hardlock.org.ua/viewtopic.php?f=9&t=442).

### Modes

 Nmbr |   Mode  | AC  | BATT | TIMER | OUT | LED
:----:| ------- |:---:|:----:|:-----:|:---:|:------
  0   | AC_OK   |  1  |  x   |   x   |  1  |  
  1   | BAT_OK  |  0  | >3v3 |   x   |  1  |  
  2   | BAT_LOW |  0  | <3v3 | TMR-- |  1  |  
  3   | BAT_CRT |  0  | <3v1 |   x   |  0  |  
  4   | STOPPED |  0  |  x   |   x   |  0  |  

### Hardware
Atmel controller
![ATtiny25/45/85 Pinouts](images/ATtiny-Pinouts-8.jpg "ATtiny25/45/85 Pinouts")
TP4056 1A Standalone Linear Li-lon Battery Charger
![TP4056 module](images/tp4056view.jpg "TP4056 module")
<img src="images/tp4056view.jpg" width="320">

##### Fuses (default):
- Internal 1 MHz
- low_fuses = 0x62
- high_fuses = 0xDF
- extended_fuses = 0xFF

### Links
- http://hardlock.org.ua/viewtopic.php?f=9&t=442
- https://pi.gate.ac.uk/pages/mopi.html
- http://www.mosaic-industries.com/embedded-systems/microcontroller-projects/raspberry-pi/on-off-power-controller
- http://www.atmel.com/images/atmel-2586-avr-8-bit-microcontroller-attiny25-attiny45-attiny85_datasheet.pdf
- https://dlnmh9ip6v2uc.cloudfront.net/datasheets/Prototyping/TP4056.pdf


### Help
- https://guides.github.com/features/mastering-markdown/
- https://github.com/adam-p/markdown-here/wiki/Markdown-Cheatsheet
