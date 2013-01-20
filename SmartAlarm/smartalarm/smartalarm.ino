#include <SPI.h>
#include <Time.h>
#include <TimeAlarms.h>

//CONSTANTS
#define STATE_SHOWTIME 0
#define STATE_SETTIME 1
#define STATE_SETALARM 2
#define BLINK_FREQ 500
#define BTN_HOLD 2000


// Pin constants
const int BTN_MENU_PIN = A3;
const int BTN_HR_PIN = A4;
const int BTN_MIN_PIN = A5;
const int SPEAKER_PIN = 3;
const int ALARM_LED_PIN = 4;
// Digit select pins
const int DIGIT_1 = 9, DIGIT_2 = A2, DIGIT_3 = A1, DIGIT_4 = A0;
// Chip select for SPI protocol
const int CHIP_SELECT_PIN = 10;

// Last readings for buttons
int last_button_menu = LOW;  // the previous reading from the input pin
int last_button_hour = LOW;  // the previous reading from the input pin
int last_button_minute = LOW;  // the previous reading from the input pin

//Alarm settings and values
int block_length = 30; // measure movement for 30 minutes.
int in_measure_block = false;
int hours = 0;
int mins = 0;
int inc_hours_amount = 0;
int inc_mins_amount = 0;
int blink_delay = 0;

//User interface State
int state = 0;

//Accelerometer registers, values and and pins
char POWER_CTL = 0x2D;  //Power Control Register
char DATA_FORMAT = 0x31;
char DATAX0 = 0x32; //X-Axis Data 0
char DATAX1 = 0x33; //X-Axis Data 1
char DATAY0 = 0x34; //Y-Axis Data 0
char DATAY1 = 0x35; //Y-Axis Data 1
char DATAZ0 = 0x36; //Z-Axis Data 0
char DATAZ1 = 0x37; //Z-Axis Data 1

//This buffer will hold values read from the ADXL345 registers.
char values[10];
//These variables will be used to hold the x,y and z axis accelerometer values.
int acc_x, acc_y, acc_z;
int last_x, last_y, last_z;
int acc_change;

/*
 * SETUP method
 */
void setup() {
    DDRD = DDRD | B11111100; //all pins 2-7 outputs
    pinMode(8, OUTPUT);
    pinMode(9, OUTPUT);
    PORTD = 0x00; // make pins 0-7 LOWs
    PORTB = 0x00; // make pins 8-13 LOWs
    pinMode(DIGIT_2, OUTPUT);
    pinMode(DIGIT_3, OUTPUT);
    pinMode(DIGIT_4, OUTPUT);
    pinMode(SPEAKER_PIN, OUTPUT);
    pinMode(ALARM_LED_PIN, OUTPUT);
    digitalWrite(ALARM_LED_PIN, LOW);
    setTime(0, 0, 0, 0, 0, 0);

    SPI.begin();
    SPI.setDataMode(SPI_MODE3);

    Serial.begin(9600);

    pinMode(CHIP_SELECT_PIN, OUTPUT);
    digitalWrite(CHIP_SELECT_PIN, HIGH);

    //Put the ADXL345 into +/- 2G range by writing the value 0x01 to the DATA_FORMAT register.
    writeRegister(DATA_FORMAT, 0x00);
    //Put the ADXL345 into Measurement Mode by writing 0x08 to the POWER_CTL register.
    writeRegister(POWER_CTL, 0x08);  //Measurement mode
}

/*
 * LOOP method
 */
void loop() {
    int currentTime = millis();
    int delta_time = 0;
    Alarm.delay(0);

    // Check for state button
    onButtonRisingEdge(BTN_MENU_PIN, &last_button_menu, &toggleState);
    //Execute the current state
    switch (state) {
    case STATE_SHOWTIME:
        //show the time
        displayTime(minute(), second());
        break;
    case STATE_SETTIME:
        delta_time = currentTime - blink_delay;
        if (delta_time < BLINK_FREQ) {
            displayTime(minute() + inc_hours_amount, second() + inc_mins_amount);
        } else if (delta_time >= BLINK_FREQ && delta_time < BLINK_FREQ + 100) {
            turnOnSquare(0);
        } else {
            blink_delay = currentTime;
        }
        onButtonRisingEdge(BTN_HR_PIN, &last_button_hour, &incTimeHours);
        onButtonRisingEdge(BTN_MIN_PIN, &last_button_minute, &incTimeMinutes);
        break;
    case STATE_SETALARM:
        displayTime(hours, mins);
        onButtonRisingEdge(BTN_HR_PIN, &last_button_hour, &incHours);
        onButtonRisingEdge(BTN_MIN_PIN, &last_button_minute, &incMinutes);
        break;
    default:
        displayTime(99, 99); //error!
        break;
    }

    // Measure accelerometer data if needed
    if (in_measure_block) {
        //ACCELEROMETER stuff
        //Reading 6 bytes of data starting at register DATAX0 will retrieve the x,y and z acceleration values from the ADXL345.
        //The results of the read operation will get stored to the values[] buffer.
        readRegister(DATAX0, 6, values);

        //The ADXL345 gives 10-bit acceleration values, but they are stored as bytes (8-bits). To get the full value, two bytes must be combined for each axis.
        //The X value is stored in values[0] and values[1].
        acc_x = ((int)values[1] << 8) | (int)values[0];
        //The Y value is stored in values[2] and values[3].
        acc_y = ((int)values[3] << 8) | (int)values[2];
        //The Z value is stored in values[4] and values[5].
        acc_z = ((int)values[5] << 8) | (int)values[4];

        acc_change = abs(last_x - acc_x) + abs(last_y - acc_y) + abs(last_z - acc_z);
        last_x = acc_x;
        last_y = acc_y;
        last_z = acc_z;

        // Print the results to the terminal.
        Serial.print(acc_x, DEC);
        Serial.print(',');
        Serial.print(acc_y, DEC);
        Serial.print(',');
        Serial.println(acc_z, DEC);
        Serial.println(acc_change);

        if (acc_change > 150) {
            alarmSound();
        }
    }
}

/*
 * Walk through the user interface states
 */
void toggleState() {
    state = (state + 1) % 3;
    if (state == 0) { // state switched to 0, set the alarm
        setAlarm();
    } else if (state == 1) {
        blink_delay = millis();
    } else if (state == 2) {
        setTime(0, minute() + inc_hours_amount, second() + inc_mins_amount, day(), month(), year());
        inc_hours_amount = 0;
        inc_mins_amount = 0;
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
    inc_hours_amount = (inc_hours_amount + 1) % 24;
    Serial.println("hours incremented");
}

void incTimeMinutes() {
    inc_mins_amount = (inc_mins_amount + 1) % 60;
    Serial.println("minutes incremented");
}
/*
 * Set the alarm to the current hours and minutes set by the user
 */
void setAlarm() {
    Alarm.alarmOnce(0, hours, mins, blockStart); //TODO: reset to hours and mins, instead of minutes and seconds
    Alarm.alarmOnce(0, hours + ((mins + block_length) / 60), (mins + block_length) % 60, alarmSound); //TODO: reset to hours and mins, instead of minutes and seconds
    Serial.print("Alarm set on: ");
    Serial.print(hours);
    Serial.print(":");
    Serial.print(mins);
    Serial.print("\n");

    digitalWrite(ALARM_LED_PIN, HIGH); //Laat zien dat het alarm ingesteld is
}

/*
 * This functions calls the onRisingEdge function 
 * whenever the input pin changes from low to high.
 * Requires a lastButtonState var inorder to keep in memory the last state of the button
 */
void onButtonRisingEdge(int pin, int *lastButtonState, void (*onRisingEdge)()) {
    int buttonState = digitalRead(pin);
    if (buttonState != *lastButtonState) {
        Serial.println("button: ");
        Serial.println(buttonState);
        if (buttonState == HIGH) {
            (*onRisingEdge)();
        }
    }

    *lastButtonState = buttonState;
}

/*
 * Is called when the measure block for the alarm has started
 */
void blockStart() {
    Serial.println("Engaging Block");
    in_measure_block = true;
}

/*
 * Is called when the measure block is over, or when the accelerometer has registered movement
 * Generates a simple alarm melody.
 */
void alarmSound() {
    if (in_measure_block) {
        in_measure_block = false;
        digitalWrite(ALARM_LED_PIN, LOW);
        Serial.println("Alarm triggered!");
        tone(SPEAKER_PIN, 440, 500);
        Alarm.delay(100);
        noTone(SPEAKER_PIN);
        tone(SPEAKER_PIN, 523, 500);
        Alarm.delay(100);
        noTone(SPEAKER_PIN);
        tone(SPEAKER_PIN, 659, 500);
        Alarm.delay(100);
        noTone(SPEAKER_PIN);
    }
}

/*
 * A wrapper function for the three functions below, 
 * accepts two integers in hours and minutes, 
 * and calculates the resulting digits for the display
 */
void displayTime(int hours, int minutes) {
    displayDigitOnSquare(hours / 10, 1);
    displayDigitOnSquare(hours % 10, 2);
    displayDigitOnSquare(minutes / 10, 3);
    displayDigitOnSquare(minutes % 10, 4);
}

/*
 * A wrapper function for the two functions below
 */
void displayDigitOnSquare(int num, int square) {
    turnOnSquare(square);
    displayDigit(num);
    Alarm.delay(0); //This makes sure that the value of the previous digit doesn't 'blend' into the next one
}

/*
 * This function sends a binary coded decimal to the BCD
 */
void displayDigit(int num) {
    switch (num) {
    case 0:
        digitalWrite(8, LOW); // turn off pin 8
        digitalWrite(7, LOW); // turn off pin 7
        digitalWrite(6, LOW); // turn off pin 6
        digitalWrite(5, LOW); // turn off pin 5
        break;
    case 1:
        digitalWrite(8, LOW); // turn off pin 8
        digitalWrite(7, LOW); // turn off pin 7
        digitalWrite(6, LOW); // turn off pin 6
        digitalWrite(5, HIGH); // turn ON pin 5
        break;
    case 2:
        digitalWrite(8, LOW); // turn off pin 8
        digitalWrite(7, LOW); // turn off pin 7
        digitalWrite(6, HIGH); // turn ON pin 6
        digitalWrite(5, LOW); // turn off pin 5
        break;
    case 3:
        digitalWrite(8, LOW); // turn off pin 8
        digitalWrite(7, LOW); // turn off pin 7
        digitalWrite(6, HIGH); // turn ON pin 6
        digitalWrite(5, HIGH); // turn ON pin 5
        break;
    case 4:
        digitalWrite(8, LOW); // turn off pin 8
        digitalWrite(7, HIGH); // turn ON pin 7
        digitalWrite(6, LOW); // turn off pin 6
        digitalWrite(5, LOW); // turn off pin 5
        break;
    case 5:
        digitalWrite(8, LOW); // turn off pin 8
        digitalWrite(7, HIGH); // turn ON pin 7
        digitalWrite(6, LOW); // turn off pin 6
        digitalWrite(5, HIGH); // turn ON pin 5
        break;
    case 6:
        digitalWrite(8, LOW); // turn off pin 8
        digitalWrite(7, HIGH); // turn ON pin 7
        digitalWrite(6, HIGH); // turn ON pin 6
        digitalWrite(5, LOW); // turn off pin 5
        break;
    case 7:
        digitalWrite(8, LOW); // turn off pin 8
        digitalWrite(7, HIGH); // turn ON pin 7
        digitalWrite(6, HIGH); // turn ON pin 6
        digitalWrite(5, HIGH); // turn ON pin 5
        break;
    case 8:
        digitalWrite(8, HIGH); // turn ON pin 8
        digitalWrite(7, LOW); // turn off pin 7
        digitalWrite(6, LOW); // turn off pin 6
        digitalWrite(5, LOW); // turn off pin 5
        break;
    case 9:
        digitalWrite(8, HIGH); // turn ON pin 8
        digitalWrite(7, LOW); // turn off pin 7
        digitalWrite(6, LOW); // turn off pin 6
        digitalWrite(5, HIGH); // turn ON pin 5
        break;
    }
}
/*
 * This function sets the digit select pin on the 7-segment display. 
 * An int 0 deselects all the pins, which is useful for blinking the display
 */
void turnOnSquare(int num) {
    switch (num) {
    case 0:
        digitalWrite(DIGIT_2, LOW);
        digitalWrite(DIGIT_3, LOW);
        digitalWrite(DIGIT_4, LOW);
        digitalWrite(DIGIT_1, LOW);
        break;
    case 1:
        digitalWrite(DIGIT_2, LOW);
        digitalWrite(DIGIT_3, LOW);
        digitalWrite(DIGIT_4, LOW);
        digitalWrite(DIGIT_1, HIGH);
        break;
    case 2:
        digitalWrite(DIGIT_1, LOW);
        digitalWrite(DIGIT_3, LOW);
        digitalWrite(DIGIT_4, LOW);
        digitalWrite(DIGIT_2, HIGH);
        break;
    case 3:
        digitalWrite(DIGIT_1, LOW);
        digitalWrite(DIGIT_2, LOW);
        digitalWrite(DIGIT_4, LOW);
        digitalWrite(DIGIT_3, HIGH);
        break;
    case 4:
        digitalWrite(DIGIT_1, LOW);
        digitalWrite(DIGIT_2, LOW);
        digitalWrite(DIGIT_3, LOW);
        digitalWrite(DIGIT_4, HIGH);
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
void writeRegister(char registerAddress, char value) {
    //Set Chip Select pin low to signal the beginning of an SPI packet.
    digitalWrite(CHIP_SELECT_PIN, LOW);
    //Transfer the register address over SPI.
    SPI.transfer(registerAddress);
    //Transfer the desired register value over SPI.
    SPI.transfer(value);
    //Set the Chip Select pin high to signal the end of an SPI packet.
    digitalWrite(CHIP_SELECT_PIN, HIGH);
}

//This function will read a certain number of registers starting from a specified address and store their values in a buffer.
//Parameters:
//  char registerAddress - The register addresse to start the read sequence from.
//  int numBytes - The number of registers that should be read.
//  char * values - A pointer to a buffer where the results of the operation should be stored.
void readRegister(char registerAddress, int numBytes, char *values) {
    //Since we're performing a read operation, the most significant bit of the register address should be set.
    char address = 0x80 | registerAddress;
    //If we're doing a multi-byte read, bit 6 needs to be set as well.
    if (numBytes > 1)address = address | 0x40;

    //Set the Chip select pin low to start an SPI packet.
    digitalWrite(CHIP_SELECT_PIN, LOW);
    //Transfer the starting register address that needs to be read.
    SPI.transfer(address);
    //Continue to read registers until we've read the number specified, storing the results to the input buffer.
    for (int i = 0; i < numBytes; i++) {
        values[i] = SPI.transfer(0x00);
    }
    //Set the Chips Select pin high to end the SPI packet.
    digitalWrite(CHIP_SELECT_PIN, HIGH);
}
