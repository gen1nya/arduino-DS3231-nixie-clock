#include<RTClib.h>
#include <Wire.h>
#include <util/delay.h>

#define SQW_IN ~PINC & (1 <<  PC3) // тактовый сигнал
#define BTN_H ~PINC & (1 << PC2) // лапка
#define BTN_M ~PINC & (1 << PC1) // еще лапка
#define BTN_S ~PINC & (1 << PC0) // лапка на всякий случай

const uint8_t psc[13]={245,231,210,189,168,147,126,105,84,63,42,21,1}; // ступени яркости

#define ANIMATION_LENGTH 400
#define MS_PER_FRAME sizeof(pcs) / ANIMATION_LENGTH

RTC_DS3231 rtc;

uint8_t currentElement = 0;
uint8_t tscr[6]; // временный экран
uint8_t display[6] = {1, 2, 3, 4, 5, 6 };
uint8_t brightness[6] = {245,245,245,245,245 };
DateTime now;
bool needUpdateTime = false;
bool needIncrementMinutes = false;
bool needIncrementHours = false;

bool needToApplyEffect[6] = {true, true, true, true, true, true};

void soft(char D1, char D2, char D3) { // эффект плавной смены цифр, здесь идет поиск тех цифр, которые надо менять
  int8_t i = 0,y = 0;
  tscr[0] = D1 / 10; // запомнили новое время
  tscr[1] = D1 % 10;
  tscr[2] = D2 / 10;
  tscr[3] = D2 % 10;
  tscr[4] = D3 / 10;
  tscr[5] = D3 % 10;

  for (i = 0; i < 6; i++){
      needToApplyEffect[i] = display[i] != tscr[i];
  }

 for (i = 0; i < 13; i++){
    _delay_ms(20);
    for (y = 0; y < 6; y++)
      if (needToApplyEffect[y]) brightness[y] = psc[i];
  }
  for (i = 0; i < 6; i++)
      display[i] = tscr[i];
  
  for (i = 12; i >= 0; i--){
    _delay_ms(20);
    for (y = 0; y < 6; y++)
      if (needToApplyEffect[y]) brightness[y] = psc[i];
  }
}

void incrementHours(){
  if (now.hour() >= 23) {
    rtc.adjust(
      DateTime(
        now.year(), 
        now.month(), 
        now.day(), 
        0,
        now.minute(), 
        now.second()
      )
    );
  } else {
    rtc.adjust(
      DateTime(
        now.year(),
        now.month(),
        now.day(), 
        now.hour() +1, 
        now.minute(), 
        now.second()
      )
    );
  }
}

void incrementMinutes(){
  if (now.minute() >= 59) {
    rtc.adjust(
      DateTime(
        now.year(),
        now.month(),
        now.day(),  
        now.hour(),
        0, 
        now.second()
      )
    );
  } else {
    rtc.adjust(
      DateTime(
        now.year(), 
        now.month(),
        now.day(), 
        now.hour(), 
        now.minute() +1, 
        now.second()
      )
    );
  } 
}

void setup() {
  DDRB |= (1<<DDB0)|(1<<DDB1)|(1<<DDB2)|(1<<DDB3);
  DDRD |= (1<<DDD1)|(1<<DDD2)|(1<<DDD3)|(1<<DDD4)|(1<<DDD5)|(1<<DDD6)|(1<<DDD6);
  DDRC = 0;
  PORTC |= (1<<PC3)|(1<<PC2)|(1<<PC1)|(1<<PC0);
  Serial.begin(9600);
  rtc.begin(); 
  rtc.writeSqwPinMode(DS3231_SquareWave1Hz);
  
  cli();
  TCCR2B = (1<<CS20)|(1<<CS21);
  TIMSK2 = (1<<OCIE2A)|(1<<TOIE2);
  OCR2A = 245;
  PCICR |= (1 << PCIE1);
  PCMSK1 = (1 << PCINT8)|(1 << PCINT9)|(1 << PCINT10)|(1 << PCINT11);
  sei();
}

void loop() {
 if (needUpdateTime){
    Serial.print("time update");
    Serial.println(now.second());
    now = rtc.now();
    needUpdateTime = false;
    soft(now.hour(), now.minute(), now.second());
 }

  if (needIncrementHours){
    needIncrementHours = false;
    Serial.println("inc hours");
    incrementHours();
    now = rtc.now();
    soft(now.hour(), now.minute(), now.second());
    delay(200);
    needIncrementHours = false;
    
  }

  if (needIncrementMinutes){
    needIncrementMinutes = false;
    Serial.println("inc Minutes");
    incrementMinutes();
    now = rtc.now();
    soft(now.hour(), now.minute(), now.second());
    delay(200);
    needIncrementMinutes = false;
    
  }
}

ISR (PCINT1_vect){ // прерывание изменения состояния выводов внопок и генератора секундныз импульсов
  if(SQW_IN){
    needUpdateTime = true;
  } else {
    needUpdateTime = false;
  }
  
  if (BTN_H){
    needIncrementHours = true;
  }
  
  if (BTN_M){
    needIncrementMinutes = true;
  }

}

ISR (TIMER2_OVF_vect){ // реализация ШИМ управления яркостью ламп
  PORTB = display[currentElement];
  PORTD = (1<<(currentElement + 2));
  if (++currentElement > 6) currentElement = 0;
  OCR2A = brightness[currentElement];
}

ISR (TIMER2_COMPA_vect){
   PORTD = 0x00;
   PORTB = 11;
}
