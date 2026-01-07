// 8 Channel Transmitter & Trims | 8 Kanal Verici ve Trimler

  #include <SPI.h>
  #include <nRF24L01.h>
  #include <RF24.h>
  #include <EEPROM.h>
    
  const uint64_t pipeOut = 000322;        // NOTE: The same as in the receiver 000322 | Alıcı kodundaki adres ile aynı olmalı
  RF24 radio(9, 10);                      // Select CE,CSN pin | CE ve CSN pinlerin seçimi
  
 #define trimbut_1 1                      // Trim button 1 / Pin D1
 #define trimbut_2 2                      // Trim button 2 / Pin D2
 #define trimbut_3 3                      // Trim button 3 / Pin D3
 #define trimbut_4 4                      // Trim button 4 / Pin D4
 #define trimbut_5 5                      // Trim button 5 / Pin D5
 #define trimbut_6 6                      // Trim button 6 / Pin D6
 
 int tvalue1 = EEPROM.read(1) * 4;        // Reading trim values from Eprom  |  Trim değerlerinin Epromdan okunması
 int tvalue2 = EEPROM.read(3) * 4;        
 int tvalue3 = EEPROM.read(5) * 4;        

  struct Signal {
  byte throttle;
  byte pitch;
  byte roll;
  byte yaw;
  byte aux1;
  byte aux2;
  byte aux3;
  byte aux4;  
};

  Signal data;
  void ResetData() 
{
  data.throttle = 512;                      // Signal lost position | Sinyal kesildiğindeki pozisyon
  data.pitch = 127;
  data.roll = 127;
  data.yaw = 127;
  data.aux1 = 0;                         
  data.aux2 = 0;
  data.aux3 = 0;
  data.aux4 = 0;
}
  void setup()
{
                                         // Configure the NRF24 module  | NRF24 modül konfigürasyonu
  radio.begin();
  radio.openWritingPipe(pipeOut);
  radio.setChannel(100);
  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);       // The lowest data rate value for more stable communication  | Daha kararlı iletişim için en düşük veri hızı.
  radio.setPALevel(RF24_PA_MAX);         // Output power is set for maximum |  Çıkış gücü maksimum için ayarlanıyor.
  radio.stopListening();                 // Start the radio comunication for Transmitter | Verici için sinyal iletişimini başlatır.
  ResetData();
 
  pinMode(trimbut_1, INPUT_PULLUP); 
  pinMode(trimbut_2, INPUT_PULLUP);
  pinMode(trimbut_3, INPUT_PULLUP); 
  pinMode(trimbut_4, INPUT_PULLUP);
  pinMode(trimbut_5, INPUT_PULLUP); 
  pinMode(trimbut_6, INPUT_PULLUP);

  tvalue1= EEPROM.read(1) * 4;
  tvalue2= EEPROM.read(3) * 4;
  tvalue3= EEPROM.read(5) * 4;
}

// Joystick center and its borders | Joystick merkez ve sınırları
  int Border_Map(int val, int lower, int middle, int upper, bool reverse)
{
  val = constrain(val, lower, upper);
  if ( val < middle )
  val = map(val, lower, middle, 0, 128);
  else
  val = map(val, middle, upper, 128, 255);
  return ( reverse ? 255 - val : val );
}
  void loop()
{

// Trims and Limiting trim values  |  Trimler ve Trim değerlerini sınırlandırma

  if(digitalRead(trimbut_1)==LOW and tvalue1 < 630) {
    tvalue1=tvalue1+15;
    EEPROM.write(1,tvalue1/4); 
    delay (130);
  }   
  if(digitalRead(trimbut_2)==LOW and tvalue1 > 280){
    tvalue1=tvalue1-15;
    EEPROM.write(1,tvalue1/4);
    delay (130);
  }
 
  if(digitalRead(trimbut_3)==LOW and tvalue2 < 630) {
    tvalue2=tvalue2+15;
    EEPROM.write(3,tvalue2/4);
    delay (130);
  }   
  if(digitalRead(trimbut_4)==LOW and tvalue2 > 280){
    tvalue2=tvalue2-15;
    EEPROM.write(3,tvalue2/4);
    delay (130);
  }  
    
    if(digitalRead(trimbut_5)==LOW and tvalue3 < 630) {
     tvalue3=tvalue3+15;
     EEPROM.write(5,tvalue3/4);
     delay (130);
  }   
  if(digitalRead(trimbut_6)==LOW and tvalue3 > 280){
    tvalue3=tvalue3-15;
    EEPROM.write(5,tvalue3/4);
    delay (130);
  }
  
// Control Stick Calibration for channels  |  Her bir kanal için kumanda Kol Kalibrasyonları 

  data.roll = Border_Map( analogRead(A3), 0, tvalue1, 1023, true );       // "true" or "false" for signal direction | "true" veya "false" sinyal yönünü belirler
  data.pitch = Border_Map( analogRead(A2), 0, tvalue2, 1023, true );            
  data.throttle = Border_Map( analogRead(A1),570, 800, 1023, false );    // For Single side ESC | Tek yönlü ESC için
  // data.throttle = Border_Map( analogRead(A1),0, 512, 1023, false );   // For Bidirectional ESC | Çift yönlü ESC için
  data.yaw = Border_Map( analogRead(A0), 0, tvalue3, 1023, true );              
  data.aux1 = Border_Map( analogRead(A4), 0, 512, 1023, true );                 
  data.aux2 = Border_Map( analogRead(A5), 0, 512, 1023, true );                 
  data.aux3 = digitalRead(7);
  data.aux4 = digitalRead(8);
  radio.write(&data, sizeof(Signal)); 
}
