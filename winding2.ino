#include <LiquidCrystal_I2C.h> // Library for LCD
#include <Servo.h>
#include <Keypad.h>
#include <EEPROM.h>


LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4); // Change to (0x27,16,2) for 16x2 LCD.

/******************************
 * PINS
 *****************************/
////Mega 2560 wiring
const int PROG_PIN = 30;
const int OHM_MODE_PIN = 31;
const int GAUSS_MODE_PIN = 32;
const int PAUSE_PIN = 33;
const int ENABLE_SCATTER_PIN = 34;                          

const int GAUSS_VALUE_PIN = A1;
const int OHM_VALUE_PIN = 0;
const int OPTICAL_SENSOR = 42;
const int SCATTER_PIN = 12;

const int MOTOR_ENABLE = 13;
const int MOTOR_IN1 = 40;
const int MOTOR_IN2 = 41;
const int MOTOR_POT = A2;


//Mega 2560 pro wiring
//const int PROG_PIN = 12;
//const int OHM_MODE_PIN = 13;
//const int GAUSS_MODE_PIN = 14;
//const int PAUSE_PIN = 15;
//const int ENABLE_SCATTER_PIN = 16;                          
//
//
//const int GAUSS_VALUE_PIN = A1;
//const int OHM_VALUE_PIN = 0;
//const int OPTICAL_SENSOR = 32;
//const int SCATTER_PIN = 33;
//
//const int MOTOR_ENABLE = 10;
//const int MOTOR_IN1 = 8;
//const int MOTOR_IN2 = 9;
//const int MOTOR_POT = A2;


// CONFIG MOTORS
const int MAXRPM = 2000;
const int SERVO_SPEED = 12; //0.12 Sec/60 Degrees, type 12


//CONFIG GAUSS METER
#define NOFIELD 505L    // Analog output with no applied field, calibrate this
#define TOMILLIGAUSS 3756L  // A1302: 1.3mV=1Gaus, and 1024 analog steps = 5V, so 1 step = 3756mG

//CONFIG OHM METER
const int RESISTANCE = 9970;


//DON'T MODIFY WHAT FOLLOW

// EEPROM ADDRESSES
#define LEFT  0
#define RIGHT  1
#define TYPE 2
#define SPEED 3
#define RPM_H 4
#define RPM_L 5
#define DIRECTION 6

const int RANDOM = 0;
const int UNIFORM = 1;

//TRAVERSE SPEED
#define MIN_SPEED  1
#define MAX_SPEED  30

/****************************
 * OHM METER
 ***************************/
const long unsigned MIN_OHM = 500;
const long unsigned MAX_OHM = 50000;
const int VIN = 5;

/******************************************
 * Keypad initialisaztion
 *****************************************/
const int ROW_NUM = 4; //four rows
const int COLUMN_NUM = 4; //three columns

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte pin_rows[ROW_NUM] = {29, 28, 27, 26}; //connect to the row pinouts of the keypad
byte pin_column[COLUMN_NUM] = {25, 24, 23, 22}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

/****************************
 * SCATTER MOTOR
 ***************************/
Servo scatterMotor;

/*********************************
 * MENU DEFINITION
 *********************************/
const char IDLE_APP = '0';
const char ROUNDS_EDIT = 'A';
const char RPM_EDIT = 'B';
const char DIR_EDIT = 'C';
const char SCAT_MENU = 'D';

const char CW = 0;
const char CCW = 1;

/**************************
 * CUSTOM CHARACTERS
 **************************/
const int OHM_CHAR = 0; 
const int ISTO_OFF_CHAR = 1; 
const int ISTO_ON_CHAR = 2; 

byte ohmChar[] = {
  B01110,
  B10001,
  B10001,
  B10001,
  B01010,
  B01010,
  B11011,
  B00000
};

byte istoOffChar[] = {
  B10101,
  B01010,
  B10101,
  B01010,
  B10101,
  B01010,
  B10101,
  B00000
};

byte istoOnChar[] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B00000
};
/****************
 * APP STATE
 *****************/

struct State {
  long maxRPM = MAXRPM;
  long maxRounds = 0;
  char dir = CCW;
  long rounds = 0;
  
  char currentState = IDLE_APP;
  int programmingMode = LOW;
  int ohmMeterMode = LOW;
//  int inductanceMeterMode = LOW;
  int gaussMeterMode = LOW;
  bool boot = true;

  unsigned long prevScatterTs = 0;
  int scatterPos = 0;
  int scatterEnabled = HIGH;  
  int scatterType = UNIFORM;
  int scatterSpeed = MAX_SPEED;
  int leftLimit = 0;
  int rightLimit = 180;
  bool scatterAttached = false;
};

struct State state;

void switchState(char menuKey, struct State *state) {
  if (menuKey == RPM_EDIT || menuKey == ROUNDS_EDIT || menuKey == DIR_EDIT || menuKey == SCAT_MENU) {
    state->currentState = menuKey;
  }
}

/********************************
 * Settings
 */

void contextMenuDirection() {
  lcd.setCursor (0,2);
  lcd.print ("1=ccw 2=cw          ");
  lcd.setCursor(0,3);
  lcd.print ("#=confirm *=cancel  ");
}

void contextMenuScatter() {
  lcd.setCursor (0,2);
  lcd.print ("1=uniform 2=random  ");
  lcd.setCursor(0,3);
  lcd.print ("#=confirm *=cancel  ");
}

void clearContextMenuDirectionScatter() {
  lcd.setCursor (0,2);
  lcd.print ("                    ");
  lcd.setCursor(0,3);
  lcd.print ("                    ");
}

void countLoopContextMenu() {
  lcd.setCursor(0,3);
  lcd.print ("A=start D=exit");
}

char inputDirection (char d, int col, int row) {
  char exit = '#';
  char key = 'x';

  bool set = false;
  char output = d;

  contextMenuDirection();

  while (key!=exit) {
    lcd.setCursor(col,row); 
    lcd.blink();
    key = keypad.getKey() ;
      
    if (key == '1') {
      output = CCW;
      lcd.setCursor (col,row);
      lcd.print("ccw");
    }
    if (key == '2') {
      output = CW;
      lcd.setCursor (col,row);
      lcd.print("cw ");
    }

    if (key == '*') {
      lcd.noBlink();
      clearContextMenuDirectionScatter();
      return d;
    }
  }
  lcd.noBlink();
  clearContextMenuDirectionScatter();
  return output;
}


int inputScatter (int d, int col, int row) {
  char exit = '#';
  char key = 'x';

  bool set = false;
  int output = d;

  contextMenuScatter();

  while (key!=exit) {
    lcd.setCursor(col,row); 
    lcd.blink();
    key = keypad.getKey() ;
      
    if (key == '1') {
      output = UNIFORM;
      lcd.setCursor (col,row);
      lcd.print("uni ");
    }
    if (key == '2') {
      output = RANDOM;
      lcd.setCursor (col,row);
      lcd.print("rand");
    }

    if (key == '*') {
      lcd.noBlink();
      clearContextMenuDirectionScatter();
      return d;
    }
  }
  lcd.noBlink();
  clearContextMenuDirectionScatter();
  return output;
}

void contextMenuNumber() {
  lcd.setCursor(0,3);
  lcd.print ("#=confirm *=cancel  ");
}

void clearContextMenuNumber() {
  lcd.setCursor(0,3);
  lcd.print ("                    ");
}


void updatePosition(int position, int edge) {
  if (edge == LEFT) {
    lcd.setCursor(0,1);
    lcd.print("left:");
    lcd.setCursor(5,1);
    lcd.print((int)position);
  } else {
    lcd.setCursor(10,1);
    lcd.print("right:");
    lcd.setCursor(16,1);
    lcd.print((int)position);
  }
  lcd.print(" ");
}




int inputPosition(int def, int edge)  {
  
  scatterMotor.attach(SCATTER_PIN);
  unsigned long curTs = millis();
  unsigned long ts = curTs;
  char key = 'X';
  int val = def;
  int position = 0;
  while (key!='*' && key!='#') {
    key = keypad.getKey();
    
    curTs = millis();
    if (curTs - ts >100) {
      val = analogRead(MOTOR_POT);
      int normalisedVal = min(val,1000);
      normalisedVal = max(24,normalisedVal);
      if (edge == LEFT) {
        lcd.blink();
        position = ((float)normalisedVal-24)/(1000-24)*90;
        updatePosition(position, LEFT);
        lcd.setCursor(5,1);
      } else {
        lcd.blink();
        position = 90 + ((float)normalisedVal-24)/(1000-24)*90;
        updatePosition(position, RIGHT);
        lcd.setCursor(16,1);     
      }
      scatterMotor.write(180-position);
      ts = curTs;    
    }
  }

  scatterMotor.detach();
  lcd.noBlink();
  if (key == '*') return def;
  return position;
}

long inputNumberB(long d, int col, int row, int maxDigits) {
  long number = 0;
  char exit = '#';
  char key = 'x';
  int digits = 0;

  contextMenuNumber();
  
  while(key!=exit) {
    lcd.setCursor(col,row); 
    lcd.blink();
    
    key = keypad.getKey();
    if ((key == '1' || key == '2' || key == '3' || key == '4' || key == '5' || key == '6' || key == '7' || key == '8' || key == '9' || key == '0')) {
      if (digits < maxDigits) {
        number = (number * 10 + ((long) key - 48));
        digits++;
      }

      lcd.setCursor(col,row);
      for (int x=0; x<maxDigits; x++) {
        lcd.print(" ");
      }
      lcd.setCursor(col,row);
      lcd.print(number);
    }
    else if (key == '*') {
      lcd.noBlink();
      clearContextMenuNumber();
      return d;
    }
  }
  lcd.noBlink();
  clearContextMenuNumber();
  return number;
}

void readyScreen() {
  lcd.setCursor(0, 0);
  lcd.print("       Ready.       ");
  lcd.setCursor(0,1);
  lcd.print("Please,set the speed");
  lcd.setCursor(0,2);
  lcd.print("to zero and press D ");
  lcd.setCursor(0,3);
  lcd.print("when ready.         ");
}

void goToProgramScreen() {
  lcd.setCursor(0, 0); // Set the cursor on the first column and first row.
  lcd.print("   Program needed.  "); // Print the string "Hello World!"
  lcd.setCursor(0,1);
  lcd.print("Please, switch to   ");
  lcd.setCursor(0,2);
  lcd.print("program mode and set");
  lcd.setCursor(0,3);
  lcd.print("turns and max rpm.  ");
}

void splashScreen() {
  lcd.setCursor(0, 0); // Set the cursor on the first column and first row.
  lcd.print("JCC Winding Machine"); // Print the string "Hello World!"
  lcd.setCursor(4, 1); // Set the cursor on the first column and first row.
  lcd.print("version 1.0"); // Print the string "Hello World!"
  lcd.setCursor(3, 2); // Set the cursor on the first column and first row.
  lcd.print("-------------"); // Print the string "Hello World!"

  lcd.setCursor(0, 3); // Set the cursor on the first column and first row.
  lcd.print("(c) Lorenzo Iannone"); // Print the string "Hello World!"
  
  delay(2000);
  readyScreen();

}

void scatterMenuScreen(struct State *state, struct State *prevState) {

    if (prevState->currentState != SCAT_MENU) {
      lcd.clear();
    }

    lcd.setCursor(0,0);
    lcd.print("type:");
    if (state->scatterType == RANDOM) {
      lcd.print("rand ");
    } else {
      lcd.print("uni  ");
    }

    lcd.setCursor(11,0);
    lcd.print("speed:");
    lcd.print(state->scatterSpeed);
    lcd.print(" ");


    lcd.setCursor(0,1);
    lcd.print("left:");
    lcd.setCursor(5,1);
    lcd.print(state->leftLimit);
    lcd.print("  ");
    
    lcd.setCursor(10,1);
    lcd.print("right:");
    lcd.setCursor(16,1);
    lcd.print(state->rightLimit);
    lcd.print("  ");
    
    lcd.setCursor(0,2);
    lcd.print("A=type B=lt C=rt    ");
    lcd.setCursor(0,3);
    lcd.print("D=exit 0=cnt 8=speed");
}

void scatterMenuLoop(struct State *state, struct State *prevState) {
  scatterMenuScreen(state, prevState);  
  char key = 'x';
  while (key !='D' && state->programmingMode == HIGH) {
    state->programmingMode = digitalRead(PROG_PIN);
    key = keypad.getKey();
    int pos = 0;
    
    switch(key){
      case 'A':
        state->scatterType = inputScatter(state->scatterType, 5,0);
        scatterMenuScreen(state, prevState);  
        EEPROM.write(TYPE, state->scatterType);
        break;
      case 'B':
        pos = inputPosition(0,LEFT) ;
        state->leftLimit = pos;
        EEPROM.write(LEFT, pos);
        scatterMenuScreen(state, prevState);  
        break;
      case 'C':
        pos = inputPosition(0,RIGHT) ;
        state->rightLimit = pos;
        EEPROM.write(RIGHT, pos);
        scatterMenuScreen(state, prevState);  
        break;
      case '0':
        scatterMotor.attach(SCATTER_PIN);
        scatterMotor.write(90);
        scatterMenuScreen(state, prevState);  
        scatterMenuScreen(state, prevState);  
        break;
      case '8':
        state->scatterSpeed  = inputNumberB(state->scatterSpeed, 17,0,2);
        state->scatterSpeed = max(MIN_SPEED, state->scatterSpeed);
        state->scatterSpeed = min(MAX_SPEED, state->scatterSpeed);        
        EEPROM.write(SPEED, state->scatterSpeed);
        scatterMenuScreen(state, prevState);          
        break;
    }
  }
  scatterMotor.detach();
  lcd.clear();
}

  
void editWhenEditState(struct State *state, struct State *prevState) {
  if (state->currentState == ROUNDS_EDIT) {
      state->maxRounds = inputNumberB(state->maxRounds, 6,0,5);
      state->currentState = IDLE_APP;
      stateSummary(state);
  }

 if (state->currentState == RPM_EDIT) {
    state->maxRPM = inputNumberB(state->maxRPM, 16,0,4);
    int rpmH = state->maxRPM/100;
    int rpmL = state->maxRPM - rpmH*100;   
    EEPROM.write(RPM_H,rpmH);
    EEPROM.write(RPM_L,rpmL);
    state->currentState = IDLE_APP;
    stateSummary(state);
 }

 
 if (state->currentState == DIR_EDIT) {
    state->dir = inputDirection(state->dir, 6,1);
    EEPROM.write (DIRECTION, state->dir);
    state->currentState = IDLE_APP;
    stateSummary(state);
 }

 if (state->currentState == SCAT_MENU) {
    scatterMenuLoop(state, prevState);
    stateSummary(state);  
    state->currentState = IDLE_APP;
 }
}



void resetState() {
  state.maxRounds = 0;
  state.dir = CCW;
  state.maxRPM = MAXRPM;
  state.rounds = 0;
  state.prevScatterTs = 0;
  state.scatterPos = 0;
}

void stateSummary(State *state) {
  if (state->programmingMode == HIGH || state->currentState==IDLE_APP) {
    lcd.setCursor(0,0);
    lcd.print("turns:");
    lcd.setCursor(6,0);
    char r[32];
    sprintf(r, "%-5ld", state->maxRounds);
    lcd.print(r);
    
    lcd.setCursor(12,0);
    lcd.print("rpm:");
    char t[32];
    sprintf(t, "%-4ld", state->maxRPM);
    lcd.setCursor(16,0);
    lcd.print(t);

    lcd.setCursor(0,1);
    lcd.print("dir. :");
    lcd.setCursor(6,1);
    if (state->dir == CCW) {
      lcd.print("ccw");
    } else {
      lcd.print("cw ");
    }

    lcd.setCursor(0,2);
    lcd.print("A=turns B=rpm");
    lcd.setCursor(0,3);
    lcd.print("C=dir D=traverse");

  }
}

void ohmMeterScreen() {
   lcd.setCursor(6,1);
   lcd.print (" 0.00 K");
   lcd.write(OHM_CHAR);
   lcd.print("       ");
}


void gaussMeterScreen() {
   lcd.setCursor(6,1);
   lcd.print ("    0 G     ");
}

//void inductanceMeterScreen() {
//   lcd.setCursor(6,1);
//   lcd.print (" 0.00 H");
//   lcd.print("       ");
//}

void programmingLoop(struct State*state, struct State* prevState) {

    //if there is a transition in the programming mode pin then clear the screen to display the new state
    if (prevState->programmingMode == LOW || prevState->currentState == SCAT_MENU) {
      lcd.clear();
      stateSummary(state);
    }
    
    char key = keypad.getKey();

    switchState(key, state);
    editWhenEditState(state, prevState);  
}

void spinLoop(struct State*state, struct State* prevState) {
    if (prevState->programmingMode == HIGH || state->boot == true || prevState->ohmMeterMode == LOW) {
      if (state->boot == true) {
        state->boot = false;
      }
      if (state->maxRounds == 0 || state->maxRPM == 0) {
        goToProgramScreen();
      } else {
        lcd.clear();
        countLoop(state);
      }
    }
}

void countLoopScreen(int count, long maxRounds) {
    lcd.setCursor(8-(int)log10(count),1);
    lcd.print(count);
    lcd.print("/");
    lcd.print(maxRounds);
    lcd.print("     ");
}
void countIstogram(int count, long maxRounds) {
    long  on;
    on = 12 * (long)count / maxRounds;
    long  off;
    off = 12 - on;
    lcd.setCursor(4,2);
    for (int x=0; x<on; x++) {
      lcd.write(ISTO_ON_CHAR);  
    }
    for (int x=0; x<off; x++) {
      lcd.write(ISTO_OFF_CHAR);  
    }
}

int rampUpSpeed(unsigned long intialMillis, int speed) {
  unsigned long ts = millis();
  unsigned long rampSpeed = (ts-intialMillis) / 100;
  if (rampSpeed > speed) {
    return speed;
  }
  return (int)rampSpeed;
}


void spinMotor(struct State *state, unsigned long initialMillis) {
  if (digitalRead(PAUSE_PIN) == LOW) {
    if (state->dir == CW) {
      digitalWrite(MOTOR_IN1, LOW);
      digitalWrite(MOTOR_IN2, HIGH);
    } else {
      digitalWrite(MOTOR_IN1, HIGH);
      digitalWrite(MOTOR_IN2, LOW);    
    }
  
    int speed = analogRead(MOTOR_POT);
    speed = speed * state->maxRPM / MAXRPM;
    speed = speed * 0.3;
    speed = rampUpSpeed(initialMillis, speed);
    if (speed>255) speed = 255;
    analogWrite(MOTOR_ENABLE, speed );
  } else {
    analogWrite(MOTOR_ENABLE, 0 );
  }
 }

void countLoop(struct State *state) {
  
  int prevRead = HIGH; //prevOptState
  int count = 0;
  int upTransition = true;
  int highCount = 0;
  char key = 'x';
  
  scatterMotor.attach(SCATTER_PIN); // start servo control
  unsigned long initialMillis = millis();

  countLoopContextMenu();
  countLoopScreen(0,state->maxRounds);
  countIstogram(0,state->maxRounds);
  
  while (key!='A' && key!='D') {
    key = keypad.getKey();
    if(key=='D') {
      analogWrite(MOTOR_ENABLE, 0);
      return;
    }
  }

  int total = 0;
  int read = LOW;
  unsigned long ts = millis();
  while (key != 'D') {
    scatter(state);  
    spinMotor(state, initialMillis);
    read = digitalRead(OPTICAL_SENSOR);
    key = keypad.getKey();
    
    if (prevRead == LOW && read == HIGH) {
        upTransition = true;
    } else if (prevRead == HIGH && read == LOW){
        upTransition = false;
    } 
    if (upTransition == true & read == HIGH) {
        highCount++;
        if (highCount>40) {
          upTransition = false;
          count++;
        }
    }
      
    prevRead = read;
  
    unsigned long curTs =  millis();
    if (curTs - ts >100) {
      countLoopScreen(count, state->maxRounds);
      countIstogram(count,state->maxRounds);
      ts = curTs;
    }
    

    if (count >= state->maxRounds) {
      countLoopScreen(state->maxRounds, state->maxRounds);
      lcd.setCursor(4,2);
      lcd.print ("  complete  ");
      analogWrite(MOTOR_ENABLE, 0);
      resetState();
      scatterMotor.detach(); // start servo control
      while (keypad.getKey() !='D') {
        //nop
      }
      return;
    }
  }
  
 
  analogWrite(MOTOR_ENABLE, 0);
  resetState();
  scatterMotor.detach(); // start servo control  
}

//void inductanceMeterLoop(struct State *state, struct State* prevState) {
//  if (prevState->inductanceMeterMode == LOW || prevState->programmingMode == HIGH || prevState->ohmMeterMode == HIGH || prevState->gaussMeterMode == HIGH) {
//    lcd.clear(); 
//    inductanceMeterScreen(); 
//    }
// 
//    while (state->inductanceMeterMode == HIGH) {
//      state->inductanceMeterMode = digitalRead(INDUCTANCE_MODE_PIN);
//    }
//}


void gaussMeterLoop(struct State*state, struct State* prevState) {
  if (prevState->gaussMeterMode == LOW || prevState->programmingMode == HIGH || prevState->ohmMeterMode == HIGH) {
    lcd.clear(); 
    gaussMeterScreen();
  }  

  unsigned long ts = millis();
  
  while (state->gaussMeterMode == HIGH) {
    state->gaussMeterMode = digitalRead(GAUSS_MODE_PIN);
    unsigned long now = millis();
    if (now-ts >= 1000) {
        ts = now;
        // measure magnetic field
        int raw = analogRead(GAUSS_VALUE_PIN);
    
        long compensated = raw - NOFIELD;                 
        long gauss = compensated * TOMILLIGAUSS / 1000;   


        lcd.setCursor(6,1);
        lcd.print(gauss);
        lcd.print(" G");
        
        if (gauss > 0)     lcd.print(" South   ");
        else if(gauss < 0) lcd.print(" North   ");
        else               lcd.print("        ");
    }
  }
}

void ohmMeterLoop(struct State*state, struct State* prevState) {
  if (prevState->ohmMeterMode == LOW || prevState->programmingMode == HIGH || prevState->gaussMeterMode == HIGH) {
    lcd.clear(); 
    ohmMeterScreen();
  }


  int raw = 0;
  float Vout = 0;
  float R2 = 0;
  float buffer = 0;

  unsigned long samples = 0;
  unsigned long ohm = 0;
  unsigned long total = 0;
  
  while (state->ohmMeterMode == HIGH) {
    state->ohmMeterMode = digitalRead(OHM_MODE_PIN);
    raw = analogRead(OHM_VALUE_PIN);

    
      if(raw){
        buffer = raw * VIN;
        Vout = (buffer)/1024.0;
        buffer = (VIN/Vout) - 1;
        R2 = RESISTANCE * buffer;

        if (R2>MIN_OHM && R2<MAX_OHM) {          
          total = total+R2;
          samples ++;
          ohm = total/samples;
        } else {
          samples = 0;
          ohm = 0;
          total = 0;
        }
        
        lcd.setCursor(6,1);
        char u[32];
        sprintf(u, "%2d", (ohm/1000));
        lcd.print (u);
        lcd.print (".");
        char d[32];
        sprintf(d, "%02d", (ohm - ohm/1000*1000)/10);
        lcd.print(d);
        lcd.print(" K");   
        lcd.write(OHM_CHAR);   
        lcd.print("   ");   
    }    
  }
}

int nextStep = 0;
int direction = 1;
void scatter(struct State *state) {
  if (digitalRead(PAUSE_PIN) == LOW)  {
    //calculate the waiting interval, the motor does 
    float interval = (float)(state->rightLimit - state->leftLimit)/6* (SERVO_SPEED + 2); //5 is a bit of margin to cover motors inaccuracy
    
    state->scatterEnabled = digitalRead(ENABLE_SCATTER_PIN);
    if (state->scatterEnabled == HIGH) {
      if (state->scatterAttached == false) {
        scatterMotor.attach(SCATTER_PIN);
        state->scatterAttached = true;
      }
      unsigned long curTime = millis();
      int speed = 10;
  
      if (curTime - state->prevScatterTs > 40) {
        if (state->scatterType == RANDOM) {
          speed = random(state->scatterSpeed - 20, state->scatterSpeed + 20);
        } else {
          speed = state->scatterSpeed;
        }
        
        state->prevScatterTs = curTime;
        state->scatterPos = speed * direction + state->scatterPos;
        
        if (direction == 1) {        
          if (state->scatterPos >= state->rightLimit) {
            state->scatterPos = state->rightLimit;
            direction = -1 * direction;
          }     
        } else {
          if (state->scatterPos <= state->leftLimit) {
            state->scatterPos = state->leftLimit;
            direction = -1 * direction;
          }
        }
        
        scatterMotor.write(180-state->scatterPos);
  
      }
  
    } else {
      if (state->scatterAttached == true) {
        state->scatterAttached = false;
        scatterMotor.detach();
      }
    }
  }
}

int count = 0;
int p = LOW;
int r = LOW;


void setup() {
  pinMode (PROG_PIN, INPUT_PULLUP);
  pinMode (OHM_MODE_PIN, INPUT_PULLUP);
  pinMode (OPTICAL_SENSOR, INPUT_PULLUP);
  pinMode (PAUSE_PIN, INPUT_PULLUP);
  pinMode (GAUSS_MODE_PIN, INPUT_PULLUP);
  pinMode (ENABLE_SCATTER_PIN, INPUT_PULLUP);
  pinMode (MOTOR_IN1, OUTPUT);
  pinMode (MOTOR_IN2, OUTPUT); 
  pinMode (MOTOR_ENABLE, OUTPUT);
  
  Serial.begin(9600);
  Serial.println("Ready");                               

  //Initialise the scatter motor
  Serial.println("Positioning scatter motor");
  Serial.println("scatter motor ok");
  
  // Initiate the LCD:
  lcd.init();

  lcd.createChar(OHM_CHAR, ohmChar);
  lcd.createChar(ISTO_OFF_CHAR, istoOffChar);
  lcd.createChar(ISTO_ON_CHAR, istoOnChar);

  state.leftLimit = EEPROM.read(LEFT);
  if (state.leftLimit > 90) {
    state.leftLimit = 0;
  } 
  state.rightLimit = EEPROM.read(RIGHT);
  if (state.rightLimit < 90 || state.rightLimit > 180) {
    state.rightLimit = 180;
  }

  state.scatterType = EEPROM.read(TYPE);
  if (state.scatterType != UNIFORM && state.scatterType != RANDOM) {
    state.scatterType = UNIFORM;
  }

  state.scatterSpeed = EEPROM.read(SPEED);
  if (state.scatterType < MIN_SPEED && state.scatterSpeed > MAX_SPEED) {
    state.scatterSpeed = MAX_SPEED;
  } 

  int rpmH = EEPROM.read(RPM_H);
  int rpmL = EEPROM.read(RPM_L);
  state.maxRPM = 100*rpmH + rpmL;
  if(state.maxRPM < 0 && state.maxRPM > MAXRPM) {
    state.maxRPM = MAXRPM;
  }

  state.dir = EEPROM.read(DIRECTION);
  if (state.dir != CCW && state.dir!=CW) {
    state.dir = CCW;
  }

  randomSeed(analogRead(A0));
  
  lcd.backlight();
  splashScreen();
  lcd.clear();
}

void loop() {
  
  struct State prevState;
  memcpy (&prevState, &state, sizeof (struct State));

  state.programmingMode = digitalRead(PROG_PIN);
  state.ohmMeterMode = digitalRead(OHM_MODE_PIN);
  state.gaussMeterMode = digitalRead(GAUSS_MODE_PIN);

  if (state.programmingMode == HIGH ) {
    programmingLoop(&state, &prevState);
  }
  else if (state.ohmMeterMode == HIGH) {
    ohmMeterLoop(&state, &prevState);
  } else if (state.gaussMeterMode == HIGH) {
    gaussMeterLoop(&state, &prevState);
  }
  else {
    spinLoop(&state, &prevState);
    resetState();
  }
}                                                                                                                                                                                                                                         
