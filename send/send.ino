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
char show_debug = 0;

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
}

// ---------- capturing ----------

volatile byte responseIndex = 0;

void capture() {
  if(responseIndex>=BITS) return; // avoid spureous
  
  //byte data = PINB;
  response[responseIndex++] = digitalRead(DATA1);
  response[responseIndex++] = digitalRead(DATA2);
  response[responseIndex++] = digitalRead(DATA3);
}

int communicate(String data, int waitfor){
  responseIndex = 3; // 3 because of the falling fix
  for(int i = 0; i < data.length(); i+=2){
    SerialHw.write(str2b(data[i],data[i+1]));
  }
  if(waitfor == 1) waitfor = 3+1; // because of the falling fix

  int counter = 10000;
  while((waitfor < 0 || responseIndex < waitfor) && counter > 0) counter--;
  return responseIndex - 3; // because of the falling fix
}

// ---------- main ----------

void loop() {
  digitalWrite(LED, LOW);

  
  mainCommand();
  //secondaryCommand();
  

  if(show_debug) debug();

  commands();


  Joystick.send_now();
  digitalWrite(LED, HIGH);
  delay(delayTime);
}

// ---------- commands ----------

void mainCommand(){
  
  communicate("a50d",24*3);

  // buttons
  for(int i=0;i<17;++i){
    Joystick.button(i + 1, response[24+i]);
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

    // break
    int brk = 0;
    for(int i=0;i<8;++i){
      brk += response[49+i*3]<<(7-i);
    }
    brk=brk*8; // range: [0,63]->[0,511]
  
     // accel
    int acc = 0;
    for(int i=0;i<8;++i){
      acc += response[50+i*3]<<(7-i);
    }
    acc=acc*8; // range: [0,63]->[0,511]

    Joystick.Y(acc-brk+512); // positive if accel, negative if break, range: [-511,511] -> [0,1023]
    
  }else{
    Joystick.Y(512);
  }
}

void secondaryCommand(){

  communicate("a503",24);

  // TODO: detect power here
  
}

void forceCommand(String command){
  communicate(command,1);
  
  communicate("A50A00010100",1);  
}

void forceClear(){
  communicate("A50901",1); 
}

void anyCommand(String command){
  Serial.printf("sent: %s\n",command.c_str());
  int received = communicate(command,-1);
  Serial.printf("received: %i\n",received);
  debug();
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
  for(int r=0;r<3;++r){
    for(int c=r;c<BITS;c+=3){
      Serial.printf("%02i=%i ",c, response[c]);
    }
    Serial.println();
  }
  Serial.println();
}

// ---------- input ----------

String command = "";

void commands(){
  while(Serial.available() > 0){
    char c = Serial.read();
    switch(c){
      case '+':
        delayTime += 10;
        break;
      case '-':
        if(delayTime>=100) delayTime -= 10;
        break;
      case '?':
        show_debug = !show_debug;
      case '\n':
        if(command.length()>0) anyCommand(command);
        command = "";
        break;
        
      default:
        command += c;
    }
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
