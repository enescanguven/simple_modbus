#include "DHT.h"
#define DHTPIN 8
#define DHTTYPE DHT11
#define READ_HOLDING_REGISTERS 0x03
#define WRITE_MULTIPLE_HOLDING_REGISTERS 0X10
DHT dht(DHTPIN, DHTTYPE);
unsigned long currTime;
unsigned long prevTime;
float t = 0;
float h = 0;
//#define HEATER_LED 12
//#define HUMIDIFIER_LED 18
#define tx_enable 19
unsigned char frame[24];
unsigned char frame_snd[24];
unsigned char pwh_frame_snd[24];
unsigned int T1_5;
unsigned int T3_5;// inter character time out in microseconds
#define baud 115200
unsigned char slaveID = 3;
//unsigned int sicaklik;
float sicaklik = 0;
float mData[2];
unsigned int heater_state = 0;
unsigned int states[2];
unsigned int humidifier_state = 0;
struct Packet {

  unsigned char id;
  unsigned char function;
  unsigned char address;
  unsigned int no_of_registers;
  unsigned int* register_array;

};

Packet packet;


void setup() {
  Serial.begin(baud);
  T1_5 = 15000000 / baud;
  T3_5 = 35000000 / baud;
  pinMode(tx_enable, OUTPUT);
  
  digitalWrite(tx_enable, HIGH);
  delay(1000);
  digitalWrite(tx_enable, LOW);
  
  pinMode(3, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(4, OUTPUT);
 
  dht.begin();

  digitalWrite(4, HIGH);
  delay(1000);
  digitalWrite(4, LOW);


}

void loop() {
  //  sicaklik = analogRead(analogPin);
  currTime = millis();
  if (currTime - prevTime >= 2000) {
    mData[0] = dht.readTemperature();
    mData[1] = dht.readHumidity();
  }
  unsigned char buffer_ = 0;
  unsigned char overflow = 0;

  while (Serial.available()) {
    //    Serial.println("asdas"); //buraya giriyor
    frame[buffer_] = Serial.read();
    buffer_++;
    delayMicroseconds(T1_5);

  }


  if (buffer_ > 6) {
    //    Serial.println("asdas");
    //    Serial.println(frame[0]);
    //    Serial.println(frame[1]);

    packet.id = frame[0];
    packet.function = frame[1];
    unsigned char startingAddress = ((frame[2] << 8) | frame[3]);
    packet.address = ((frame[2] << 8) | frame[3]);
    //    packet.no_of_registers =  ((frame[4] << 8) | frame[5]);
    //    unsigned int maxData = packet.address + packet.no_of_registers;
    unsigned char index;
    //    unsigned char noOfBytes = packet.no_of_registers * 2;

    if (packet.id == slaveID) {
            packet.no_of_registers =  ((frame[4] << 8) | frame[5]);
            unsigned int maxData = packet.address + packet.no_of_registers;
            unsigned char noOfBytes = packet.no_of_registers * 2;

      if (packet.function == READ_HOLDING_REGISTERS) { //frame 1 yapalim
//        packet.no_of_registers =  ((frame[4] << 8) | frame[5]);
//        unsigned int maxData = packet.address + packet.no_of_registers;
//        unsigned char noOfBytes = packet.no_of_registers * 2;
        unsigned int crc_calculated = calculateCRC(buffer_ - 2, frame);
        //        Serial.println(crc_calculated);
        unsigned int crc_received = ((frame[buffer_ - 2] << 8) | frame[buffer_ - 1]);
        //        Serial.println(crc_received);

        if (crc_calculated == crc_received) {
          digitalWrite(4, HIGH);

          frame_snd[0] = slaveID;
          frame_snd[1] = READ_HOLDING_REGISTERS;
          frame_snd[2] = noOfBytes;
          unsigned char responseFrameSize = 5 + noOfBytes;
          unsigned char address = 3; // PDU starts at the 4th byte
          unsigned int temp;

          for (index = startingAddress; index < maxData; index++) {

            temp = mData[index]; // elemanin yazdigi data

            frame_snd[address] = temp >> 8;
            address++;
            frame_snd[address] = temp & 0xFF;
            address++;
          }
          frame_snd[responseFrameSize - 2] = 0xFF;
          frame_snd[responseFrameSize - 1] = 0xFF;

          digitalWrite(tx_enable, HIGH);
          for (unsigned char i = 0; i < responseFrameSize; i++) {
            Serial.write(frame_snd[i]);
          }
          Serial.flush();
          delayMicroseconds(T3_5);
          digitalWrite(tx_enable, LOW);

        }



      }
      else if (packet.function == WRITE_MULTIPLE_HOLDING_REGISTERS) {

        pwh_frame_snd[0] = slaveID;
        pwh_frame_snd[1] = WRITE_MULTIPLE_HOLDING_REGISTERS;
        unsigned char address = 6; // PDU starts at the 4th byte
        heater_state = ((frame[6] << 8) | frame[7]);
        humidifier_state = ((frame[8] << 8) | frame[9]);
        Serial.print(humidifier_state);
        
//        for (index = startingAddress; index < maxData; index++) {
//          states[index] = ((frame[address] << 8) | frame[address+1]);
//          address=address+2;
//          }

//        packet.no_of_registers =  ((frame[4] << 8) | frame[5]);
//        unsigned int maxData = packet.address + packet.no_of_registers;
//        unsigned char noOfBytes = packet.no_of_registers * 2;
//        unsigned char responseFrameSize = 5 + noOfBytes;
//        unsigned char address = 3;
//        unsigned int temp;

        
//        heater_state = states[0];
//        humidifier_state = states[1];
//        Serial.println(heater_state);
//        Serial.println(humidifier_state);

      }
    }

  }
  if (heater_state == 1) {
    digitalWrite(3, HIGH);
  }
  else {
    digitalWrite(3, LOW);
  }

  if (humidifier_state == 1) {
    digitalWrite(5, HIGH);
  }
  else {
    digitalWrite(5, LOW);
  }
  
  digitalWrite(4, LOW);



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
