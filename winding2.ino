#include <LiquidCrystal_I2C.h> // Library for LCD

#include <Keypad.h>


LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4); // Change to (0x27,16,2) for 16x2 LCD.

/******************************
 * PINS
 *****************************/
const int PROG_PIN = 30;
const int OHM_MODE_PIN = 31;
const int OHM_VALUE_PIN = 0;
const int OPTICAL_SENSOR = 3;

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


/*********************************
 * MENU DEFINITION
 *********************************/
const char IDLE_APP = '0';
const char ROUNDS_EDIT = 'A';
const char RPM_EDIT = 'B';
const char DIR_EDIT = 'C';

const char CW = 0;
const char CCW = 1;

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
  bool boot = true;
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
        digits++;
      }

      lcd.setCursor(col,row);
      if (maxDigits == 4) {
        char r[4];
        sprintf(r, "%-4d", number);
        lcd.print(r);
      } else {
        char r[5];
        sprintf(r, "%-5d", number);
        lcd.print(r);
      }
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
    char r[5];
    sprintf(r, "%-5d", state->maxRounds);
    lcd.print(r);
    
    lcd.setCursor(12,0);
    lcd.print("rpm:");
    char t[4];
    sprintf(t, "%-4d", state->maxRPM);
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
   lcd.setCursor(5,1);
   lcd.print ("0.00 K Ohm         ");
}


void setup() {
  // put your setup code here, to run once:
  pinMode (PROG_PIN, INPUT_PULLUP);
  pinMode (OHM_MODE_PIN, INPUT_PULLUP);
  pinMode (OPTICAL_SENSOR, INPUT_PULLUP);
  Serial.begin(9600);
  Serial.println("Ready");                               

  // Initiate the LCD:
  lcd.init();
  lcd.backlight();
  splashScreen();
  lcd.clear();
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

void countLoop(struct State *state) {
  int prev = LOW;
  unsigned int count = 0;
  char key = 'x';

  while (key != 'D') {
    int read = digitalRead(3);
    key = keypad.getKey();
    
    if (read==HIGH && prev ==LOW) {
      count++;
      lcd.setCursor(5,1);
      lcd.print(count);
      lcd.print("/");
      lcd.print(state->maxRounds);
    }
    prev = read;
  }
  

  
}

void ohmmeterLoop(struct State*state, struct State* prevState) {
  if (prevState->ohmMeterMode == LOW) {
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
        
        lcd.setCursor(5,1);
        lcd.print (ohm/1000);
        lcd.print (".");
        char d[2];
        sprintf(d, "%02d", (ohm - ohm/1000*1000)/10);
        lcd.print(d);
        lcd.print(" K Ohm        ");
      }
    
  }
  
}
int count = 0;
int p = LOW;
int r = LOW;

void loop() {
  struct State prevState;
  memcpy (&prevState, &state, sizeof (struct State));

  state.programmingMode = digitalRead(PROG_PIN);
  state.ohmMeterMode = digitalRead(OHM_MODE_PIN);

  if (state.programmingMode == HIGH) {
    programmingLoop(&state, &prevState);
  }  else if (state.ohmMeterMode == HIGH) {
      ohmmeterLoop(&state, &prevState);
  } else {
    spinLoop(&state, &prevState);
    resetState();
  }
}
