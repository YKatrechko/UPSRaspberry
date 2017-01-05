/*****************************************************
This program was produced by the
CodeWizardAVR V2.05.3 Professional
Automatic Program Generator
© Copyright 1998-2011 Pavel Haiduc, HP InfoTech s.r.l.
http://www.hpinfotech.com

Project : Li-Ion_voltmeter
Version : 1.0
Date    : 06.10.2016
Author  : hardlock
Company : hardlock.org.ua
Comments: Topic on my forum: http://hardlock.org.ua/viewtopic.php?f=9&t=442
          Tiny voltmeter for 1 cell li-ion battery


Chip type               : ATtiny13A
AVR Core Clock frequency: 4,800000 MHz
Memory model            : Tiny
External RAM size       : 0
Data Stack size         : 16
*****************************************************/

#include <tiny13a.h>  
#include <delay.h>

#define CALIBRATE_PIN PINB.2   //пин, который необходимо замкнуть на питание для выполнения калибровки.

#define LED_RED     PORTB.2    //выход красного светодиода
#define LED_YELLOW  PORTB.1    //выход желтого светодиода
#define LED_GREEN   PORTB.0    //выход зелёного светодиода

#define VOLT_INPUT  2          //вход АЦП

                               //"напряжение" в сотнях миливольт, т.е. 42 = 4,2 Вольта.
#define U_1            33      //3,3V
#define U_2            36      //3,6V
#define U_3            38      //3,8V
#define U_4            39      //3,9V
#define U_5            41      //4,1V

eeprom unsigned int eeEmpty2bytes = 0;         //переменная для "отступа" от начала eeprom памяти
eeprom char eeCalibrated = 0;                  //принимает значение 0x55 после успешной калибровки.
eeprom unsigned int eeUinp42V = 982;            //хранит значение АЦП после калибровки при Uпитания = 4,20В

char timer_counter;
unsigned int Uinp;
char blink = 1;

#define ADC_VREF_TYPE 0x40    //1.1V Vref

// Read the AD conversion result
unsigned int read_adc(unsigned char adc_input)
{
ADMUX=adc_input | (ADC_VREF_TYPE & 0xff);
// Delay needed for the stabilization of the ADC input voltage
delay_us(10);
// Start the AD conversion
ADCSRA|=0x40;
// Wait for the AD conversion to complete
while ((ADCSRA & 0x10)==0);
ADCSRA|=0x10;
return ADCW;
}

void led_off(void)
{
  PORTB = PINB & 0b11111000;      //устанавливаем нолики сразу на всех выходах светодиодов.    
}
                                  //Функция вычисления значения АЦП для любого напряжения.
unsigned int U_input(char U_x)    //U_x - напряжение для которого необходимо вычислить значение АЦП
{
  unsigned int temp;              //вычисление происходит исходя из того факта, что при калибровке устройства
                                  //напряжение питания было 4,20 Вольта
  temp = eeUinp42V * U_x / 42;    //АЦП = eeUinp42V / 42 * U_x;  Например для 3,3В: АЦП = 982 * 33 / 42 = 772
                                  //Если поменять местами, то компилятор оптимизирует и на выходе получим фигню
                                  //из за округления на первом шаге. Проверьте на калькуляторе в режиме округления до целых чисел )))  
  return temp;
}

// Timer 0 overflow interrupt service routine
interrupt [TIM0_OVF] void timer0_ovf_isr(void)
{
  // 18,38 Hz    
  if (++timer_counter > 2)   //понижаем частоту таймера в 3 раза
  {
    timer_counter = 0;
  }                   
  else
  {
    return;
  } 
  
  if (eeCalibrated != 0x55)           //если не прошился или сбился EEPROM, вобщем не откалиброван дэвайс.
  {                                   
    led_off();             
    switch (blink ++)                         //бегущий огонь )))
    {                       
      case 0: blink = 1;       break;
      case 1: LED_GREEN   = 1; break;
      case 2: LED_YELLOW  = 1; break;
      case 3: LED_RED     = 1; break;      
      default:      blink = 0; break;
    }; 
  }
  else
  { 
    Uinp = read_adc(VOLT_INPUT);       //читаем значение напряжения с входа АЦП
    led_off();                         //выключаем все светодиоды
    if (Uinp >= U_input(U_5))          //сравниваем полученное значение напряжения с забитыми в дэфайнах
    {                                  //Если > U_5
      LED_GREEN = 1;                   //включаем зелёный светодиод
      blink = 1;                       //сбрасываем флаг моргания
    }
    else if ((Uinp < U_input(U_5)) & (Uinp >= U_input(U_4)))
    {                                  //от U_4 до U_5
      LED_GREEN = blink;               //"моргаем" зелёным
      blink = !blink;                  //меняем (инвертируем) флаг моргания
    }
    else if ((Uinp < U_input(U_4)) & (Uinp >= U_input(U_3)))
    {                                  //от U_3 до U_4
      LED_YELLOW = 1;                  //включаем желтый светодиод
      blink = 1;                       //сбрасываем флаг моргания
    }
    else if ((Uinp < U_input(U_3)) & (Uinp >= U_input(U_2)))
    {                                  //от U_2 до U_3
      LED_YELLOW = blink;              //"моргаем" желтым
      blink = !blink;                  //меняем (инвертируем) флаг моргания
    }
    else if ((Uinp < U_input(U_2)) & (Uinp >= U_input(U_1)))
    {                                  //от U_1 до U_2
      LED_RED = 1;                     //включаем красный светодиод
      blink = 1;                       //сбрасываем флаг моргания
    }
    else if (Uinp < U_input(U_1))
    {                                 //менее U_1
      LED_RED = blink;                //"моргаем" красным
      blink = !blink;                 //меняем (инвертируем) флаг моргания
    }
  }    
}

// Declare your global variables here

void main(void)
{
// Declare your local variables here

// Crystal Oscillator division factor: 1
#pragma optsize-
CLKPR=0x80;
CLKPR=0x00;
#ifdef _OPTIMIZE_SIZE_
#pragma optsize+
#endif

// Input/Output Ports initialization
// Port B initialization
// Func5=In Func4=In Func3=In Func2=Out Func1=Out Func0=Out 
// State5=T State4=T State3=T State2=0 State1=0 State0=0 
PORTB=0x00;
DDRB=0x07;

// Timer/Counter 0 initialization
// Clock source: System Clock
// Clock value: 4,688 kHz
// Mode: Normal top=0xFF
// OC0A output: Disconnected
// OC0B output: Disconnected
TCCR0A=0x00;
TCCR0B=0x05;
TCNT0=0x00;
OCR0A=0x00;
OCR0B=0x00;

// External Interrupt(s) initialization
// INT0: Off
// Interrupt on any change on pins PCINT0-5: Off
GIMSK=0x00;
MCUCR=0x00;

// Timer/Counter 0 Interrupt(s) initialization
TIMSK0=0x02;

// Analog Comparator initialization
// Analog Comparator: Off
ACSR=0x80;
ADCSRB=0x00;
DIDR0=0x00;

// ADC initialization
// ADC Clock frequency: 600,000 kHz
// ADC Bandgap Voltage Reference: On
// ADC Auto Trigger Source: ADC Stopped
// Digital input buffers on ADC0: On, ADC1: On, ADC2: Off, ADC3: On
DIDR0&=0x03;
DIDR0|=0x10;
ADMUX=ADC_VREF_TYPE & 0xff;
ADCSRA=0x83;

if (eeEmpty2bytes != 0)                  //чтоб ничего не приснилось )))
{ 
  eeEmpty2bytes = 0;
}

led_off();                               //выключаем все светодиоды

DDRB = 0x03;                             //переводим "калибровочный пин" в режим входа


if (CALIBRATE_PIN)                       //проверяем, что "калибровочный пин" замкнут на питание
{                                        //значит начинаем калибровку!
  delay_ms(1000);                        //ждём 1000 мс
  LED_YELLOW = 1;                        //зажигаем желтый светодиод
  delay_ms(1000);                        //ждём 1000 мс
  Uinp = read_adc(VOLT_INPUT);           //читаем значение напряжения на входе АЦП "в никуда"
  LED_GREEN = 1;                         //зажигаем зелёный светодиод
  delay_ms(1000);                        //ждём 1000 мс                                           
  eeUinp42V = read_adc(VOLT_INPUT);      //читаем значение напряжения на входе АЦП и сохраняем в EEPROM
  eeCalibrated = 0x55;                   //обозначаем что откалибровано
  led_off();                             //гасим светодиоды
  delay_ms(3000);                        //ждём 3 секунды
}


DDRB=0x07;                               //настраиваем пины на выход
// Global enable interrupts
#asm("sei")                              //Разрешаем прерывания (до этого таймер не работал)

//Дальше вся работа в таймере

while (1)
      {
      // Place your code here

      }
}
