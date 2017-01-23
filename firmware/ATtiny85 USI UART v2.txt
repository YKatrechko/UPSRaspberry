/* ATtiny85 USI UART v2

   David Johnson-Davies - www.technoblogy.com - 3rd May 2015
   ATtiny85 @ 8MHz (external crystal; BOD disabled)
   
   CC BY 4.0
   Licensed under a Creative Commons Attribution 4.0 International license: 
   http://creativecommons.org/licenses/by/4.0/
*/

// Constants
const int DataIn = 0;

unsigned char ReverseByte (unsigned char x) {
  x = ((x >> 1) & 0x55) | ((x << 1) & 0xaa);
  x = ((x >> 2) & 0x33) | ((x << 2) & 0xcc);
  x = ((x >> 4) & 0x0f) | ((x << 4) & 0xf0);
  return x;    
}

// Initialise USI for UART reception.
void InitialiseUSI (void) {  
  pinMode(DataIn, INPUT);         // Define DI as input
  USICR = 0;                      // Disable USI.
  GIFR = 1<<PCIF;                 // Clear pin change interrupt flag.
  GIMSK |= 1<<PCIE;               // Enable pin change interrupts
  PCMSK |= 1<<PCINT0;             // Enable pin change on pin 0
}

// Pin change interrupt detects start of UART reseption.
ISR (PCINT0_vect) {
  if (!(PINB & 1<<PINB0)) {       // Ignore if DI is high
    GIMSK &= ~(1<<PCIE);          // Disable pin change interrupts
    TCCR0A = 2<<WGM00;            // CTC mode
    TCCR0B = 0<<WGM02 | 2<<CS00;  // Set prescaler to /8
    TCNT0 = 0;                    // Count up from 0
    OCR0A = 51;                   // Delay (51+1)*8 cycles
    TIFR |= 1<<OCF0A;             // Clear output compare flag
    TIMSK |= 1<<OCIE0A;           // Enable output compare interrupt
  }
}

// COMPA interrupt indicates middle of start bit
ISR (TIMER0_COMPA_vect) {
  TIMSK &= ~(1<<OCIE0A);          // Disable COMPA interrupt
  TCNT0 = 0;                      // Count up from 0
  OCR0A = 103;                    // Shift every (103+1)*8 cycles
  // Enable USI OVF interrupt, and select Timer0 compare match as USI Clock source:
  USICR = 1<<USIOIE | 0<<USIWM0 | 1<<USICS0;
  USISR = 1<<USIOIF | 8;          // Clear USI OVF flag, and set counter
}

// USI overflow interrupt indicates we've received a byte
ISR (USI_OVF_vect) {
  USICR = 0;                      // Disable USI         
  int temp = USIDR;
  Display(ReverseByte(temp));
  GIFR = 1<<PCIF;                 // Clear pin change interrupt flag.
  GIMSK |= 1<<PCIE;               // Enable pin change interrupts again
}

void Display (char c) {
  // Process received byte here
}

void setup(void) {
  InitialiseUSI();
}

void loop(void) {
}