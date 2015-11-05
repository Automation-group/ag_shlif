#include <EEPROM.h>

#include <LiquidCrystalRus.h>

#include <LiquidCrystal.h>

#include <Keypad.h>

LiquidCrystalRus lcd(13, 12, 11, 10, 8, 9);

const byte rows = 4; //four rows
const byte cols = 4; //three columns
char keys[rows][cols] = {
  {'1','2','3', 'A'},
  {'4','5','6', 'B'},
  {'7','8','9', 'C'},
  {'*','0','#', 'D'}
};

byte rowPins[rows] = {6,7, A5, A4}; //connect to the row pinouts of the keypad
byte colPins[cols] = {A3, A2, A1, A0}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );

long zStep = 0; // мкм
long zAll  = 0; // мкм
long z10000S_um = 10000;

bool onRestrictor = false;

void readEEPROM(void* p, int sz, int offset) {
  unsigned char* buf = (unsigned char*)p;
  for(int i=0; i<sz; ++i) {
    buf[i] = EEPROM.read(i+offset);
  }
}

void writeEEPROM(void* p, int sz, int offset) {
  unsigned char* buf = (unsigned char*)p;
  for(int i=0; i<sz; ++i) {
    EEPROM.write(i+offset, buf[i]);
  }
}

void setup() {
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  lcd.begin(16, 2);
  Serial.begin(115200);
  keypad.addEventListener(keypadEvent);
  
  lcd.print("A-вверх B-вниз");
  delay(2000);
  readEEPROM(&zStep, sizeof(long), 0);
  readEEPROM(&zAll, sizeof(long), sizeof(long));  
  readEEPROM(&z10000S_um, sizeof(long), sizeof(long)*2);    
  if (z10000S_um == 0) {
     // инициализация
     z10000S_um = 10000;
     writeEEPROM(&z10000S_um, sizeof(long), sizeof(long)*2);    
  } 
  
  printInitial(); 
  readLine(NO_KEY);
    
}

void printInitial() {
  clearLine(0);
  lcd.print("В ");  
  lcd.print(zAll);
  lcd.print(" / Ш ");
  lcd.print(zStep);  
  lcd.print(" ");  
}

int incomingByte = 0;
const int cmd_buffer_size = 20;
char cmd_buffer[cmd_buffer_size + 1]; // plus zero
int in_buffer_pos = 0;

long startTime  = 0;
const int deltaTime  = 300;
long pos = 0;

void clearLine(int row) {
  lcd.setCursor(0, row);
  lcd.write("                ");
  lcd.setCursor(0, row);  
}

const int lineBufferSize = 16;
char lineBuffer[lineBufferSize];
int lineBufferPos = -1;

void cancelReadLine() {
    lineBufferPos = 0;
    lineBuffer[lineBufferPos] = 0; // конец строки  
}

bool readLine(char key) {
  // инициализация
  if (lineBufferPos == -1) {
    lineBufferPos = 0;
    lineBuffer[lineBufferPos] = 0; // конец строки
  }
  
  if (key == NO_KEY)
    return false;
    
  if (key >= '0' && key <= '9') {
    if (lineBufferPos == 0 && key == '0')
      key = '-';
    lineBuffer[lineBufferPos] = key;
    if (lineBufferPos < lineBufferSize)
      ++lineBufferPos;
    lineBuffer[lineBufferPos] = 0; // конец строки
  } else {
    if (key == '#') {
      --lineBufferPos;
      lineBufferPos = lineBufferPos <0 ? 0 : lineBufferPos; 
      lineBuffer[lineBufferPos] = 0; // конец строки
    }
    
    if (key == '*') {
      return true;
    }    
  }
  
  return false;
}

enum ProgramState_t {
  PS_Initial,
  PS_InputStep,
  PS_InputAll,
  PS_GoToPos,
  PS_Step2um,
  PS_Run
};

ProgramState_t ProgramState = PS_Initial;
long runCurPos = 0;
long runEndPos = 0;
long runStepPos = 0;

void loop() {
    
  char key = keypad.getKey();
  if (key != NO_KEY){

    if (ProgramState == PS_InputStep || ProgramState == PS_InputAll || ProgramState == PS_GoToPos || ProgramState == PS_Step2um) {
      if (key == 'C') {
        ProgramState = PS_Initial;
        cancelReadLine();
        printInitial();
        return;
      }
      
      bool ok = readLine(key);      
      lcd.setCursor(0, 1);
      lcd.print(lineBuffer);        
      lcd.print(" ");
      
      if (ok && ProgramState == PS_InputStep) {
        zStep = atol(lineBuffer);
        cancelReadLine();
        ProgramState = PS_Initial;
        printInitial();
        writeEEPROM(&zStep, sizeof(long), 0);
        return;        
      }
      
      if (ok && ProgramState == PS_InputAll) {
        zAll = atol(lineBuffer);
        cancelReadLine();
        ProgramState = PS_Initial;
        printInitial();
        writeEEPROM(&zAll, sizeof(long), sizeof(long));          
        return;
      }      
      
      if (ok && ProgramState == PS_Step2um) {
        z10000S_um = atol(lineBuffer);
        cancelReadLine();
        ProgramState = PS_Initial;
        printInitial();
        writeEEPROM(&z10000S_um, sizeof(long), sizeof(long)*2);          
        return;
      }       
      
      if (ok && ProgramState == PS_GoToPos) {
        Serial.write('G');
        double p = (double)atol(lineBuffer) / ((double)z10000S_um / 10000.0);
        Serial.print((long)p);
        Serial.write('\n');        
        cancelReadLine();
        ProgramState = PS_Initial;
        printInitial();
        return;
      }       
      
    }
    
    if (ProgramState == PS_Initial) {
      if (key == '1') {
        ProgramState = PS_InputStep;
        lcd.clear();
        lcd.print("Шаг,мкм");
      }
      
      if (key == '2') {
        ProgramState = PS_InputAll;
        lcd.clear(); 
        lcd.print("Всего,мкм");
      }      
      
      if (key == '3') {
        ProgramState = PS_GoToPos;
        lcd.clear(); 
        lcd.print("В позицию,мкм");
      }      

      if (key == '0') {
        ProgramState = PS_Step2um;
        lcd.clear(); 
        lcd.print("10000ш = X мкм");
      }      

      if (key == '5') {
        // запуск        
        ProgramState = PS_Run;
        runCurPos = pos;
        double p = (double)zAll / ((double)z10000S_um / 10000.0);
        runEndPos = runCurPos - p;
        p = (double)zStep / ((double)z10000S_um / 10000.0);        
        runStepPos = p;
        onRestrictor = false;     
      }            
    }    
  }
 
  long time = millis();
  unsigned dT = time - startTime;
  
  if ((ProgramState == PS_Initial || ProgramState == PS_Run) && dT > deltaTime) {
    startTime = time;
    char* line = "                ";
    double p = (double)pos * ((double)z10000S_um / 10000.0);
    snprintf(line, 16, "Поз %5li", (long)p); 
    //clearLine(1);
    lcd.setCursor(0, 1);
    lcd.print(line);    
    lcd.print(" ");
    if (ProgramState == PS_Run) {
      static int pIndex = 0;
      lcd.setCursor(15, 1);
      lcd.write('0' + pIndex);//process[pIndex]);
      ++pIndex;
      pIndex &= 0xf;
    }
    Serial.print("P\n");        
  }  
  
  if (Serial.available() > 0) {
    incomingByte = Serial.read();
    if (incomingByte == '\n') {
      cmd_buffer[in_buffer_pos] = 0;
      in_buffer_pos = 0;
      analyse_command();
   
    } else {
      cmd_buffer[in_buffer_pos] = incomingByte;
      in_buffer_pos++;
      if (in_buffer_pos == cmd_buffer_size) in_buffer_pos = 0;
    }
  }  
  
  
  if (onRestrictor && ProgramState == PS_Run) {
    onRestrictor = false;
    runCurPos -= runStepPos;
    if (runCurPos<runEndPos) {
      runCurPos = runEndPos;
      ProgramState = PS_Initial;
    }
    Serial.write('G');
    Serial.print(runCurPos);  
    Serial.write('\n');        
  }
}


void analyse_command() {  
  // сработал конечник
  if ( (cmd_buffer[0] == '^')) {
    onRestrictor = true;
  }
  
  if ( (cmd_buffer[0] == 'P')) {
    union U {
      char body[6];
      long pos;
    } u;
    memcpy(u.body, cmd_buffer+1, 5);
    if (u.body[4] & 1) {
      --u.body[0];
    }

    if (u.body[4] & 2) {
      --u.body[1];
    }

    if (u.body[4] & 4) {
      --u.body[2];
    }

    if (u.body[4] & 8) {
      --u.body[3];
    }
    
    pos = u.pos;
    return;
  }  
  
  if ( (cmd_buffer[0] == 'I') && (cmd_buffer[1] == 'D') ) {
    Serial.print("ID461b5781e5067567867aw7fbbfcc7ad3\n");
    Serial.flush();
    return;
  }
}

// Example
// setup: keypad.addEventListener(keypadEvent); //add an event listener for this keypad

void keypadEvent(KeypadEvent key){
  switch (keypad.getState()){
    case PRESSED:
      switch (key){
        case 'A': 
          if (ProgramState != PS_Run)
            Serial.print("+\n"); 
        break;
        case 'B':
          if (ProgramState != PS_Run)
            Serial.print("-\n"); 
        break;
        case 'D': // отменить все
          Serial.print("S\n");
          cancelReadLine();
          ProgramState = PS_Initial;
          printInitial();          
        break;
      }
    break;
    case RELEASED:
      switch (key){
        case 'A':
        case 'B':
          Serial.print("S\n"); 
        break;
      }
    break;
    case HOLD:
      switch (key){
        //case '*': lcd.print(" HOLD"); break;
        case 'C':
          if (ProgramState != PS_Run)
            Serial.print("Z\n");
        break;
        
      }
    break;
  }
}
