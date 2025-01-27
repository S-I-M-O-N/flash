//FIXME
const int programPin = 40;
const int readPin = 4;
const int enablePin = 2;


const unsigned long romSize = 1024 * 1024;


//The pinout from the eprom is different from the snes pinout
int adrPins[20] = {22, //eprom A0  snes A0
                   23,//eprom A1  snes A1
                   24,//eprom A2  snes A2
                   25,//eprom A3  snes A3
                   26,//eprom A4  snes A4
                   27,//eprom A5  snes A5
                   28,//eprom A6  snes A6
                   29,//eprom A7  snes A7
                   30,//eprom A8  snes A8
                   31,//eprom A9  snes A9
                   32,//eprom A10 snes A10           38-a16
                   33,//eprom A11 snes A11           39-a17
                   34,//eprom A12 snes A12           40-a18
                   35,//eprom A13 snes A13           41-a19
                   36,//eprom A14 snes A14
                   37,//eprom A15 snes A15
                   38,//eprom A16 snes A18 *
                   39,//eprom A17 snes A16 *
                   41//41 //eprom A18 snes A17 *
                  };

//39,//eprom A17 snes A19 *

char dataPins[8] = {5, 6, 7, 8, 9, 10, 11, 12};
/*
  Frame Format
  program:
  |preamble|opt|addr0|addr1|addr2|numbbutes|bytes|checksum|end
  read:
  |preamble|opt|addr0|addr1|addr2|numbbutes|end


*/
byte inByte = 0;
unsigned int secH = 0, secL = 0;

void setup() {
  // put your setup code here, to run once:
  pinMode(3, OUTPUT);
  digitalWrite(3, LOW);

  pinMode(programPin, OUTPUT);
  pinMode(readPin, OUTPUT);
  pinMode(enablePin, OUTPUT);
  //FIXME
  for (int i = 0; i < 20; i++) {
    pinMode(adrPins[i], OUTPUT);
  }
  digitalWrite(programPin, HIGH);
  digitalWrite(readPin, HIGH);
  digitalWrite(enablePin, HIGH);
  Serial.begin(185000);
  delay(1000);
  readMode();
  //  setAddress(0);
  //  programByte(0x78);
  //  //writeSector(5);
}
int index = 0;
void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available()) {
    inByte = Serial.read();
    if (inByte == 0x55) {
      while (Serial.available() == 0);
      inByte = Serial.read();
      switch (inByte) {
        case 'w':
          programMode();
          while (Serial.available() < 2);
          secH = Serial.read();
          secL = Serial.read();
          writeSector(secH, secL);
          break;
        case 'r':
          readMode();
          readROM();
          break;
        case 'e':
          programMode();
          eraseROM();
          break;
        case 'c':
          programMode();
          checkROM();
          break;
      }
    }
  }
}


//low level functions, direct ccontact with hardware pins
void programMode() {
  //data as output
  for (int i = 0; i < 8; i++) {
    pinMode(dataPins[i], OUTPUT);
  }
  digitalWrite(readPin, HIGH);
  digitalWrite(programPin, LOW);
}
void readMode() {
  //data as input
  for (int i = 0; i < 8; i++) {
    pinMode(dataPins[i], INPUT);
  }
  //  for (int i = 0; i < 8; i++) {
  //    digitalWrite(dataPins[i], HIGH);
  //  }
  digitalWrite(programPin, HIGH);
  digitalWrite(readPin, LOW);

}
void setAddress(uint32_t Addr) {
  for (int i = 0; i < 8; i++) {
    digitalWrite(adrPins[i], Addr & (1 << i));
  }
  Addr = Addr >> 8;
  for (int i = 0; i < 8; i++) {
    digitalWrite(adrPins[i + 8], Addr & (1 << i));
  }
  Addr = Addr >> 8;
  for (int i = 0; i < 3; i++) {
    digitalWrite(adrPins[i + 16], Addr & (1 << i));
  }
}
byte readByte(unsigned long adr) {
  byte data;
  setAddress(adr);
  digitalWrite(enablePin, LOW);
  delayMicroseconds(10);
  for (int i = 7; i >= 0; i--) {
    data = data << 1;
    data |= digitalRead(dataPins[i]) & 1;
  }
  digitalWrite(enablePin, HIGH);
  return data;
}
void setData(char Data) {
  for (int i = 0; i < 8; i++) {
    digitalWrite(dataPins[i], Data & (1 << i));
  }
}
void programByte(byte Data) {
  //select address
  //
  //setAddress(adr);
  setData(Data);
  //Vpp pulse
  delayMicroseconds(4);
  digitalWrite(enablePin, LOW);
  delayMicroseconds(60);
  digitalWrite(enablePin, HIGH);
}

void writeSector(unsigned char sectorH, unsigned char sectorL) {
  byte dataBuffer[128];
  unsigned long address = 0;
  byte CHK = sectorH, CHKreceived;
  CHK ^= sectorL;

  address = sectorH;
  address = (address << 8) | sectorL;
  address *= 128;

  for (int i = 0; i < 128; i++) {
    while (Serial.available() == 0);
    dataBuffer[i] = Serial.read();
    CHK ^= dataBuffer[i];
  }
  while (Serial.available() == 0);
  CHKreceived = Serial.read();
  programMode();
  //only program the bytes if the checksum is equal to the one received
  if (CHKreceived == CHK) {
    for (int i = 0; i < 128; i++) {
      setAddress(0x5555);
      programByte(0xAA);
      setAddress(0x2AAA);
      programByte(0x55);
      setAddress(0x5555);
      programByte(0xA0);

      setAddress(address++);
      programByte(dataBuffer[i]);
    }
    Serial.write(CHK);
  }
  readMode();

}
int readROM() {
  unsigned long num = 1024 * 1024;
  unsigned long address;
  byte data, checksum = 0;
  address = 0;
  //read mode
  readMode();
  //start frame
  digitalWrite(readPin, LOW);
  digitalWrite(programPin, HIGH);
  for (long i; i < 524288; i++) { //1048576
    data = readByte(address++);
    Serial.write(data);
    //checksum^=data;
  }
  digitalWrite(readPin, HIGH);

  //Serial.write(checksum);
  //Serial.write(0xAA);
}
int eraseROM() {
  setAddress(0x5555);
  programByte(0xAA);

  setAddress(0x2AAA);
  programByte(0x55);

  setAddress(0x5555);
  programByte(0x80);

  setAddress(0x5555);
  programByte(0xAA);

  setAddress(0x2AAA);
  programByte(0x55);

  setAddress(0x5555);
  programByte(0x10);
  
  delay(1000);
  readMode();
  byte data, checksum = 0;
  data = readByte(0xFF);
  Serial.write(data);
  //while(~(readByte(0xFF)&(1<<7)));
  //Serial.write(readByte(0xFF));
  //if(readByte(0xFF)==0xFF){
  //  Serial.write('Y');
 // }
}

int checkROM() {
  setAddress(0x5555);
  programByte(0xAA);

  setAddress(0x2AAA);
  programByte(0x55);

  setAddress(0x5555);
  programByte(0x90);

  delay(10);
//  readMode();
//  while(~(readByte(0xFF)&(1<<7)));
//  if(readByte(0xFF)==0xFF){
//    Serial.write('Y');

  unsigned long address;
  byte data, checksum = 0;
  //read mode
  readMode();
  //start frame
  //digitalWrite(readPin, LOW);
  //digitalWrite(programPin, HIGH);
  //for (long i; i < 524288; i++) { //1048576
    data = readByte(0x00);
    Serial.write(data);
    data = readByte(0x01);
    Serial.write(data); 
    data = readByte(0x02);
    Serial.write(data);
    
  setAddress(0x0000);
  programByte(0xF0); 
  
    Serial.write('Y');
}
