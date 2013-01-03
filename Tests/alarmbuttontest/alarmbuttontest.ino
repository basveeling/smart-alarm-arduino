#include <Time.h>
#include <TimeAlarms.h>
const int buttonSetPin = A5;

int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;  // the previous reading from the input pin

void setup()
{
 DDRD=0xff; // all pins 0-7 OUTPUTs
 DDRB=0xff; // all pins 8-13 OUTPUTs even though we only use pins 8-12
 PORTD=0x00; // make pins 0-7 LOWs
 PORTB=0x00; // make pins 8-13 LOWs
 Serial.begin(9600);
}
 
void loop()
{
  displayTime(minute(), second());
  int buttonState = digitalRead(buttonSetPin);
  if (buttonState != lastButtonState) {
    Serial.println("button: "); 
    Serial.println(buttonState);
    if(buttonState == HIGH) {
      Alarm.timerOnce(5, alarmSound);
      Serial.println("Set alarm! "); 
    }
  } 
  lastButtonState = buttonState;
}

void alarmSound(){
  
  Serial.println("Alarm triggered!"); 
  tone(13, 440, 500);
  Alarm.delay(100);
  noTone(13);
  tone(13, 523, 500);
  Alarm.delay(100);
  noTone(13);
  tone(13, 659, 500);
  Alarm.delay(100);
  noTone(13);
}

void displayTime(int hours, int minutes) {
  displayDigitOnSquare(hours / 10,1);
  displayDigitOnSquare(hours % 10, 2);
  displayDigitOnSquare(minutes / 10,3);
  displayDigitOnSquare(minutes % 10,4);
}

void displayDigitOnSquare(int num, int square) {
    turnOnSquare(square);
    displayDigit(num);
    Alarm.delay(0);
}
void displayDigit(int num)
{
   switch(num)
   {
     case 0:
      PORTD=B00000011; // pins 2-7 on
      digitalWrite(8,HIGH); // turn off pin 8
     break;
     case 1:
      PORTD=B11100111; // only pins 2 and 3 are on
      digitalWrite(8,HIGH); // turn off pin 8
     break;
     case 2:
      PORTD=B10010011; // only pins 2,3,5 and 6 on
      digitalWrite(8,LOW); // segment g on
     break;
     case 3:
      PORTD=B11000011; // only pins 2,3,4 and 5 on
      digitalWrite(8,LOW); // segment g on
     break;
     case 4:
      PORTD=B01100111; // only pins 3,4 and 7 on
      digitalWrite(8,LOW); // segment g on
     break;
     case 5:
      PORTD=B01001011; //B10110100; // only pins 2,4,5 and 7 on
      digitalWrite(8,LOW); // segment g on
     break;
     case 6:
      PORTD=B00001011; //B11110100; // only pins 2,4,5,6 and 7 on
      digitalWrite(8,LOW); // segment g on
     break;
     case 7:
      PORTD=B11100011; // only pins 2,3 and 4 on
      digitalWrite(8,HIGH); // segment g off
     break;
     case 8:
      PORTD=B00000011; // pins 2-7 on
      digitalWrite(8,LOW); // turn on pin 8
     break;
     case 9:
      PORTD=B01000011; // only pins 2,3, 4 and 5 on
      digitalWrite(8,LOW); // segment g on
     break;
   }
}

void turnOnSquare(int num)
{
int d1=9,d2=10,d3=11,d4=12;
 switch(num)
 {
   case 1:
    digitalWrite(d2,LOW);
    digitalWrite(d3,LOW);
    digitalWrite(d4,LOW);
    digitalWrite(d1,HIGH);
   break;
   case 2:
    digitalWrite(d1,LOW);
    digitalWrite(d3,LOW);
    digitalWrite(d4,LOW);
    digitalWrite(d2,HIGH);
   break;
   case 3:
    digitalWrite(d1,LOW);
    digitalWrite(d2,LOW);
    digitalWrite(d4,LOW);
    digitalWrite(d3,HIGH);
   break;
   case 4:
    digitalWrite(d1,LOW);
    digitalWrite(d2,LOW);
    digitalWrite(d3,LOW);
    digitalWrite(d4,HIGH);
   break;
   default:
   // this should never occur, but do what you want here
   break;
 }
 
}
