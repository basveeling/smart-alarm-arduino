int BTN_MENU = A3;
int BTN_HR = A4;
int BTN_MIN = A5;

int selectedMenu = 0;
int hours = 0;
int mins = 0;


if(digitalRead(BTN_MENU)) {
 selectMenu(); 
}

void selectMenu() {
  while(!digitalRead(BTN_MENU)) {
   selectedMenu = (selectedMenu + 1) % 2;
   if(digitalRead(BTN_HR)) {
       hours++;
   }
   else if(digitalRead(BTN_MIN)) {
     mins++;
   }
   displayTime(hours, mins);
  }
  Alarm.alarmRepeat(0,hours,mins, MorningAlarm); 
 
}
