
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>


/* https://raw.githubusercontent.com/damellis/attiny/ide-1.6.x-boards-manager/package_damellis_attiny_index.json
  Internal 1 MHz
  low_fuses = 0x62
  high_fuses = 0xDF
  extended_fuses = 0xFF
*/

#define DEBUG 1

// MACROS FOR EASY PIN HANDLING FOR ATMEL GCC-AVR http://www.starlino.com/port_macro.html
//these macros are used indirectly by other macros , mainly for string concatination
#define _SET(type,name,bit)          type ## name  |= _BV(bit)
#define _CLEAR(type,name,bit)        type ## name  &= ~ _BV(bit)
#define _TOGGLE(type,name,bit)       type ## name  ^= _BV(bit)
#define _GET(type,name,bit)          ((type ## name >> bit) &  1)
#define _PUT(type,name,bit,value)    type ## name = ( type ## name & ( ~ _BV(bit)) ) | ( ( 1 & (unsigned char)value ) << bit )

//these macros are used by end user
#define OUTPUT(pin)         _SET(DDR,pin)
#define INPUT(pin)          _CLEAR(DDR,pin)
#define HIGH(pin)           _SET(PORT,pin)
#define LOW(pin)            _CLEAR(PORT,pin)
#define TOGGLE(pin)         _TOGGLE(PORT,pin)
#define READ(pin)           _GET(PIN,pin)


#define CALIBRATE_PIN  B,2    // short pin to vcc after reset for calibration

#define CHARGE_PIN     B,4    // input for checking charging voltage
#define LED_PIN        B,3    // out for LED
#define CHECK_PIN      B,2    // input for checking if Raspberry shutdown
#define SIGNAL_PIN     B,1    // out for signaling shutdown to Raspberry
#define POWER_PIN      B,0    // out for MOSFET key


#define U_crit           3300      //3,3V
#define U_low            3600      //3,6V


#define LED_ON()     HIGH(LED_PIN)
#define LED_OFF()    LOW(LED_PIN)
#define LED_TOGGLE() TOGGLE(LED_PIN)

#if defined DEBUG
#define RED_TOGGLE() TOGGLE(SIGNAL_PIN)
#define GREEN_TOGGLE() TOGGLE(POWER_PIN)
#define RED_ON()     HIGH(SIGNAL_PIN)
#define RED_OFF()    LOW(SIGNAL_PIN)
#define GREEN_ON()   HIGH(POWER_PIN)
#define GREEN_OFF()  LOW(POWER_PIN)
#endif


EEMEM uint16_t eeEmpty2bytes = 0;         //переменная для "отступа" от начала eeprom памяти
EEMEM uint8_t eeCalibrated = 0;           //принимает значение 0x55 после успешной калибровки.
EEMEM uint16_t eeUinp42V = 4200;       //хранит значение АЦП после калибровки при Uпитания = 4,20В

char timer_counter;
uint16_t Uinp;
char blink = 1;

enum {
  CHARGE_OK = 0,
  BAT_OK,
  BAT_LOW,
  BAT_STOP,
  ALL_STOP
} mode;

uint8_t temp, count, start;
volatile uint8_t c;

#define BAUD_C 123
#define TxD PB4

#define T_START TCCR0B = (1 << CS01) // F_CPU/8
#define T_STOP TCCR0B = 0
#define T_RESET TCNT0 = 0


// Timer 1 overflow interrupt
ISR (TIMER1_OVF_vect) {

  //  if (READ(CHARGE_PIN)) {
  //    mode = CHARGE_OK;
  //  } else {
  //     Uinp = readVcc();       // read Vcc
  //    if (Uinp <= U_crit) {
  //      mode = BAT_STOP;
  //    } else if (Uinp <= U_low) {
  //      mode = BAT_LOW;
  //    } else {
  //      mode = BAT_OK;
  //    }
  //  }
  Uinp = readVcc();       // read Vcc
  if (Uinp <= 3300) {
    GREEN_OFF();
    RED_ON();
  } else if (Uinp <= 3600) {
    GREEN_OFF();
    RED_TOGGLE();
  } else if (Uinp <= 4900) {
    RED_OFF();
    GREEN_TOGGLE();
  } else  {
    RED_OFF();
    GREEN_ON();
  }

  return;

  //#if defined DEBUG
  //  GREEN_TOGGLE();
  //  return;
  //#endif

  if (eeprom_read_byte(&eeCalibrated) != 0x55) {   // Wrong eeprom
    switch (blink++) {                //бегущий огонь )))
#if defined DEBUG
      case 0: blink = 1; GREEN_TOGGLE(); break;
      case 1:  case 2:  case 3: break;
      case 4: RED_TOGGLE(); break;
      default:  blink = 0; break;

#else
      case 0: blink = 1; break;
      case 1:  case 2:  case 3: break;
      case 4: LED_TOGGLE(); break;
      default:  blink = 0; break;
#endif
    };
  }
  //  else {
  //    Uinp = readVcc();       //читаем значение напряжения с входа АЦП
  //    led_off();                         //выключаем все светодиоды
  //    if (Uinp >= U_input(U_5)) {        //сравниваем полученное значение напряжения с забитыми в дэфайнах
  //      //Если > U_5
  //      HIGH(LED_GREEN);                   //включаем зелёный светодиод
  //      blink = 1;                       //сбрасываем флаг моргания
  //    } else if ((Uinp < U_input(U_5)) & (Uinp >= U_input(U_4))) {
  //      //от U_4 до U_5
  //      TOGGLE(LED_GREEN);               //"моргаем" зелёным
  //      blink = !blink;                  //меняем (инвертируем) флаг моргания
  //    } else if ((Uinp < U_input(U_4)) & (Uinp >= U_input(U_3))) {
  //      //от U_3 до U_4
  //      HIGH(LED_YELLOW);                  //включаем желтый светодиод
  //      blink = 1;                       //сбрасываем флаг моргания
  //    } else if ((Uinp < U_input(U_3)) & (Uinp >= U_input(U_2))) {
  //      //от U_2 до U_3
  //      TOGGLE(LED_YELLOW);              //"моргаем" желтым
  //      blink = !blink;                  //меняем (инвертируем) флаг моргания
  //    } else if ((Uinp < U_input(U_2)) & (Uinp >= U_input(U_1))) {
  //      //от U_1 до U_2
  //      HIGH(LED_RED);                     //включаем красный светодиод
  //      blink = 1;                       //сбрасываем флаг моргания
  //    } else
  //    if (Uinp < U_input(U_1)) {
  //      //менее U_1
  //      LED_ON();                //"моргаем" красным
  //      blink = !blink;                 //меняем (инвертируем) флаг моргания
  //    }

}

uint16_t readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(MUX3) | _BV(MUX2) ; // | _BV(REFS1)
  _delay_ms(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = 1125300L / result; // Calculate Vcc in mV 1125300 = 1.1*1023*1000
  return (uint16_t)result;
}

void calibrate() {
  INPUT(CALIBRATE_PIN);
  HIGH(CALIBRATE_PIN);
  if (READ(CALIBRATE_PIN)) { // start calibration
    OUTPUT(LED_PIN);
    _delay_ms(1000);                        //ждём 1000 мс
    LED_TOGGLE();                        //зажигаем желтый светодиод
    _delay_ms(1000);                        //ждём 1000 мс
    readVcc();           //читаем значение напряжения на входе АЦП "в никуда"
    LED_TOGGLE();                          //зажигаем зелёный светодиод
    _delay_ms(1000);                        //ждём 1000 мс
    //    eeUinp42V =       //читаем значение напряжения на входе АЦП и сохраняем в EEPROM

    eeprom_write_word(&eeUinp42V, readVcc());
    eeprom_write_byte(&eeCalibrated, 0x55); //обозначаем что откалибровано

    for (uint8_t i = 7; i > 0; i--) {
      LED_TOGGLE();
      _delay_ms(200);
    }
    _delay_ms(3000);                        //ждём 3 секунды
  }
}

int main(void) {
  DDRB = 0x00;    //PB off output

  GTCCR = 0x00;
  // Timer/Counter 1 initialization   Mode: Normal top=0xFF
  TCCR1 = (1 << CS13) | (0 << CS12) | (1 << CS11) | (1 << CS10) ;     // timer_prescale(1024);
  TCNT1 = 0x00;
  OCR1A = 0x00;
  OCR1B = 0x00;
  OCR1C = 0x00;
  TIMSK |= 1 << TOIE1;     // timer_ovf_enable();

  GIMSK = 0x00;  // External Interrupt(s) initialization INT0: Off
  MCUCR = 0x00;  // Interrupt on any change on pins PCINT0-5: Off

  ACSR = 0x80;   // Analog Comparator: Off
  ADCSRB = 0x00;
  DIDR0 = 0x00;

  if (eeEmpty2bytes != 0) eeprom_write_word(&eeEmpty2bytes, 0x00);

  calibrate();

  OUTPUT(POWER_PIN);
  OUTPUT(SIGNAL_PIN);
  OUTPUT(LED_PIN);

  LOW(POWER_PIN);
  LOW(SIGNAL_PIN);
  LOW(LED_PIN);

  INPUT(CHARGE_PIN);
#if defined DEBUG
  OUTPUT(POWER_PIN);
  OUTPUT(SIGNAL_PIN);
#endif


  sei(); // Global enable interrupts
  while (1) {

//    //    mode = CHARGE_OK;
//    switch (mode) {
//      case CHARGE_OK:  RED_ON(); break;
//      case BAT_OK:   RED_OFF(); break;
//      case BAT_LOW:  GREEN_TOGGLE(); break;
//      case BAT_STOP:  GREEN_ON(); break;
//      case ALL_STOP:  break;
//      default:  break;
//    }
    _delay_ms(100);
  }
}



