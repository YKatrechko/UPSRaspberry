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

### Links
- http://hardlock.org.ua/viewtopic.php?f=9&t=442
- https://pi.gate.ac.uk/pages/mopi.html
- http://www.mosaic-industries.com/embedded-systems/microcontroller-projects/raspberry-pi/on-off-power-controller


