#include <LiquidCrystal_I2C.h> // Library for LCD

#include <Keypad.h>


LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4); // Change to (0x27,16,2) for 16x2 LCD.

/******************************
 * PROGRAMMING MODE INIT
 *****************************/
const int PROG_PIN = 30;




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
const char RPM_EDIT = 'A';
const char ROUNDS_EDIT = 'B';
const char DIR_EDIT = 'C';


/****************
 * APP STATE
 *****************/

struct State {
  long maxRPM = 3000;
  long maxRounds = 0;

  long tempMaxRPM = 0;
  long tempMaxRounds = 0;
  
  char currentState = IDLE_APP;
  char programmingMode = LOW;
};

struct State state;



void switchState(char menuKey, struct State *state) {
  if (menuKey == RPM_EDIT) {
    state->currentState = menuKey;
  } else if (menuKey == ROUNDS_EDIT) {
    state->currentState = menuKey;
  }
}

/********************************
 * Settings
 */

long inputNumberB(long d, int col, int row, int maxDigits) {
  long number = 0;
  char exit = '#';
  char key = 'x';
  int digits = 0;
  
  while(key != exit) {
    lcd.setCursor(col,row); 
    lcd.blink();
    
    key = keypad.getKey();
    if (key!= NO_KEY) {
      Serial.print ("in ");
      Serial.println (key);
    }
    if ((key == '1' || key == '2' || key == '3' || key == '4' || key == '5' || key == '6' || key == '7' || key == '8' || key == '9' || key == '0')) {
      if (digits < maxDigits) {
        number = (number * 10 + ((long) key - 48));
        Serial.println(number);
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
      Serial.print ("cancelled");
      lcd.noBlink();
      return d;
    }
  }

  Serial.print ("inputed");
  Serial.println (number);
  lcd.noBlink();
  return number;
}

void confirmData(char key, struct State *state) {
  if (key != '#') return;
  
  if (state->currentState == ROUNDS_EDIT) {
    state->maxRounds = state->tempMaxRounds;
    state->tempMaxRounds = 0;
    state->currentState = IDLE_APP;
  }

  if (state->currentState == RPM_EDIT) {
    state->maxRPM = state->tempMaxRPM;
    state->tempMaxRPM = 0;
    state->currentState = IDLE_APP;
  }
}

void deleteData(char key, struct State *state) {
  if (key != '*') return;
  
  if (state->currentState == ROUNDS_EDIT) {
    state->tempMaxRounds = 0;
    state->currentState = IDLE_APP;
  }

  if (state->currentState == RPM_EDIT) {
    state->tempMaxRPM = 0;
    state->currentState = IDLE_APP;
  }
}


void resetState(struct State *state) {
  state->tempMaxRPM = 0;
  state->tempMaxRounds = 0;
}

void readyScreen() {
  lcd.setCursor(7, 0); // Set the cursor on the first column and first row.
  lcd.print("Ready"); // Print the string "Hello World!"
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
  
  delay(400);
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
}

void stateSummary(State *state) {
  if (state->programmingMode == HIGH || state->currentState==IDLE_APP) {
    lcd.setCursor(0,0);
    lcd.print("Turns:");
    lcd.setCursor(6,0);
    char r[5];
    sprintf(r, "%-5d", state->maxRounds);
    lcd.print(r);
    
    lcd.setCursor(12,0);
    lcd.print("Rpm:");
    char t[4];
    sprintf(t, "%-4d", state->maxRPM);
    lcd.setCursor(16,0);
    lcd.print(t);

    lcd.setCursor(0,3);
    lcd.print("A Rpm,B Turns,C Dir");

  }
}

int compareState(struct State *prev, struct State *cur) {
  if ( prev->maxRPM != cur->maxRPM || prev->maxRounds!=cur->maxRounds) {
    return 0;
  }
  return 1;
}

void setup() {
  // put your setup code here, to run once:
  pinMode (PROG_PIN, INPUT_PULLUP);

  Serial.begin(9600);
  Serial.println("Ready");                               

  // Initiate the LCD:
  lcd.init();
  lcd.backlight();
  splashScreen();
  lcd.clear();
}

void loop() {
  struct State previousState;
  memcpy (&previousState, &state, sizeof (struct State));
  
  char previousProgramming = state.programmingMode;
  
  state.programmingMode = digitalRead(PROG_PIN);
  
  if (state.programmingMode == HIGH) {

    if (previousProgramming != HIGH) {
      lcd.clear();
      stateSummary(&state);
    }
    
    char key1 = keypad.getKey();

    switchState(key1, &state);
    editWhenEditState(&state);
  } else {
    if (previousProgramming != LOW) {
      lcd.clear();
      readyScreen();
    }
    resetState(&state);
  }
}
