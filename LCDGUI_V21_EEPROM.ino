// Fish pond automation 
// Key features
// Automatic control of filteration system including the
// waterpump and the aerator units.
// Also allows control of dimlight for attracting insects
// User can set the start time, period and duration of the operation.
// User can set the real time clock to sync the operation with local time.

//Library files to include 
//#include <LiquidCrystal.h> // Includes liquid crystal display library
#include <LiquidCrystal_I2C.h> // Includes liquid crystal display library via I2C communication SDA SCL pins
#include <ezButton.h> // Includes eZ button library for debouncing
#include <Wire.h> // Includes wire library for I2C communication
#include <RTClib.h> // Includes Real Time Clock library
#include <EEPROM.h> // Enables EEPROM 1024 bytes of hard storage (writeable on the run)

//Harware Definition: MC pins
//SENSORS
#define MotionSensor A0 // Sensor digital input to Analog input PIN A0
#define VibrationSensor A1 // Sensor digital input Analog input PIN A1

#define Buzzer 3 
//RELAYS
#define PUMP  8 // Relay 1 control variable data out pin 8 (output)                      
#define AERATOR  9 // Relay 2 control variable data out pin 8 (output)                        
#define LIGHT  10 // Relay 3 control variable data out pin 8 (output)                        
#define FOOD  12 // Relay 4 control variable data out pin 8 (output)

//LED indicators
#define lcdBackLight 11 // controls backlight brightness

ezButton bSelect(5); // create ezButton(input) object that attach to pin 4;
ezButton bUp(4);  // create ezButton(input) object that attach to pin 5;
ezButton bDown(6);  // create ezButton(input) object that attach to pin 6;
ezButton bBack(7); // create ezButton(input) object that attach to pin 7;

RTC_DS3231 rtc; // Initialize RTC 3231 object

//LiquidCrystal lcd(13, 12, 11, 10, 9, 8); // with the arduino pin>> const int rs = 13, en = 12, d4 = 11, d5 = 10, d6 = 9, d7 = 8;
//LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // SET I2C Address 0x27 to connect LCD
//LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7);//, 3);//, POSITIVE);
LiquidCrystal_I2C lcd(0x27,16,2);
//SOFTWARE declarations
int pageCounter = 0;//to move between pages
int subPageCounter = 0;//Counter sub menu pages
boolean pageType = 0; // Main page = 0 and sub page = 1
boolean editFlag = 0; // Edit sub-page value when editFlag = 1
byte setMinute = 0; // 0: Set Hour, 1: Set Minute value, 2: AM/PM
byte arrowPosition = 0; // Set arrow position top line(0) or bottom line(1) of LCD
boolean timeOut = 0; // To set timeout condition for auto transition to home page
unsigned long previousTime = 0; //Timer to display clock and check for periodic device operation
boolean updateClock = 1; //Set condition to update the clock display time
unsigned long previousTimeOut = 0; // Timer to timeout the display brightness to low and to move to home screen
char *menuItems[] = {"MODE","TIME","PUMP","AERATOR","LIGHT", "FOOD", "MOTION SENS", "VIBR SEN"};
char *menuDevices[] = {PUMP, AERATOR, LIGHT, FOOD, MotionSensor, VibrationSensor};
#define menuSize 8 // Number of main pages = number of elements in menuItems[]
// parameter array for various equipments: 
//format of paraMatrix[PageCounter][SubPageCounter]
//{"START TIME HOUR","START TIME MINUTE","START TIME 1AM/0PM", "DURATION", "PERIOD", "FLAG enable/disable AUTO", "FLAG RELAY/SENSOR", "Equipment State Flag"} 
byte paraMatrix[8][8] = {{ 0, 0, 0, 0, 0, 0, 0, 0}, //MODE [PageCounter][SubPageCounter]
                         { 0, 0, 1, 0, 0, 0, 0, 0}, //TIME
                         { 8, 6, 1, 1, 24, 1, 1, 0}, //PUMP
                         { 11, 7, 1, 1, 1, 1, 1, 0}, //AERATOR
                         { 11, 8, 1, 1, 1, 0, 1, 0}, //LIGHT
                         { 12, 9, 0, 1, 1, 1, 1, 0}, //FOOD
                         { 12, 6, 0, 1, 1, 1, 0, 0}, // PIR motion sensor
                         { 12, 0, 0, 1, 1, 1, 0, 0} // vibration sensor
                         };
long timerBegin[8] = {0,0,0,0,0,0,0,0}; // Timer array for different paraMatrix rows
int subPageCount = 4; //How many submenu options to show = {"START TIME HOUR:MINUTE:1AM/0PM", "DURATION", "PERIOD", "FLAG enable/disable AUTO"} 
boolean modeValue; //Toggle switch for selection: Manual = 1 and Automatic = 0
long buzzerTime = 0;
int Intensity = 128;
int nextStartHour[8] = {0,0,0,0,0,0,0,0};
boolean powerFailReboot = 1;

void setup() {
  //Serial.print(sizeof(menuItems));
  bSelect.setDebounceTime(50); // set debounce time to 50 milliseconds
  bUp.setDebounceTime(50); // set debounce time to 50 milliseconds
  bDown.setDebounceTime(50); // set debounce time to 50 milliseconds    
  bBack.setDebounceTime(50); // set debounce time to 50 milliseconds    
   
  lcd.init();
  lcd.begin(16,2); // Initializes and clears the LCD screen
  lcd.clear();
  pinMode(lcdBackLight, OUTPUT);
  analogWrite(lcdBackLight, Intensity);
//  // Welcome screen on startup.
//  lcd.print("Auto Filter");
//  lcd.setCursor(0,1);
//  lcd.print("Ver.092020");
//  delay(500);
//  lcd.clear();
//  lcd.print("Author: Vineeth");
//  delay(500);
//  lcd.clear(); 
  Serial.begin(9600);

// initialize RTC
  rtc.begin();
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
//  if (rtc.lostPower()) {
//    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
//    Serial.println("RTC lost power, lets set the time!");
//    lcd.clear(); 
//    lcd.setCursor(0,0);
//    lcd.print("> CLOCK POWER LOSS");
//    lcd.setCursor(0,1);
//    lcd.print("> TIME SET TO DEFAULT");
//    delay(1000);
//  }
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  //rtc.adjust(DateTime(2020, 10, 11, 14, 59, 59));
   
  menuDisplay(0); // Display home page pageCounter = 0
  showClock(0);
  previousTime = millis();
  previousTimeOut = millis();
  buzzerTime = millis();
  // Configure digital outputs
  pinMode(Buzzer, OUTPUT);
  pinMode(PUMP, OUTPUT);   
  pinMode(AERATOR, OUTPUT);    
  pinMode(LIGHT, OUTPUT);    
  pinMode(FOOD, OUTPUT); 
  digitalWrite(PUMP, HIGH); // Relay off
  digitalWrite(AERATOR, HIGH);
  digitalWrite(LIGHT, HIGH);
  digitalWrite(FOOD, HIGH);
  
  // Sensor input pins config 
  pinMode(MotionSensor, INPUT_PULLUP);
  pinMode(VibrationSensor, INPUT_PULLUP);
  powerFailReboot = 1;
  readArrayFromEEPROM();
}

//EEPROM functions
void writeElementToEEPROM(int row, int column, byte elementValue){
  EEPROM.update(row*sizeof(paraMatrix[0]) + column, elementValue);  
  }

void writeArrayToEEPROM(){
  for (int i = 0; i < sizeof(paraMatrix)/sizeof(paraMatrix[0]); i++){  // row
    for (int j = 0; j < sizeof(paraMatrix[0]); j++){ //column
      EEPROM.update(i*sizeof(paraMatrix[0]) + j, byte(paraMatrix[i][j]));
      delay(5);
      //Serial.println(byte(paraMatrix[i][j]));
      }  
    }    
  }

int readElementFromEEPROM(int row, int column){
  int elementValue = EEPROM.read(row*sizeof(paraMatrix[0]) + column);  
  return(elementValue);
  }

//Read EEPROM array and store in Paramatrix
void readArrayFromEEPROM(){
  for (int i = 0; i < sizeof(paraMatrix)/sizeof(paraMatrix[0]); i++){  // row
    for (int j = 0; j < sizeof(paraMatrix[0]); j++){ //column
      paraMatrix[i][j] = EEPROM.read(i*sizeof(paraMatrix[0]) + j);
      }  
    }   
  }

// Display clock on LCD
void showClock(int localPageCounter){
  DateTime now = rtc.now();
  byte localHour = now.hour();
  String afterNoon = "PM";
  if (localHour > 12){localHour = localHour%12; afterNoon = "PM";}
  else if (localHour == 12){localHour = localHour; afterNoon = "PM";}
  else if (localHour == 0){localHour = 12; afterNoon = "AM";}
  else{afterNoon = "AM";}
  switch(localPageCounter){
    case 0:{ 
      if(localHour<9){
        lcd.setCursor(6,1);
        lcd.print("0");
        lcd.setCursor(7,1);
        lcd.print(localHour);
        }
      else{
        lcd.setCursor(6,1);
        lcd.print(localHour);
        }
      if(now.second()%2 == 0){lcd.print(":");}
      else{lcd.print(" ");}
      if(now.minute()<10){
        lcd.setCursor(9,1);
        lcd.print("0");
        lcd.setCursor(10,1);
        lcd.print(now.minute());
        lcd.print(afterNoon);
      }
      else{
        lcd.setCursor(9,1);
        lcd.print(now.minute());
        lcd.print(afterNoon);
      } 
    }
    break;
    case 1:{ 
      if(localHour<9){
        lcd.setCursor(6,0);
        lcd.print("0");
        lcd.setCursor(7,0);
        lcd.print(localHour);
        }
      else{
        lcd.setCursor(6,0);
        lcd.print(localHour);
        }
      if(now.second()%2 == 0){lcd.print(":");}
      else{lcd.print(" ");}
      if(now.minute()<10){
        lcd.setCursor(9,0);
        lcd.print("0");
        lcd.setCursor(10,0);
        lcd.print(now.minute());
        lcd.print(afterNoon);
      }
      else{
        lcd.setCursor(9,0);
        lcd.print(now.minute());
        lcd.print(afterNoon);
      }      
      }
    break;
    }
  }

//Showcase the main menu
void menuDisplay(int localPageCounter){
  switch(localPageCounter){
    case 0:{
      lcd.clear(); 
      lcd.setCursor(0,0);
      lcd.print(">");
      lcd.setCursor(1,0);
      lcd.print(menuItems[0]);//MODE AUTO/MANUAL
      lcd.setCursor(1,1);
      lcd.print(menuItems[1]);
      // Check if the control system is in auto or semi-auto or manual state
      if (paraMatrix[2][5]==1 && paraMatrix[3][5]==1 && paraMatrix[4][5]==1){
        lcd.setCursor(6,0);
        lcd.print("AUTO");
      }
      else if(paraMatrix[2][5]==0 && paraMatrix[3][5]==0 && paraMatrix[4][5]==0){
        lcd.setCursor(6,0);
        lcd.print("MANUAL");        
        }
      else{
      lcd.setCursor(6,0);
      lcd.print("SEMI AUTO");            
      }
    }
      break;
      
    case 1:{
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(">");
      lcd.setCursor(1,0);
      lcd.print(menuItems[1]);
      lcd.setCursor(1,1);
      lcd.print(menuItems[2]);
      if (paraMatrix[2][7] == 1){ //equipmentState[2]==1 &&
        //lcd.setCursor(14,1);
        lcd.print(" ON");
      }
      else{
        //lcd.setCursor(14,1);
        lcd.print(" OFF");
        }      
      } 
      break; 
        
    case 2:{
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(">");
      lcd.setCursor(1,0);
      lcd.print(menuItems[2]);
      if (paraMatrix[2][7]==1){ //equipmentState[2]==1 && 
        //lcd.setCursor(14,1);
        lcd.print(" ON");
      }
      else{
        //lcd.setCursor(14,1);
        lcd.print(" OFF");
        }
      lcd.setCursor(1,1);
      lcd.print(menuItems[3]); 
      if (paraMatrix[3][7]){
        //lcd.setCursor(14,1);
        lcd.print(" ON");
      }
      else{
        //lcd.setCursor(14,1);
        lcd.print(" OFF");
        }     
      } 
      break;
      
    case 3:{
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(">");
      lcd.setCursor(1,0);
      lcd.print(menuItems[3]);
      if (paraMatrix[3][7] ==1){ //equipmentState[3]==1 && 
        //lcd.setCursor(14,1);
        lcd.print(" ON");
      }
      else{
        //lcd.setCursor(14,1);
        lcd.print(" OFF");
        }
      lcd.setCursor(1,1);
      lcd.print(menuItems[4]);
      if (paraMatrix[4][7] == 1){ //equipmentState[4]==1 && 
        //lcd.setCursor(14,1);
        lcd.print(" ON");
      }
      else{
        //lcd.setCursor(14,1);
        lcd.print(" OFF");
        }      
      } 
      break; 
                 
    case 4:{
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(">");
      lcd.setCursor(1,0);
      lcd.print(menuItems[4]);
      if (paraMatrix[4][7] == 1){ //equipmentState[4]==1 && 
        //lcd.setCursor(14,1);
        lcd.print(" ON");
      }
      else{
        //lcd.setCursor(14,1);
        lcd.print(" OFF");
        }
      lcd.setCursor(1,1);
      lcd.print(menuItems[5]);
      if (paraMatrix[5][7] == 1){ //equipmentState[5]==1 && 
        //lcd.setCursor(14,1);
        lcd.print(" ON");
      }
      else{
        //lcd.setCursor(14,1);
        lcd.print(" OFF");
        }      
      } 
      break; 
        
    case 5:{
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(">");      
      lcd.setCursor(1,0);
      lcd.print(menuItems[5]);
      if (paraMatrix[5][7] == 1){ //equipmentState[5]==1 && 
        //lcd.setCursor(14,1);
        lcd.print(" ON");
      }
      else{
        //lcd.setCursor(14,1);
        lcd.print(" OFF");
        }
      lcd.setCursor(1,1);
      lcd.print(menuItems[6]);      
      if (paraMatrix[6][7] == 1){ //equipmentState[6]==1 &&
        //lcd.setCursor(14,1);
        lcd.print(" ON");
      }
      else{
        //lcd.setCursor(14,1);
        lcd.print(" OFF");
        }
    } 
      break;
    
    case 6:{
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(">");      
      lcd.setCursor(1,0);
      lcd.print(menuItems[6]);
      if (paraMatrix[6][7] == 1){ //equipmentState[6]==1 &&
        //lcd.setCursor(14,1);
        lcd.print(" ON");
      }
      else{
        //lcd.setCursor(14,1);
        lcd.print(" OFF");
        }
      lcd.setCursor(1,1);
      lcd.print(menuItems[7]);      
      if (paraMatrix[7][7] == 1){ //equipmentState[7]==1 && 
        //lcd.setCursor(14,1);
        lcd.print(" ON");
      }
      else{
        //lcd.setCursor(14,1);
        lcd.print(" OFF");
        }
    } 
      break;
    
   case 7:{
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(">");      
      lcd.setCursor(1,0);
      lcd.print(menuItems[7]);
      if (paraMatrix[7][7]== 1){ //equipmentState[7]==1 && 
        //lcd.setCursor(14,1);
        lcd.print(" ON");
      }
      else{
        //lcd.setCursor(14,1);
        lcd.print(" OFF");
        }
      lcd.setCursor(1,1);
      lcd.print(menuItems[0]);
      // Check if the control system is in auto or semi-auto or manual state
      lcd.setCursor(6,1);
      if (paraMatrix[2][5]==1 && paraMatrix[3][5]==1 && paraMatrix[4][5]==1){
        lcd.print("AUTO");
      }
      else if(paraMatrix[2][5]==0 && paraMatrix[3][5]==0 && paraMatrix[4][5]==0){
        lcd.print("MANUAL");        
        }
      else{
      lcd.print("SEMI AUTO");            
      }
    } 
      break;    
                            
  }
}// end Main page subroutine

void subMenuDisplay(int localPageCounter, int localSubPageCounter, byte arrowPosition){
  switch(localSubPageCounter){
    case 0:{  // Showcase the time here in format 12:59 AM
      lcd.clear(); 
      lcd.setCursor(0,arrowPosition);
      lcd.print(">");
      lcd.setCursor(1,0);
      lcd.print("Start:");
      // Display hours
      if (paraMatrix[localPageCounter][localSubPageCounter]>12){
        if(paraMatrix[localPageCounter][localSubPageCounter]%12<10){
          lcd.setCursor(1,1);
          lcd.print("0");
          lcd.setCursor(2,1);
          lcd.print(paraMatrix[localPageCounter][localSubPageCounter]%12);
        }
        else{
          lcd.setCursor(1,1);
          lcd.print(paraMatrix[localPageCounter][localSubPageCounter]%12);  
        }
      }
      else if (paraMatrix[localPageCounter][localSubPageCounter]== 0){
        lcd.setCursor(1,1);
        lcd.print("12");
      }
      else if(paraMatrix[localPageCounter][localSubPageCounter]<=12){
        if(paraMatrix[localPageCounter][localSubPageCounter]<10){
          lcd.setCursor(1,1);
          lcd.print("0");
          lcd.setCursor(2,1);
          lcd.print(paraMatrix[localPageCounter][localSubPageCounter]);
        }
        else{
          lcd.setCursor(1,1);
          lcd.print(paraMatrix[localPageCounter][localSubPageCounter]);  
        }        
      }            
      lcd.setCursor(3,1);
      lcd.print(":");

      // Display minutes
      if (paraMatrix[localPageCounter][localSubPageCounter+1]>=10){
        lcd.setCursor(4,1);
        lcd.print(paraMatrix[localPageCounter][localSubPageCounter+1]);//Set minutes
    }
      else{
        lcd.setCursor(4,1);
        lcd.print("0");
        lcd.setCursor(5,1);
        lcd.print(paraMatrix[localPageCounter][localSubPageCounter+1]);//Set minutes     
      }

      // Display AM PM   
      if (paraMatrix[localPageCounter][localSubPageCounter+2]==1){
        lcd.setCursor(7,1);
        lcd.print("AM");//Afternoon 
        //Serial.println("AM :" + String(paraMatrix[localPageCounter][localSubPageCounter+2]));
      }
      else{
        lcd.setCursor(7,1);
        lcd.print("PM");// Beforenoon
        //Serial.println("PM"+ String(paraMatrix[localPageCounter][localSubPageCounter+2]));       
      }    
    }
    break;

    case 1:{ // Showcase the duration of operation in hours
      lcd.clear(); 
      lcd.setCursor(0,arrowPosition);
      lcd.print(">");
      lcd.setCursor(1,0);
      lcd.print("Duration");
      lcd.setCursor(1,1);
      lcd.print(paraMatrix[localPageCounter][localSubPageCounter + 2]);//("00 h"); // set start time value here
      lcd.setCursor(4,1);
      lcd.print("Minutes");
      }
      break;

    case 2:{ // Showcase periodicity in hours
      lcd.clear(); 
      lcd.setCursor(0,arrowPosition);
      lcd.print(">");
      lcd.setCursor(1,0);
      lcd.print("Period");
      lcd.setCursor(1,1);
      lcd.print(paraMatrix[localPageCounter][localSubPageCounter + 2]);//("00 h"); // set start time value here
      lcd.setCursor(4,1);
      lcd.print("hour");
      }
      break;

    case 3:{ // Enable/disable the equipment
      lcd.clear(); 
      lcd.setCursor(0,arrowPosition);
      lcd.print(">");
      lcd.setCursor(1,0);
      lcd.print("Enable");
      lcd.setCursor(1,1);
      if(paraMatrix[localPageCounter][localSubPageCounter + 2] == 1){lcd.print("AUTO");}
      else{lcd.print("MANUAL");}     
      } 
      Serial.print("Enable: "+String(paraMatrix[localPageCounter][localSubPageCounter + 2]));
      break;      
    }//end of: switch  
}

//Subroutine maintaining subpage counter variable
void subPageRoutine(){
  //Sub Page down
  if (bDown.isReleased()){
    if (subPageCounter < subPageCount-1){
      subPageCounter = subPageCounter + 1;
      }
    else{
      subPageCounter = 0;
      }
    subMenuDisplay(pageCounter, subPageCounter,arrowPosition);  
  }//end of: if button released
//Sub Page up
  if (bUp.isReleased()){
    if (subPageCounter > 0){
      subPageCounter = subPageCounter - 1;
      }
    else{
      subPageCounter = subPageCount-1;
      }
    subMenuDisplay(pageCounter, subPageCounter,arrowPosition);
  }//end of: if button released  
}//end subPageRoutine()

// Subroutine to set start time and time in the format 12:59 AM
void setStartTime(int localPageCounter, int localSubPageCounter){
  int localHour = paraMatrix[localPageCounter][localSubPageCounter];
  int localMinute = paraMatrix[localPageCounter][localSubPageCounter+1];
  int localMeridian = paraMatrix[localPageCounter][localSubPageCounter+2];
  
  if (bSelect.isReleased()){
    switch(setMinute){
      case 0:{    
        lcd.noBlink();
        lcd.noCursor();
        lcd.setCursor(5,1);
        lcd.cursor();
        lcd.blink();
        delay(100);
        setMinute = 1;
        Serial.println("setMinute = " + String(setMinute));
        }
        break; 
      case 1:{   
        lcd.noBlink();
        lcd.noCursor();
        lcd.setCursor(8,1);
        lcd.cursor();
        lcd.blink();
        delay(100);
        setMinute = 2;
        Serial.println("setMinute = " + String(setMinute));
        }
        break;
      case 2:{   
        lcd.noBlink();
        lcd.noCursor();
        lcd.setCursor(1,1);
        lcd.cursor();
        lcd.blink();
        delay(100);
        setMinute = 0;
        Serial.println("setMinute = " + String(setMinute));
        }
        break;           
    }
  }  

  // Set 1AM-0PM in clock  
  if ((bUp.isReleased()||bDown.isReleased()) && setMinute == 2){
    if (paraMatrix[localPageCounter][localSubPageCounter+2] == 0){// if PM
      paraMatrix[localPageCounter][localSubPageCounter+2] =  1;// Set to AM
      if(paraMatrix[localPageCounter][localSubPageCounter]>12 && paraMatrix[localPageCounter][localSubPageCounter]<=23){
        paraMatrix[localPageCounter][localSubPageCounter]= paraMatrix[localPageCounter][localSubPageCounter]-12;
      }
      else if(paraMatrix[localPageCounter][localSubPageCounter]==12){
        paraMatrix[localPageCounter][localSubPageCounter] = 0;
      }
      else{
        //paraMatrix[localPageCounter][localSubPageCounter]= localHour;
        paraMatrix[localPageCounter][localSubPageCounter+2]= 1;
      }      
      Serial.println("AM "+ String(paraMatrix[localPageCounter][localSubPageCounter+2])+" "+String(paraMatrix[localPageCounter][localSubPageCounter]));// AM/PM of the clock is the paramatrix[..][2]
      
    }
    else{
      paraMatrix[localPageCounter][localSubPageCounter+2] = 0;//PM
      if(paraMatrix[localPageCounter][localSubPageCounter]<12 && paraMatrix[localPageCounter][localSubPageCounter]>0){
        paraMatrix[localPageCounter][localSubPageCounter]= paraMatrix[localPageCounter][localSubPageCounter] + 12;
      }
      else if(paraMatrix[localPageCounter][localSubPageCounter] == 0){
         paraMatrix[localPageCounter][localSubPageCounter]= 12;  
      }
      else{
         paraMatrix[localPageCounter][localSubPageCounter+2]= 0;
      }
      Serial.println("PM "+ String(paraMatrix[localPageCounter][localSubPageCounter+2])+" "+String(paraMatrix[localPageCounter][localSubPageCounter]));// AM/PM of the clock is the paramatrix[..][2]
    }
   subMenuDisplay(localPageCounter, localSubPageCounter,1);
   lcd.setCursor(8,1);
  }

// Set hours
  if (localMeridian == 0){//PM Afternoon
    // Set hours
    if (bUp.isReleased() && setMinute == 0){
      if (localHour < 23){
       localHour = localHour + 1;
      }
      else{
       localHour = 12;
      }
      Serial.println("Hour: "+ String(localHour));
      paraMatrix[localPageCounter][localSubPageCounter]= localHour;    
      subMenuDisplay(localPageCounter, localSubPageCounter,1);
      lcd.setCursor(1,1);
    }
  
    if (bDown.isReleased()&& setMinute == 0){
      if (localHour > 12){
        localHour = localHour - 1;
        }
      else{
        localHour = 23;
        }
      Serial.println("Hour: "+ String(localHour));
      paraMatrix[localPageCounter][localSubPageCounter]= localHour;    
      subMenuDisplay(localPageCounter, localSubPageCounter,1);
      lcd.setCursor(1,1); 
    }
  }
  else{
    // Set hours
    if (bUp.isReleased() && setMinute == 0){
      if (localHour < 12){
       localHour = localHour + 1;
      }
      else{
       localHour = 1;
      }
      Serial.println("Hour: "+ String(localHour));
      paraMatrix[localPageCounter][localSubPageCounter]= localHour;    
      subMenuDisplay(localPageCounter, localSubPageCounter,1);
      lcd.setCursor(1,1);
    }
  
    if (bDown.isReleased()&& setMinute == 0){
      if (localHour > 1){
        localHour = localHour - 1;
        }
      else{
        localHour = 12;
        }
      Serial.println("Hour: "+ String(localHour));
      paraMatrix[localPageCounter][localSubPageCounter]= localHour;    
      subMenuDisplay(localPageCounter, localSubPageCounter,1);
      lcd.setCursor(1,1); 
    }
  }

  // Set minutes in clock  
  if (bUp.isReleased() && setMinute == 1){
    if (localMinute < 59){
      localMinute = localMinute + 1;
      }
    else{
      localMinute = 0;
      }
   Serial.println("Minute: "+ String(localMinute));
   paraMatrix[localPageCounter][localSubPageCounter+1]= localMinute; // Minutes of the clock is the paramatrix[..][1]
   subMenuDisplay(localPageCounter, localSubPageCounter,1);
   lcd.setCursor(5,1);
  }
  if (bDown.isReleased()&& setMinute == 1){
    if (localMinute > 0){
      localMinute = localMinute - 1;
      }
    else{
      localMinute = 59;
      }
   Serial.println("Minute "+ String(localMinute));
   paraMatrix[localPageCounter][localSubPageCounter+1]= localMinute;// Minutes of the clock is the paramatrix[..][1]
   subMenuDisplay(localPageCounter, localSubPageCounter,1);
   lcd.setCursor(5,1);
  }       
}

// Subroutine to set time durations like duration, period etc.
void setTimeDuration(int localPageCounter, int localSubPageCounter){
  int localTimeDuration = paraMatrix[localPageCounter][localSubPageCounter+2];  
  // Set hours
  if (bUp.isReleased()){
    if (localTimeDuration < 1440){ //Total minutes in a day
     localTimeDuration = localTimeDuration + 1;
    }
    else{
     localTimeDuration = 1;
    }
    Serial.println("Hour: "+ String(localTimeDuration));
    paraMatrix[localPageCounter][localSubPageCounter+2]= localTimeDuration;
    subMenuDisplay(localPageCounter, localSubPageCounter,1);
    lcd.setCursor(1,1);
  }
  if (bDown.isReleased()){
    if ( localTimeDuration > 1){
      localTimeDuration = localTimeDuration - 1;
      }
    else{
      localTimeDuration = 1440;
      }
    Serial.println("Duration hour: "+ String(localTimeDuration));
    paraMatrix[localPageCounter][localSubPageCounter+2]= localTimeDuration;
    subMenuDisplay(localPageCounter, localSubPageCounter,1);
    lcd.setCursor(1,1); 
  }
}

// Subroutine to toggle 0/1
void enableDisable(int localPageCounter, int localSubPageCounter){
  if ((bUp.isReleased()||bDown.isReleased())){
    if (paraMatrix[localPageCounter][localSubPageCounter+2] == 0){ // Add 2 to address the specific Paramatrix parameter corresponding to Enable/Disable
      paraMatrix[localPageCounter][localSubPageCounter+2] = 1;  
    }
    else{
      paraMatrix[localPageCounter][localSubPageCounter+2] = 0;
      }  
  Serial.print("Auto:" + String(paraMatrix[localPageCounter][localSubPageCounter+2]));
  subMenuDisplay(localPageCounter, localSubPageCounter,1);
  }
}

// Subroutine to edit subpages/submenu values
void subMenuEdit(int localPageCounter, int localSubPageCounter){  
  switch(localSubPageCounter){
    case 0:{
      setStartTime(localPageCounter, localSubPageCounter);// Set time    
      }
      break; 
    case 1:{
      setTimeDuration(localPageCounter, localSubPageCounter);//Set duration     
      }
      break; 
    case 2:{
      setTimeDuration(localPageCounter, localSubPageCounter);//Set period     
      }
      break;
    case 3:{
      enableDisable(localPageCounter, localSubPageCounter);//Set period     
      }
      break; 
  } 
}

void timeOutHome(){
  analogWrite(lcdBackLight, 0);
  pageType = 0;
  subPageCounter = 0;
  pageCounter = 0;
  editFlag = 0;
  lcd.noBlink();
  lcd.noCursor();
  menuDisplay(pageCounter);
  showClock(pageCounter);
  }

// Main loop
void loop() {
  bSelect.loop(); // MUST call the loop() function first
  bUp.loop(); // MUST call the loop() function first
  bDown.loop(); // MUST call the loop() function first
  bBack.loop(); // MUST call the loop() function first

  if((pageCounter ==0 || pageCounter ==1)&&pageType==0 && millis()- previousTime >800 && updateClock){
    showClock(pageCounter);
    //Serial.println("ShowClock");
    updateClock = 0;   
  }
  //Page down 
  if (bDown.isReleased() && pageType == 0){
    analogWrite(lcdBackLight, Intensity);
    timeOut = 0;
    previousTimeOut = millis();
    if (pageCounter < menuSize-1){
      pageCounter = pageCounter + 1;
      }
    else{
      pageCounter = 0;//pageCounter%menuSize;
      }
    menuDisplay(pageCounter);  
  }//end of: if button released
  
  //Main Page up
  if (bUp.isReleased() && pageType == 0){
    timeOut = 0;
    analogWrite(lcdBackLight, Intensity);
    previousTimeOut = millis();
    if (pageCounter > 0){
      pageCounter = pageCounter - 1;
      }
    else{
      pageCounter = menuSize-1;
      }
    menuDisplay(pageCounter);
  }//end of: if button released

  //back to home page from any main pages
  if(bBack.isPressed()&& editFlag == 0 && pageType ==0 && pageCounter!=0){
    timeOut = 0;
    analogWrite(lcdBackLight, Intensity);
    timeOut = 0;
    previousTimeOut = millis();
    pageCounter = 0;
    menuDisplay(pageCounter);  
  }

  // back to main page from subpage
  if (bBack.isReleased()&& editFlag == 0){
    timeOut = 0;
    analogWrite(lcdBackLight, Intensity);
    previousTimeOut = millis();
    timeOut = 0;
    pageType = 0;
    subPageCounter = 0;
    lcd.noBlink();
    lcd.noCursor();
    menuDisplay(pageCounter);
    //Serial.print("back");
    }

// Back to view mode from edit mode
  if (bBack.isReleased()&& editFlag == 1){
    timeOut = 0;
    analogWrite(lcdBackLight, Intensity);
    previousTimeOut = millis(); 
    timeOut = 0;
    pageType = 1;
    editFlag = 0;
    lcd.noBlink();
    lcd.noCursor();
    subMenuDisplay(pageCounter, subPageCounter,arrowPosition);
    writeElementToEEPROM(pageCounter, subPageCounter,paraMatrix[pageCounter][subPageCounter]);    
    if(pageCounter == 1){
      rtc.adjust(DateTime(2020, 10, 9,int(paraMatrix[1][0]), paraMatrix[1][1], 0));
    }  
  }
 
   //Time out to back to home page and switchoff backlight 
  if((millis()-previousTimeOut > 60000)&& (timeOut==0)){  //&& (timeOut==0)
    timeOutHome();
    timeOut = 1;
    Serial.println("time out " + String(millis()-previousTimeOut));
    previousTimeOut = millis();
  }

    
  // Explore sub page/menu of all pages except page = 1: MODE
  if (bSelect.isReleased() && pageType == 0 && pageCounter != 0){//&& pageCounter == 2){
    analogWrite(lcdBackLight, Intensity);
    timeOut = 0;
    previousTimeOut = millis();
    pageType = 1;
    subMenuDisplay(pageCounter, subPageCounter,arrowPosition);
    delay(500);
  } 

  if (bSelect.isReleased() && pageType == 0 && pageCounter == 0){
    analogWrite(lcdBackLight, Intensity);
    timeOut = 0;
    previousTimeOut = millis();
  }
  
  if (pageType == 1 && editFlag == 0 && pageCounter != 0){  // subpage viewer
    //Serial.println(String(pageType)+","+String(editFlag));
    subPageRoutine();// counts subpage in subPageRoutine
    if (bSelect.isPressed()){   // bSelect.isPressed() is used to avoid accidental detection of bSelect.isReleased()
      //delay(500);
      timeOut = 0;
      analogWrite(lcdBackLight, Intensity);
      previousTimeOut = millis();
      editFlag = 1; // Lets you edit subpage menu options
      Serial.println(String(pageCounter)+","+ String(subPageCounter)+","+String(editFlag));
      setMinute = 2;
      subMenuDisplay(pageCounter, subPageCounter,1);
      }
  }

  if (pageType == 1 && editFlag == 1 && pageCounter != 0){
    subMenuEdit(pageCounter, subPageCounter);
    timeOut = 0;
    analogWrite(lcdBackLight, Intensity);
  }
  
  // Check time for operating equipments every 1000 ms.
  if(millis()-previousTime > 1000){
    DateTime now = rtc.now();
                
      for (int i = 2; i <= 6; i++) {        
          if (paraMatrix[i][7] == 0 && powerFailReboot == 1){
            if((paraMatrix[i][0] - now.hour())< 0){
                paraMatrix[i][0] = paraMatrix[i][0] + paraMatrix[i][4];
                if(paraMatrix[i][0]>=24){paraMatrix[i][0] = paraMatrix[i][0]%24;}
                if(paraMatrix[i][0]>=12){paraMatrix[i][2] = 0;}
                else{paraMatrix[i][2] = 1;}
                Serial.println("Next start time of "+ String(menuItems[i])+ " is " + String(paraMatrix[i][0])+":" + String(paraMatrix[i][1]) +":"+ String(paraMatrix[i][2]));  
            }
            else if(((paraMatrix[i][0] - now.hour())== 0)&& paraMatrix[i][1]< now.minute()){
              paraMatrix[i][0] = paraMatrix[i][0] + paraMatrix[i][4];
              if(paraMatrix[i][0]>=24){paraMatrix[i][0] = paraMatrix[i][0]%24;}
              if(paraMatrix[i][0]>=12){paraMatrix[i][2] = 0;}
              else{paraMatrix[i][2] = 1;}
              Serial.println("Next start time of "+ String(menuItems[i])+ " is " + String(paraMatrix[i][0])+":" + String(paraMatrix[i][1]) +":"+ String(paraMatrix[i][2])); 
            }
            else if((paraMatrix[i][0] - now.hour())> paraMatrix[i][4]){
              paraMatrix[i][0] = paraMatrix[i][0] - paraMatrix[i][4];             
              if(paraMatrix[i][0]>=24){paraMatrix[i][0] = paraMatrix[i][0]%24;}
              if(paraMatrix[i][0]>=12){paraMatrix[i][2] = 0;}
              else{paraMatrix[i][2] = 1;}
              Serial.println("Next start time of "+ String(menuItems[i])+ " is " + String(paraMatrix[i][0])+":" + String(paraMatrix[i][1]) +":"+ String(paraMatrix[i][2])); 
            }
        }
          
        if(paraMatrix[i][5] == 1){  // Check if the device is in Auto mode == 1 
          if((editFlag == 0)&&(paraMatrix[i][0] == now.hour()) && (paraMatrix[i][1] == now.minute()) ){
            if(paraMatrix[i][7] == 0){ // Check for the equipment state. Allow if OFF
              Serial.println("start machine i = "+String(i)+" "+menuItems[i]); 
              paraMatrix[i][7] = 1; // Set equipment state ON/OFF 
              powerFailReboot = 0;    
              timerBegin[i] = millis();
              if(paraMatrix[i][6]==1){ // If the device is a Relay or a sensor
                digitalWrite(menuDevices[i-2], LOW); // Turn the Relay ON
              }
              if(pageCounter == i || pageCounter == i-1 ){ // Update the device status in the menu page in real time
                menuDisplay(pageCounter);                 
              }              
            }
          }

          ////Read the sensor value(0) and is auto enabled(1)
          if(paraMatrix[i][6]==0 && paraMatrix[i][7] == 1){
            Serial.println("Sensor " + String(menuItems[i])+ ": " + digitalRead(menuDevices[i-2])); // Read the sensor value                         
            if(digitalRead(menuDevices[i-2])){
              digitalWrite(menuDevices[2], LOW); // Turn the Relay ON
              digitalWrite(Buzzer, HIGH);
              }
            else{
              digitalWrite(menuDevices[2], HIGH); // Turn the Relay OFF
              digitalWrite(Buzzer, LOW);
              }                           
            }
         
          // Check condition when to stop the device by checking the timerBegin array values for each device.
          if((millis()-timerBegin[i] >= paraMatrix[i][3]*60000)&& paraMatrix[i][7] == 1){ //(equipmentState[i] == 1) &&
            paraMatrix[i][7] = 0;
            if(paraMatrix[i][6]==1){// If the device is a Relay
            digitalWrite(menuDevices[i-2], HIGH); // Turn the Relay OFF
            }
            timerBegin[i] = 0;
            Serial.println("Stop Machine = "+String(i)+" "+menuItems[i]);

            // set next start time
            paraMatrix[i][0] = paraMatrix[i][0] + paraMatrix[i][4];
            if(paraMatrix[i][0]>=24){paraMatrix[i][0] = paraMatrix[i][0]%24;}
            if(paraMatrix[i][0]>=12){
              paraMatrix[i][2] = 0;
              }
            else{
              paraMatrix[i][2] = 1;
              }              
            Serial.println("Next start time of "+ String(menuItems[i])+ " is "+String(paraMatrix[i][0])+":" + String(paraMatrix[i][1]));          
           
            if(pageCounter == i || pageCounter == i-1 ){
              menuDisplay(pageCounter);                 
              }            
          digitalWrite(Buzzer, LOW);
          digitalWrite(menuDevices[2], HIGH); // Turn the Relay OFF
          } 
        
        }//End of check auto enable/disable
        else{
          if(paraMatrix[i][5]==0){
            digitalWrite(menuDevices[i-2], HIGH); // Turn OFF relay
            digitalWrite(Buzzer, LOW);
            //digitalWrite(menuDevices[2], HIGH); // Turn the Relay OFF
          }
          paraMatrix[i][7] = 0;          
        }          
      }//end For loop       
    previousTime = millis();
    updateClock = 1;
  } // timeout condition to check device operations 
}//End of void loop




//{"START TIME HOUR","START TIME MINUTE","START TIME 1AM/0PM", "DURATION", "PERIOD", "FLAG enable/disable AUTO", "FLAG RELAY/SENSOR"} 
//byte paraMatrix[8][7] = {{ 0, 0, 0, 0, 0, 0, 0}, //MODE [PageCounter][SubPageCounter]
//                         { 0, 0, 1, 0, 0, 0, 0}, //TIME
//                         { 23, 05, 0, 1, 1, 1, 1 }, //PUMP
//                         { 23, 05, 0, 1, 1, 1, 1 }, //AERATOR
//                         { 23, 05, 0, 1, 1, 1, 1 }, //LIGHT
//                         { 23, 05, 0, 1, 1, 1, 1 }, //FOOD
//                         { 23, 05, 0, 1, 1, 1, 0 }, // PIR motion sensor
//                         { 23, 05, 0, 1, 1, 1, 0 } // vibration sensor
//                         };
