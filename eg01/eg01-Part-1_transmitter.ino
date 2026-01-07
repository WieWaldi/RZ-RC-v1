// 1 Channel Transmitter | 1 Kanal Verici
// Input pin A5

  #include <SPI.h>
  #include <nRF24L01.h>
  #include <RF24.h>
  const uint64_t pipeOut = 000322;    // NOTE: The same as in the receiver 000322 | Alıcı kodundaki adres ile aynı olmalı
  RF24 radio(9, 10);                          // select CE,CSN pin | CE ve CSN pinlerin seçimi

  struct Signal {
  byte aux1;
  byte aux3;
  
};
  Signal data;
  void ResetData() 
{
  data.aux1 = 0;                       // Signal lost position | Sinyal kesildiğindeki pozisyon
  data.aux3 = 0;
}
  void setup()
{
                                       //Configure the NRF24 module  | NRF24 modül konfigürasyonu
  radio.begin();
  radio.openWritingPipe(pipeOut);
  radio.setChannel(100);
  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);    // The lowest data rate value for more stable communication  | Daha kararlı iletişim için en düşük veri hızı.
  radio.setPALevel(RF24_PA_MAX);      // Output power is set for maximum |  Çıkış gücü maksimum için ayarlanıyor.
  radio.stopListening();              // Start the radio comunication for Transmitter | Verici için sinyal iletişimini başlatır.
  ResetData();
 
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
                                     // Control Stick Calibration for channels  |  Her bir kanal için kumanda Kol Kalibrasyonları 
  data.aux1 = Border_Map( analogRead(A5), 0, 512, 1023, true );   // "true" or "false" for change signal direction | "true" veya "false" sinyal yönünü değiştirir.
  
  data.aux3 = digitalRead(7);
  radio.write(&data, sizeof(Signal));  
}
