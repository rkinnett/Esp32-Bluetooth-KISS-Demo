// Bluetooth KISS Demo
// R. Kinnett, 2020
//
// Demonstrates KISS interface over Bluetooth for AX25 packet radio.
// This demo provides KISS interface framework only.
// This demo does not include AFSK modulation/demodulation.
//
// This demo logs to (USB) serial interface.
// SerialBT and Serial interfaces are interchangeable! 
// Either of these interfaces may be used for KISS interface.
// More generally, any Stream object may be assigned to KISS!
//
// KISS protocol:  https://en.wikipedia.org/wiki/KISS_(TNC)
// 

#include "BluetoothSerial.h"
#include "kiss.h"

BluetoothSerial SerialBT;

Kiss kiss;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32test"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");

  kiss.setLogTerminal(&Serial);
  kiss.setKissTerminal(&SerialBT);
}

void loop() {
  kiss.receiveFromHost();
  delay(1);
}
