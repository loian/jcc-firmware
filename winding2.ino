#include <LiquidCrystal_I2C.h> // Library for LCD
#include <Servo.h>
#include <Keypad.h>


LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4); // Change to (0x27,16,2) for 16x2 LCD.

/******************************
 * PINS
 *****************************/
const int PROG_PIN = 30;
const int OHM_MODE_PIN = 31;
const int INDUCTANCE_MODE_PIN = 32;
const int GAUSS_MODE_PIN = 33;
const int ENABLE_SCATTER_PIN = 34;
const int OHM_VALUE_PIN = 0;
const int OPTICAL_SENSOR = 3;
const int SCATTER_PIN = 8;

/****************************
 * OHM METER
 */

const int RESISTANCE = 9970;
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
  long maxRPM = 3000;
  long maxRounds = 0;
  char dir = CW;
  long rounds = 0;
  
  char currentState = IDLE_APP;
  int programmingMode = LOW;
  int ohmMeterMode = LOW;
  int inductanceMeterMode = LOW;
  int gaussMeterMode = LOW;
  bool boot = true;

  unsigned long prevScatterTs = 0;
  int scatterPos = 0;
  int scatterEnabled = HIGH;
  
};

struct State state;

void switchState(char menuKey, struct State *state) {
  if (menuKey == RPM_EDIT || menuKey == ROUNDS_EDIT || menuKey == DIR_EDIT) {
    state->currentState = menuKey;
  }
}

/********************************
 * Settings
 */

void contextMenuDirection() {
  lcd.setCursor (0,2);
  lcd.print ("1=cw 2=ccw");
  lcd.setCursor(0,3);
  lcd.print ("#=confirm *=cancel  ");
}

void clearContextMenuDirection() {
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
      output = CW;
      lcd.setCursor (col,row);
      lcd.print("cw ");
    }
    if (key == '2') {
      output = CCW;
      lcd.setCursor (col,row);
      lcd.print("ccw");
    }

    if (key == '*') {
      lcd.noBlink();
      clearContextMenuDirection();
      return d;
    }
  }
  lcd.noBlink();
  clearContextMenuDirection();
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
        Serial.println(number);
        digits++;
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
  lcd.print("the number of turns.");
}

void splashScreen() {
  lcd.setCursor(0, 0); // Set the cursor on the first column and first row.
  lcd.print("JCC Winding Machine"); // Print the string "Hello World!"
  lcd.setCursor(4, 1); // Set the cursor on the first column and first row.
  lcd.print("version 0.1"); // Print the string "Hello World!"
  lcd.setCursor(3, 2); // Set the cursor on the first column and first row.
  lcd.print("-------------"); // Print the string "Hello World!"

  lcd.setCursor(0, 3); // Set the cursor on the first column and first row.
  lcd.print("(c) Lorenzo Iannone"); // Print the string "Hello World!"
  
  delay(3000);
  readyScreen();
}


void editWhenEditState(struct State *state) {
  if (state->currentState == ROUNDS_EDIT) {
      state->maxRounds = inputNumberB(state->maxRounds, 6,0,5);
      state->currentState = IDLE_APP;
      stateSummary(state);
  }

 if (state->currentState == RPM_EDIT) {
    state->maxRPM = inputNumberB(state->maxRPM, 16,0,4);
    state->currentState = IDLE_APP;
    stateSummary(state);
 }

 
 if (state->currentState == DIR_EDIT) {
    state->dir = inputDirection(state->dir, 6,1);
    state->currentState = IDLE_APP;
    stateSummary(state);
 }
}

void resetState() {
  state.maxRounds = 0;
  state.dir = 0;
  state.maxRPM = 3000;
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
    if (state->dir == CW) {
      lcd.print("cw ");
    } else {
      lcd.print("ccw ");
    }

    lcd.setCursor(0,3);
    lcd.print("A=turns B=rpm C=dir");

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
   lcd.print (" 00.0 G     ");
}

void inductanceMeterScreen() {
   lcd.setCursor(6,1);
   lcd.print (" 0.00 H");
   lcd.print("       ");
}

void programmingLoop(struct State*state, struct State* prevState) {

    //if there is a transition in the programming mode pin then clear the screen to display the new state
    if (prevState->programmingMode == LOW) {
      lcd.clear();
      stateSummary(state);
    }
    
    char key = keypad.getKey();

    switchState(key, state);
    editWhenEditState(state);  
}

void spinLoop(struct State*state, struct State* prevState) {
    if (prevState->programmingMode == HIGH || state->boot == true || prevState->ohmMeterMode == LOW) {
      if (state->boot == true) {
        state->boot = false;
      }
      if (state->maxRounds == 0) {
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
    int on;
    on = 12 * count / maxRounds;
    int off;
    off = 12 - on;
    
    lcd.setCursor(4,2);
    for (int x=0; x<on; x++) {
      lcd.write(ISTO_ON_CHAR);  
    }
    for (int x=0; x<off; x++) {
      lcd.write(ISTO_OFF_CHAR);  
    }
}

void countLoop(struct State *state) {
  
  int prev = LOW;
  int read = HIGH;
  int count = -1;
  char key = 'x';
  
  scatterMotor.attach(SCATTER_PIN); // start servo control

  countLoopContextMenu();
  countLoopScreen(0,state->maxRounds);
  countIstogram(0,state->maxRounds);
  
  while (key!='A' && key!='D') {
    key = keypad.getKey();
    if(key=='D') {
      return;
    }
  }

  while (key != 'D') {
    scatter(state);  
    read = digitalRead(OPTICAL_SENSOR);
    
    key = keypad.getKey();
    
    if (read==HIGH && prev ==LOW) {
      count++;
      countLoopScreen(count, state->maxRounds);
    }
    
    countIstogram(count,state->maxRounds);

    if (count >= state->maxRounds) {
      lcd.setCursor(4,2);
      lcd.print ("  complete  ");
      while (keypad.getKey() !='D') {
        //nop
      }
      scatterMotor.detach(); // start servo control
      return;
    }
    prev = read;
  }
  scatterMotor.detach(); // start servo control  
}

void inductanceMeterLoop(struct State *state, struct State* prevState) {
  if (prevState->inductanceMeterMode == LOW || prevState->programmingMode == HIGH || prevState->ohmMeterMode == HIGH || prevState->gaussMeterMode == HIGH) {
    lcd.clear(); 
    inductanceMeterScreen(); 
    }
 
    while (state->inductanceMeterMode == HIGH) {
      state->inductanceMeterMode = digitalRead(INDUCTANCE_MODE_PIN);
    }
}


void gaussMeterLoop(struct State*state, struct State* prevState) {
  if (prevState->gaussMeterMode == LOW || prevState->programmingMode == HIGH || prevState->inductanceMeterMode == HIGH || state->ohmMeterMode == HIGH) {
    lcd.clear(); 
    gaussMeterScreen();
  }  
  while (state->gaussMeterMode == HIGH) {
    state->gaussMeterMode = digitalRead(GAUSS_MODE_PIN);
  }
}

void ohmMeterLoop(struct State*state, struct State* prevState) {
  if (prevState->ohmMeterMode == LOW || prevState->programmingMode == HIGH || prevState->inductanceMeterMode == HIGH || prevState->gaussMeterMode == HIGH) {
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

void scatter(struct State *state) {
  state->scatterEnabled = digitalRead(ENABLE_SCATTER_PIN);
  if (state->scatterEnabled == HIGH) {   
    unsigned long curTime = millis();
    if (curTime - state->prevScatterTs > 350) {
      Serial.println("SCATTER");
      state->prevScatterTs = curTime;

      if (state->scatterPos == 0) {
        state->scatterPos = 180;
      } else {   
        state->scatterPos = 0;
      }
      scatterMotor.write(state->scatterPos);
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
  pinMode (INDUCTANCE_MODE_PIN, INPUT_PULLUP);
  pinMode (GAUSS_MODE_PIN, INPUT_PULLUP);
  pinMode (ENABLE_SCATTER_PIN, INPUT_PULLUP);

  
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

  
  lcd.backlight();
  splashScreen();
  lcd.clear();
}

void loop() {
  
  struct State prevState;
  memcpy (&prevState, &state, sizeof (struct State));

  state.programmingMode = digitalRead(PROG_PIN);
  state.ohmMeterMode = digitalRead(OHM_MODE_PIN);
  state.inductanceMeterMode = digitalRead(INDUCTANCE_MODE_PIN);
  state.gaussMeterMode = digitalRead(GAUSS_MODE_PIN);

  if (state.programmingMode == HIGH ) {
    Serial.println("PMODE ON");
    programmingLoop(&state, &prevState);
  }
  else if (state.ohmMeterMode == HIGH) {
    ohmMeterLoop(&state, &prevState);
  } else if (state.inductanceMeterMode == HIGH) {
    inductanceMeterLoop(&state, &prevState);
  } else if (state.gaussMeterMode == HIGH) {
    gaussMeterLoop(&state, &prevState);
  }
  else {
    Serial.println("PMODE OFF");
    spinLoop(&state, &prevState);
    resetState();
  }
}                                                                                                                                                                                                                                         
