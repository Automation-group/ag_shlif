
void setup() {
  Serial.begin(115200);
  
  // put your setup code here, to run once:
  // PB3 P- 
  pinMode(11, OUTPUT); 
 
  // PB4 D-
  pinMode(12, OUTPUT); 
  digitalWrite(12, HIGH);
  
  // PB5 E-
  pinMode(13, OUTPUT); 
  digitalWrite(13, HIGH);
  
  // PC5 F-
  
}

int incomingByte = 0;
const int cmd_buffer_size = 20;
char cmd_buffer[cmd_buffer_size + 1]; // plus zero
int in_buffer_pos = 0;

unsigned long startTime = 0;
const int stepDelay = 350;
char stepPinState = LOW;

long pos = 0;
bool dirPlus = true;
bool flagDoSteps = false;
bool flagGo2Pos = false;
long go2Pos = 0;

char prevSensor1 = 0;
char prevSensor2 = 0;
char prevSensorsFlag = 0;

void loop() {
  
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
  
  unsigned long time = micros();
  unsigned int dT = time - startTime;
  if (flagDoSteps && (dT > stepDelay)) {
    startTime = time;
    
    flagDoSteps = true;
    if (flagGo2Pos && go2Pos == pos) {
      flagDoSteps = false;
      flagGo2Pos = false;
    }  
    
    if(stepPinState == HIGH)
      pos += dirPlus ? 1 : -1;    
    digitalWrite(11, stepPinState);
    stepPinState = stepPinState == LOW ? HIGH : LOW;
  }
  
  char sensor1 = digitalRead(A0);
  char sensor2 = digitalRead(A1);
  char sensorsFlag = prevSensorsFlag;
  sensorsFlag = prevSensor1 != sensor1 ? 1 : sensorsFlag;
  sensorsFlag = prevSensor2 != sensor2 ? 0 : sensorsFlag;
  
  if (prevSensorsFlag != sensorsFlag) {
    Serial.print("^\n");
  }
/*
  if (prevSensor1 != sensor1) {
    Serial.print("1\n");
  }
  
  if (prevSensor2 != sensor2) {
    Serial.print("2\n");
  }
*/
  prevSensor1 = sensor1;
  prevSensor2 = sensor2;
  prevSensorsFlag = sensorsFlag;
  /*
  // put your main code here, to run repeatedly:
  digitalWrite(11, LOW);
  delayMicroseconds(300);
  //delay(100);
  digitalWrite(11, HIGH);
  //delay(100);
  delayMicroseconds(300);
  */
  
}

void analyse_command() {  
  // Команды
  
  // D - выключить двиг
  if ( (cmd_buffer[0] == 'D')) {
    digitalWrite(13, LOW);
    flagGo2Pos = false;
    flagDoSteps = false;
    Serial.print("D\n");
    Serial.flush();
    return;
  }
  
  // E - уменьшение счетчика
  if ( (cmd_buffer[0] == 'E')) {
    digitalWrite(13, HIGH);
    flagGo2Pos = false;
    flagDoSteps = false;
    Serial.print("E\n");
    Serial.flush();
    return;
  }
  
  // + - увелич. счетчика
  if ( (cmd_buffer[0] == '+')) {
    digitalWrite(12, HIGH);
    dirPlus = true;
    flagGo2Pos = false;
    flagDoSteps = true;
    Serial.print("+\n");
    Serial.flush();
    return;
  }
  
  // - - уменьшение счетчика
  if ( (cmd_buffer[0] == '-')) {
    digitalWrite(12, LOW);
    dirPlus = false;
    flagGo2Pos = false;
    flagDoSteps = true;
    Serial.print("-\n");
    Serial.flush();
    return;
  }
  
  // S - стоп
  if ( (cmd_buffer[0] == 'S')) {
    flagDoSteps = false;
    flagGo2Pos = false;
    Serial.print("S\n");
    Serial.flush();
    return;
  }
  
  // P - получить текущую позицию
  if ( (cmd_buffer[0] == 'P')) {
    Serial.print("P");
    union U {
      char body[6];
      long pos;
    } u;
    u.pos = pos;
    
    u.body[4] = 0;
    
    if (u.body[0] == '\n') {
      u.body[4] |= 1;
      ++u.body[0];
    }
    
    if (u.body[1] == '\n') {
      u.body[4] |= 2;
      ++u.body[1];
    }
    
    if (u.body[2] == '\n') {
      u.body[4] |= 4;
      ++u.body[2];
    }    
    
    if (u.body[3] == '\n') {
      u.body[4] |= 8;
      ++u.body[3];
    }
    
    u.body[5] = 0;
    Serial.write(u.body[0]);    
    Serial.write(u.body[1]);    
    Serial.write(u.body[2]);        
    Serial.write(u.body[3]);        
    Serial.write('\n');    
    Serial.flush();
    return;
  }
  
  // Z - обнулить позицию
  if ( (cmd_buffer[0] == 'Z')) {
    flagGo2Pos = false;
    pos = 0;
    Serial.print("Z\n");
    Serial.flush();
    return;
  }
  
  // G - переместить в позицию
  if ( (cmd_buffer[0] == 'G')) {
    flagDoSteps = true;
    flagGo2Pos = true;
    go2Pos = atol(cmd_buffer+1);
    dirPlus = pos < go2Pos ? true : false;
    if (dirPlus)
      digitalWrite(12, HIGH);
    else
      digitalWrite(12, LOW);
      
    Serial.print("G\n");
    Serial.flush();
    return;
  }
  
  if ( (cmd_buffer[0] == 'I') && (cmd_buffer[1] == 'D') ) {
    Serial.print("ID461b5781e505b424d5e7f4fbbfcc7ad3\n");
    Serial.flush();
    return;
  }
}

