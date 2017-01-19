
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
// https://raw.githubusercontent.com/damellis/attiny/ide-1.6.x-boards-manager/package_damellis_attiny_index.json



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

//"voltage" in hundrets millivolt 42 = 4,2V.
#define U_1            33      //3,3V
#define U_2            36      //3,6V
#define U_3            38      //3,8V
#define U_4            39      //3,9V
#define U_5            41      //4,1V

#define LED_ON()     HIGH(LED_PIN)
#define LED_OFF()    LOW(LED_PIN)
#define LED_TOGGLE() TOGGLE(LED_PIN)


EEMEM uint16_t eeEmpty2bytes = 0;         //переменная для "отступа" от начала eeprom памяти
EEMEM uint8_t eeCalibrated = 0;           //принимает значение 0x55 после успешной калибровки.
EEMEM uint16_t eeUinp42V = 4200;       //хранит значение АЦП после калибровки при Uпитания = 4,20В

char timer_counter;
unsigned int Uinp;
char blink = 1;

// Timer 0 overflow interrupt service routine
ISR (TIMER1_OVF_vect) {
  // 18,38 Hz
  if (++timer_counter > 2) {  //понижаем частоту таймера в 3 раза
    timer_counter = 0;
  } else {
    return;
  }

  if (eeprom_read_byte(&eeCalibrated) != 0x55) {   // Wrong eeprom
    switch (blink++) {                //бегущий огонь )))
      case 0: blink = 1; break;
      case 1:  case 2:  case 3: break;
      case 4: LED_TOGGLE(); break;
      default:  blink = 0; break;
    };
  } else {
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
    if (Uinp < U_input(U_1)) {
      //менее U_1
      LED_ON();                //"моргаем" красным
      blink = !blink;                 //меняем (инвертируем) флаг моргания
    }
  }
}

//Функция вычисления значения АЦП для любого напряжения.
unsigned int U_input(char U_x) {   //U_x - напряжение для которого необходимо вычислить значение АЦП
  unsigned int temp;              //вычисление происходит исходя из того факта, что при калибровке устройства
  //напряжение питания было 4,20 Вольта
  temp = eeUinp42V * U_x / 42;    //АЦП = eeUinp42V / 42 * U_x;  Например для 3,3В: АЦП = 982 * 33 / 42 = 772
  //Если поменять местами, то компилятор оптимизирует и на выходе получим фигню
  //из за округления на первом шаге. Проверьте на калькуляторе в режиме округления до целых чисел )))
  return temp;
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


  sei(); // Global enable interrupts
  while (1) {}
}



