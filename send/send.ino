// fixed hardware
#define SerialHw Serial1
#define LED 11

// configurable hardware
#define DATACLK 5
#define DATA1 0
#define DATA2 1
#define DATA3 2

// common data
#define BITS (24*3)
volatile byte response[BITS];
int delayTime = 10;

// ---------- setup ----------

void setup() {
  Serial.begin(9600);
  SerialHw.begin(31250);

  Joystick.useManualSend(true);

  pinMode(LED, OUTPUT);

  pinMode(DATA1, INPUT);
  pinMode(DATA2, INPUT);
  pinMode(DATA3, INPUT);
  pinMode(DATACLK, INPUT);
  attachInterrupt(digitalPinToInterrupt(DATACLK), capture, FALLING); // should be rising, but doesn't work reliably

  Serial.println("Teensy ready. Send 'h' to get a list of commands.");
}

// ---------- capturing ----------

volatile byte responseIndex = 0;

void capture() {
  if(responseIndex>=BITS) return; // avoid spureous
  
  byte data = PINB;
  response[responseIndex++] = (data>>DATA1) & 1; //digitalRead(DATA1);
  response[responseIndex++] = (data>>DATA2) & 1; //digitalRead(DATA2);
  response[responseIndex++] = (data>>DATA3) & 1; //digitalRead(DATA3);
}

int communicate(String data, int waitfor){  
  responseIndex = 3; // 3 because of the falling fix
  for(int i = 0; i < data.length(); i+=2){
    SerialHw.write(str2b(data[i],data[i+1]));
  }
  if(waitfor == 1) waitfor = 1+3; // +3 because of the falling fix

  int counter = 32767;
  while((waitfor < 0 || responseIndex < waitfor) && counter > 0) counter--;
  return responseIndex - 3; // -3 because of the falling fix
}

// ---------- loop ----------

byte enable_mainPoll = 1;
byte enable_secondPoll = 0;
byte enable_feedback = 0;
byte enable_debug = 0;

void loop() {
  digitalWrite(LED, LOW);

  
  if(enable_mainPoll) mainPoll();
  if(enable_secondPoll) secondPoll();
  

  if(enable_feedback) feedback();
  
  if(enable_debug) debug();


  commands();


  Joystick.send_now();
  digitalWrite(LED, HIGH);
  delay(delayTime);
}

// ---------- main polling ----------

// button assignment
#define BUTTONS 17
byte assign[BUTTONS] = {
  33, // button A
  34, // button B
  36, // button C
  37, // button D
  27, // button E
  28, // button F
  30, // button G
  31, // button H
  38, // trigger left
  35, // trigger right
  29, // pad left
  40, // pad down
  32, // pad right
  39, // pad up
  25, // button O
  26, // button Set
  24, // button Force on/off
};

byte joined_axes = 1;

void mainPoll(){

  // ask
  communicate("a50d",24*3);

  // buttons
  for(int i=0;i<BUTTONS;++i){
    Joystick.button(i + 1, response[assign[i]]);
  }

  // wheel
  int wheel = 0;
  for(int i=0;i<8;++i){
    wheel += response[48+i*3]<<(7-i);
  }
  if(response[42]) wheel = -(255-wheel);
  wheel = (wheel*2)+512; // range: [-256,256] -> [0,1023]
  Joystick.X(wheel);

  if(response[41]){

    // accel
    int acc = 0;
    for(int i=0;i<8;++i){
      acc += response[50+i*3]<<(7-i);
    }
    acc=acc*8; // range: [0,63]->[0,511]

    // break
    int brk = 0;
    for(int i=0;i<8;++i){
      brk += response[49+i*3]<<(7-i);
    }
    brk=brk*8; // range: [0,63]->[0,511]

    if(joined_axes){
      Joystick.Y(acc-brk+512); // positive if accel, negative if break, range: [0,511]-[0,511] -> [0,1023]
    }else{
      Joystick.Y(acc+512); // range: [0,511] -> [512,1023]
      Joystick.Z(brk+512); // range: [0,511] -> [512,1023]
    }
    
  }else{
    Joystick.Y(512);
  }
}

// ---------- secondary polling ----------

void secondPoll(){
  
  // ask  
  communicate("a503",24);

  // TODO: do something with this
  // byte force = response[13];
  // byte power = response[16];
  
}

// ---------- any command ----------

void anyCommand(String command){
  if(command.length()%2==1) command+='0';
  Serial.printf("sent: %s\n",command.c_str());
  int received = communicate(command,-1);
  Serial.printf("received: %i\n",received);
  if(received>0) printReceived(received+3); // +3 because of the falling fix
}

// ---------- test feedback ----------

byte active_effects[15] = {0};
char dataString[30] = {0};

char* effectString[14] = {
  "A50882%02XF4FFFF057E007E000000",
  "A50884%02X542C2C056500005A0096",
  "A50882%02X5490900A7E0006960096",
  "A50888%02XF1FFFF0065657E7E0000",
  "A50887%02XF1FFFF007E7E7E7E0900",
  "A50880%02X0464640A65007E086500",
  "A50880%02XFCFFFF0A7E0000000000",
  "A50880%02XF4FFFF0A7E0000000000",
  "A5088A%02XF1FFFF007E197E7E0000",
  "A50888%02XF1FFFF007F7F4B4B0900",
  "A50885%02X54F4F40C65CE00000000",
  "A50888%02XF1FFFF007F7F7E7E0000",
  "A50884%02XF4E8E8027E0000000000",
  "A50882%02X542C2C117E007E00005E"
};
char* enableString = "A50A00%02X0100";
char* cancelString = "A509%02X";
String resetString = "A502";

void feedback(){

  // test buttons
  for(int i=0;i<14;++i){
    if(active_effects[i] != response[27+i]){
      // changed effect
      if(response[27+i]){
        // enable
        sprintf(dataString,effectString[i],i+1);
        communicate(dataString,1);
        Serial.printf("Activated: %s\n",dataString);
        sprintf(dataString,enableString,i+1);
        communicate(dataString,1);
      }else{
        // disable
        sprintf(dataString,cancelString,i+1);
        communicate(dataString,1);
        Serial.printf("Deactivated: %s\n",dataString);
      }
      active_effects[i] = response[27+i];
    }
  }
  if(response[25]){
    // master reset
    communicate(resetString,1);
  }
}

// ---------- debug ----------

byte prevResponse[BITS];

void debug(){  
  for(int i=0;i<BITS;++i){
    if(response[i] != prevResponse[i]){
      Serial.printf("%2i: %s\n",i,response[i]?"0->1":"1->0");
      prevResponse[i]=response[i];
    }
  }
  printReceived(BITS);
}

void printReceived(int amount){
  for(int r=0;r<3;++r){
    for(int c=r;c<amount;c+=3){
      Serial.printf("%02i=%i ",c, response[c]);
    }
    Serial.println();
  }
  Serial.println();
}

// ---------- command input ----------

#define TOGGLE(name,var)                       \
  var = !var;                                  \
  Serial.printf(name ": %s\n", var?"ON":"OFF");

String command = "";

void commands(){
  while(Serial.available() > 0){
    char c = Serial.read();
    switch(c){
      case 'h':
        Serial.println("'+' to increase polling time (slower)");
        Serial.println("'-' to decrease polling time (faster)");
        Serial.println("'?' to toggle debug mode");
        Serial.println("'*' to toggle feedback test mode");
        Serial.println("'z' to toggle joined axis mode");
        Serial.println("'m' to toggle main polling");
        Serial.println("'s' to toggle secondary polling");
        Serial.println("any hex-string and then enter to send that message via uart");
        Serial.println("(any other is ignored)");
        break;
      case '+':
        delayTime += 10;
        Serial.printf("Delay set to %i\n",delayTime);
        break;
      case '-':
        if(delayTime>=10) delayTime -= 10;
        Serial.printf("Delay set to %i\n",delayTime);
        break;
      case '?':
        TOGGLE("Debug mode", enable_debug);
        break;
      case '*':
        TOGGLE("Feedback test mode", enable_feedback);
        break;
      case 'z':
        TOGGLE("Joined axes", joined_axes);
        break;
      case 'm':
        TOGGLE("Main polling command", enable_mainPoll);
        break;
      case 's':
        TOGGLE("Secondary polling command", enable_secondPoll);
        break;
        
      case '\n':
        if(command.length()>0) anyCommand(command);
        command = "";
        break;
    }

    if('0' <= c && c <= '9') command += c;
    if('a' <= c && c <= 'f') command += c;
    if('A' <= c && c <= 'F') command += c;
  }
}

// ---------- utils ----------

byte str2b(char c1, char c2){
  return str2b(c1)<<4 | str2b(c2);
}

byte str2b(char c){
  if('0' <= c && c <= '9') return c - '0';
  if('a' <= c && c <= 'f') return c - 'a' + 10;
  if('A' <= c && c <= 'F') return c - 'A' + 10;
  return 0;
}
