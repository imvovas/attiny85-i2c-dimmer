// автономный диммер на ATtiny85



// управляется по I2C.



#include <TinyWireS.h>

#define I2C_SLAVE_ADDR (0x41) //адрес устройства

#define DETECT 1 //физический пин 6, детектор ноля, прерывание 0

#define GATE 3 //физический пин 2, выход на симистор

#define PULSE 5 //константа счетчика импульсов, определяет ширину управляющего импулса

// I2C pins: PB2 - SCL физический пин 7, PB0 - SDA физический пин 5

volatile byte i2cValue, ocr, jj;



void setup() {

  delay(2000); //задержка нужна чтобы не хватать мусор из шины при инициализации

  ocr = 0; // выключение нагрузки при инициализации

  // set up pins

  TinyWireS.begin(I2C_SLAVE_ADDR);

  DDRB &= ~(1 << DETECT); //zero cross detect

  PORTB |= (1 << DETECT); //enable pull-up resistor

  DDRB |= 1 << GATE; //triac gate control

  GIMSK = 1 << PCIE;

  PCMSK = 1 << PCINT1;

  OCR1A = 50; //initialize the comparator

  TIMSK = _BV(OCIE1A) | _BV(TOIE1); //interrupt on Compare Match A | enable timer overflow interrupt

  TinyWireS.onRequest(requestEvent);

  TCCR1 = 0;

  while ((PINB & (1 << PINB1)) != 0);

  while ((PINB & (1 << PINB1)) == 0);

  for (uint8_t ii = 0; ii < 255; ii++) jj++;

  TIFR = 0xff;

  for (uint8_t ii = 0; ii < 200; ii++) jj++;

  sei(); // enable interrupts

}

ISR (PCINT0_vect) {

  if (TCCR1 > 0) return;

  TCCR1 = 0;

  TCNT1 = 0; //reset timer - count from zero

  OCR1A = ocr;

  TCCR1 = B00001010; // prescaler on 1024, see table 12.5 of the tiny85 datasheet

}

ISR(TIMER1_COMPA_vect) //comparator match

{

  PORTB |= (1 << GATE); //set triac gate to high

  TCCR1 = 0;

  TCNT1 = 255 - PULSE; //trigger pulse width, when TCNT1=255 timer1 overflows

  TCCR1 = B00001010;

}

ISR(TIMER1_OVF_vect) { //timer1 overflow

  PORTB &= ~(1 << GATE); //turn off triac gate

  TCCR1 = 0; //disable timer stop unintended triggers

}



void loop() {

  if (TinyWireS.available()) i2cValue = TinyWireS.receive();// проверка значений полученных по i2c что бы отсеять ненужное

  if ((i2cValue < 1) || (i2cValue > 254)) ocr = 0; // все что меньше 1 и больше 254 выключает нагрузку

  else ocr = map(i2cValue, 0, 254, 145, 29); // рабочий диапазон от 1-254

  TinyWireS_stop_check();

}



void requestEvent() {

  TinyWireS.send(i2cValue);

}
