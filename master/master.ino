
#define READ_HOLDING_REGISTERS 0x03
#define WRITE_MULTIPLE_HOLDING_REGISTERS 0X10
#define tx_enable 19
#define slaveID 0x03
#define baud 115200

#define polling 200 // the scan rate
unsigned int T1_5; // inter character time out in microseconds
unsigned int T3_5; // frame delay in microseconds
unsigned char frame[24]; //frame to send 
unsigned char frame_rcv[24]; //frame 
unsigned char  pwh_frame[24];
//unsigned int sicaklik;
float sicaklik;
float nem;

// used to toggle the receive/transmit pin on the driver
#define TxEnablePin 2
#define baud 115200
enum {
  PACKET,
  TOTAL_NO_OF_PACKETS
};

struct Packet {

  unsigned char id;
  unsigned char function;
  unsigned char address;
  unsigned int no_of_registers;
  unsigned int* register_array;

};


//typedef Packet* packetPointer;

//Packet packets[TOTAL_NO_OF_PACKETS];

//packetPointer packet = &packets[PACKET];
Packet packet;
Packet pwh; // packet for write heaters register

unsigned int regs[9];
unsigned int pwh_regs[9];

void setup() {
  packet.id = slaveID;
  packet.function = READ_HOLDING_REGISTERS;
  packet.address = 0;
  packet.no_of_registers = 2;
  packet.register_array = regs;

  pwh.id = slaveID;
  pwh.function = WRITE_MULTIPLE_HOLDING_REGISTERS;
  pwh.address = 0;
  pwh.no_of_registers = 2;
  pwh.register_array = pwh_regs;

  Serial.begin(baud);
  T1_5 = 15000000 / baud;
  T3_5 = 35000000 / baud;
  pinMode(tx_enable, OUTPUT);
  digitalWrite(tx_enable, LOW);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
  digitalWrite(4, LOW);
  digitalWrite(3, HIGH);
  delay(1000);
  digitalWrite(3, LOW);

}

void loop() {
  unsigned int crc16;

  frame[0] = packet.id;
  frame[1] = packet.function;
  frame[2] = packet.address >> 8; // address Hi
  frame[3] = packet.address & 0xFF; // address Lo
  frame[4] = packet.no_of_registers >> 8; // no_of_registers Hi
  frame[5] = packet.no_of_registers & 0xFF; // no_of_registers Lo
  crc16 = calculateCRC(6, frame);
  frame[6] = crc16 >> 8; //crc16 = calculateCRC(6,frame);
  frame[7] = crc16 & 0xFF;

  pwh_frame[0] = pwh.id;
  pwh_frame[1] = pwh.function;
  unsigned char noOfBytes = pwh.no_of_registers * 2;
  pwh_frame[2] = pwh.address >> 8; // address Hi
  pwh_frame[3] = pwh.address & 0xFF; // address Lo
  pwh_frame[4] = pwh.no_of_registers >> 8; // no_of_registers Hi
  pwh_frame[5] = pwh.no_of_registers & 0xFF;
  pwh_frame[6] = pwh.register_array[0] >> 8; // no_of_registers Hi
  pwh_frame[7] = pwh.register_array[0] & 0xFF; //crc16 = calculateCRC(6,frame); sicaklik

  pwh_frame[8] = pwh.register_array[1] >> 8; // no_of_registers Hi
  pwh_frame[9] = pwh.register_array[1] & 0xFF; //crc16 = calculateCRC(6,frame); nem

  crc16 = calculateCRC(8, pwh_frame);
  pwh_frame[10] = crc16 >> 8;
  pwh_frame[11] = crc16 & 0xFF;

  digitalWrite(tx_enable, HIGH);
  for (unsigned char i = 0; i < 8; i++)
    Serial.write(frame[i]);
  Serial.flush();
  delayMicroseconds(T1_5);
  digitalWrite(tx_enable, LOW);
  delay(100);
  unsigned char buffer_r = 0;
  while (Serial.available()) {
    frame_rcv[buffer_r] = Serial.read();
    buffer_r++;
    delayMicroseconds(T1_5);
  }

  if (buffer_r > 6) {
    if (frame_rcv[0] == slaveID) {
      if (frame_rcv[1] == READ_HOLDING_REGISTERS) {
        unsigned char index = 3;
        for (unsigned char i = 0; i < packet.no_of_registers; i++) {
          // start at the 4th element in the recieveFrame and combine the Lo byte
          packet.register_array[i] = (frame_rcv[index] << 8) | frame_rcv[index + 1];
          index += 2;
        }
      }
    }
  }

  digitalWrite(tx_enable, HIGH);
  for (unsigned char i = 0; i < 12; i++)
    Serial.write(pwh_frame[i]);
  Serial.flush();
  delayMicroseconds(T1_5);
  digitalWrite(tx_enable, LOW);
  delay(100);


  sicaklik = packet.register_array[0];
  nem = packet.register_array[1];
  Serial.println(sicaklik);
  Serial.println(nem);
  Serial.println(frame_rcv[1]);
  if (sicaklik <= 40) {
    //    digitalWrite(4, HIGH);
    pwh.register_array[0] = 1;
  }
  else {
    //    digitalWrite(4, LOW);
    pwh.register_array[0] = 0;
  }

  if (nem <= 40) {
    //    digitalWrite(4, HIGH);
    pwh.register_array[1] = 1;
  }
  else {
    //    digitalWrite(4, LOW);
    pwh.register_array[1] = 0;
  }

  delay(1000);


}

unsigned int calculateCRC(unsigned char bufferSize, unsigned char frame[])
{
  unsigned int temp, temp2, flag;
  temp = 0xFFFF;
  for (unsigned char i = 0; i < bufferSize; i++) {
    temp = temp ^ frame[i];
    for (unsigned char j = 1; j <= 8; j++) {
      flag = temp & 0x0001;
      temp >>= 1;
      if (flag)
        temp ^= 0xA001;
    }
  }
  // Reverse byte order.
  temp2 = temp >> 8;
  temp = (temp << 8) | temp2;
  temp &= 0xFFFF;
  return temp; // the returned value is already swopped - crcLo byte is first & crcHi byte is last
}
