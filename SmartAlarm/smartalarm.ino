#include <SPI.h>
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

int blocklength = 30; // measure movement for 30 minutes.
int inMeasureBlock = false; 

int state = 0;
int hours = 0;
int mins = 0;
int incHoursAmount = 0;
int incMinsAmount = 0;

int blink_delay = 0;
//Assign the Chip Select signal to pin 10.
int CS=10;
char POWER_CTL = 0x2D;	//Power Control Register
char DATA_FORMAT = 0x31;
char DATAX0 = 0x32;	//X-Axis Data 0
char DATAX1 = 0x33;	//X-Axis Data 1
char DATAY0 = 0x34;	//Y-Axis Data 0
char DATAY1 = 0x35;	//Y-Axis Data 1
char DATAZ0 = 0x36;	//Z-Axis Data 0
char DATAZ1 = 0x37;	//Z-Axis Data 1

//This buffer will hold values read from the ADXL345 registers.
char values[10];
//These variables will be used to hold the x,y and z axis accelerometer values.
int accx,accy,accz;
int lastx,lasty,lastz;
int change;

/*
 * SETUP method
 */
void setup()
{
 DDRD = DDRD | B11111100; //all pins 2-7 outputs
 pinMode(8, OUTPUT);
 pinMode(9, OUTPUT);
 PORTD=0x00; // make pins 0-7 LOWs
 PORTB=0x00; // make pins 8-13 LOWs
 pinMode(d2, OUTPUT);
 pinMode(d3, OUTPUT);
 pinMode(d4, OUTPUT);
 setTime(0,0,0,0,0,0);
 
 SPI.begin();
 SPI.setDataMode(SPI_MODE3);
 
 Serial.begin(9600);
 
 pinMode(CS, OUTPUT);
 digitalWrite(CS, HIGH);
  
 //Put the ADXL345 into +/- 4G range by writing the value 0x01 to the DATA_FORMAT register.
 writeRegister(DATA_FORMAT, 0x00);
 //Put the ADXL345 into Measurement Mode by writing 0x08 to the POWER_CTL register.
 writeRegister(POWER_CTL, 0x08);  //Measurement mode  
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
  Alarm.alarmRepeat(0,hours,mins,blockStart); //TODO: reset to hours and mins, instead of minutes and seconds
  Alarm.alarmRepeat(0,hours +((mins+blocklength)/60),(mins+blocklength) % 60,alarmSound); //TODO: reset to hours and mins, instead of minutes and seconds
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
void loop() {
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
  if(inMeasureBlock) {
    //ACCELEROMETER stuff
    //Reading 6 bytes of data starting at register DATAX0 will retrieve the x,y and z acceleration values from the ADXL345.
    //The results of the read operation will get stored to the values[] buffer.
    readRegister(DATAX0, 6, values);

    //The ADXL345 gives 10-bit acceleration values, but they are stored as bytes (8-bits). To get the full value, two bytes must be combined for each axis.
    //The X value is stored in values[0] and values[1].
    accx = ((int)values[1]<<8)|(int)values[0];
    //The Y value is stored in values[2] and values[3].
    accy = ((int)values[3]<<8)|(int)values[2];
    //The Z value is stored in values[4] and values[5].
    accz = ((int)values[5]<<8)|(int)values[4];
    
    change = abs(lastx-accx) + abs(lasty-accy) + abs(lastz-accz);
    lastx = accx;
    lasty = accy;
    lastz = accz;

    //Print the results to the terminal.
    // Serial.print(accx, DEC);
    // Serial.print(',');
    // Serial.print(accy, DEC);
    // Serial.print(',');
    // Serial.println(accz, DEC);
    Serial.println(change);

    if(change > 100) {
      alarmSound();
    }
  }
}

void blockStart() {
  Serial.println("Engaging Block"); 
  inMeasureBlock = true;
}

void alarmSound() {
  inMeasureBlock = false;
  Serial.println("Alarm triggered!"); 
  // tone(SPEAKER, 440, 500);
  // Alarm.delay(100);
  // noTone(SPEAKER);
  // tone(SPEAKER, 523, 500);
  // Alarm.delay(100);
  // noTone(SPEAKER);
  // tone(SPEAKER, 659, 500);
  // Alarm.delay(100);
  // noTone(SPEAKER);

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

void displayDigit(int num) {
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

void turnOnSquare(int num) {
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

//This function will write a value to a register on the ADXL345.
//Parameters:
//  char registerAddress - The register to write a value to
//  char value - The value to be written to the specified register.
void writeRegister(char registerAddress, char value){
  //Set Chip Select pin low to signal the beginning of an SPI packet.
  digitalWrite(CS, LOW);
  //Transfer the register address over SPI.
  SPI.transfer(registerAddress);
  //Transfer the desired register value over SPI.
  SPI.transfer(value);
  //Set the Chip Select pin high to signal the end of an SPI packet.
  digitalWrite(CS, HIGH);
}

//This function will read a certain number of registers starting from a specified address and store their values in a buffer.
//Parameters:
//  char registerAddress - The register addresse to start the read sequence from.
//  int numBytes - The number of registers that should be read.
//  char * values - A pointer to a buffer where the results of the operation should be stored.
void readRegister(char registerAddress, int numBytes, char * values){
  //Since we're performing a read operation, the most significant bit of the register address should be set.
  char address = 0x80 | registerAddress;
  //If we're doing a multi-byte read, bit 6 needs to be set as well.
  if(numBytes > 1)address = address | 0x40;
  
  //Set the Chip select pin low to start an SPI packet.
  digitalWrite(CS, LOW);
  //Transfer the starting register address that needs to be read.
  SPI.transfer(address);
  //Continue to read registers until we've read the number specified, storing the results to the input buffer.
  for(int i=0; i<numBytes; i++){
    values[i] = SPI.transfer(0x00);
  }
  //Set the Chips Select pin high to end the SPI packet.
  digitalWrite(CS, HIGH);
}
