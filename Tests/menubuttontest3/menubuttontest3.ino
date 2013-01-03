#include <Time.h>
#include <TimeAlarms.h>

#define STATE_SHOWTIME 0
#define STATE_SETTIME 1
#define STATE_SETALARM 2
#define BLINK_FREQ 500
#define BTN_HOLD 2000

const int BTN_MENU = A3;
const int BTN_HR = A4;
const int BTN_MIN = A5;
int lastButtonMenu = LOW;  // the previous reading from the input pin
int lastButtonHour = LOW;  // the previous reading from the input pin
int lastButtonMinute = LOW;  // the previous reading from the input pin
const int d1=9,d2=A2,d3=A1,d4=A0;

int state = 0;
int hours = 0;
int mins = 0;
int incHoursAmount = 0;
int incMinsAmount = 0;

int blink_delay = 0;


/*
 * SETUP method
 */
void setup()
{
 Serial.begin(9600);
 DDRD=0xff; // all pins 0-7 OUTPUTs
 DDRB=0xff; // all pins 8-13 OUTPUTs even though we only use pins 8-12
 PORTD=0x00; // make pins 0-7 LOWs
 PORTB=0x00; // make pins 8-13 LOWs
 setTime(0,0,0,0,0,0);
 pinMode(d2, OUTPUT);
 pinMode(d3, OUTPUT);
 pinMode(d4, OUTPUT);
}

void toggleState() {
  state = (state + 1) % 3;
  if(state == 0) { // state switched to 0, set the alarm
    setAlarm();
  } else if(state == 1) { 
    blink_delay = millis();
  } else if(state == 2) {
    setTime(0, minute() + incHoursAmount, second() + incMinsAmount, day(), month(), year());
    incHoursAmount = 0;
    incMinsAmount = 0;
  }
  Serial.println("state switched!"); 
}

void incHours() {
  hours = (hours + 1) % 24;
  Serial.println("hours incremented"); 
}

void incMinutes() {
  mins = (mins + 1) % 60;
  Serial.println("minutes incremented"); 
}

void incTimeHours() {
  incHoursAmount = (incHoursAmount + 1) % 24;
  Serial.println("hours incremented"); 
}

void incTimeMinutes() {
  incMinsAmount = (incMinsAmount + 1) % 60;
  Serial.println("minutes incremented"); 
}

void setAlarm() {
  Alarm.alarmRepeat(0,hours,mins,alarmSound); //TODO: reset to hours and mins, instead of minutes and seconds
  Serial.print("Alarm set on: "); 
  Serial.print(hours); 
  Serial.print(":");
  Serial.print(mins); 
  Serial.print("\n");
}

void onButtonRisingEdge(int pin, int *lastButtonState, void (*onRisingEdge)()) {
  int buttonState = digitalRead(pin);
  if (buttonState != *lastButtonState) {
    Serial.println("button: "); 
    Serial.println(buttonState);  
    if(buttonState == HIGH) {
      (*onRisingEdge)();
    }
  }

  *lastButtonState = buttonState;
}

/*
 * Executes the method onLongPressed() when the button is 
 * hold longer than BTN_HOL ms
 */
void onLongPressed(int btnPin, void (*onLongPressed)()) {
    int buttonState = digitalRead(btnPin);
    int done = 0;
    time_t time0 = millis();
    time_t timeT = time0;
    while(buttonState && !done) {
      delay(100);
      timeT = millis();
      if(timeT - time0 >= BTN_HOLD) {
        done = 0;
        (*onLongPressed)();
      }
      buttonState = digitalRead(btnPin);
    }
}

/*
 * LOOP method
 */
void loop()
{
  int currentTime = millis();
  int delta_time = 0;
  Alarm.delay(0);
  // check for button state
  onButtonRisingEdge(BTN_MENU, &lastButtonMenu, &toggleState);
  switch(state) {
    case STATE_SHOWTIME:
      //show the time
      displayTime(minute(), second());
      break;
     case STATE_SETTIME:
      delta_time = currentTime - blink_delay;
      if (delta_time < BLINK_FREQ) {
        displayTime(minute() + incHoursAmount, second() + incMinsAmount);
      } else if (delta_time >= BLINK_FREQ && delta_time < BLINK_FREQ + 100){
         PORTD=B11111111;
         digitalWrite(8,HIGH);
      } else {
        blink_delay = currentTime;
      }
      onButtonRisingEdge(BTN_HR, &lastButtonHour, &incTimeHours);
      onButtonRisingEdge(BTN_MIN, &lastButtonMinute, &incTimeMinutes);
      break;
    case STATE_SETALARM:
      displayTime(hours, mins);
      onButtonRisingEdge(BTN_HR, &lastButtonHour, &incHours);
      onButtonRisingEdge(BTN_MIN, &lastButtonMinute, &incMinutes);
      break;
    default:
      displayTime(99,99); //error!
      break;
  }
}

void alarmSound() {
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

//  void setTimeState() {
//   state = STATE_SETTIME;
// }
