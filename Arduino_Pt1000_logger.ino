//Pt1000 Amplifier lib
#include <Adafruit_MAX31865.h>
//RTC lib
#include "RTClib.h"
RTC_DS1307 rtc;
long startMs = 0; //start record for runtime
//SDCard lib
#include <SPI.h>
#include <SD.h>
const int chipSelect = 4; //cs for SD card
int okPin = 8;            //sensor connected
int failPin = 7;          //no SD card LED
int workPin = 2;          //measurement
//software SPI: CS, DI, DO, CLK
Adafruit_MAX31865 thermo = Adafruit_MAX31865(9, 5, 6, 3);
#define RREF      4300.0 // Rref, 4300.0 for PT1000 (use 430.0 for Pt100)
#define RNOMINAL  1000.0 // nominal 0-degrees-C resistance

void setup() {
  Serial.begin(9600);
  // SD Card
  Serial.print("Initializing SD card...");
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
  }
  Serial.println("card initialized.");
  //indicators
  pinMode(okPin, OUTPUT);
  pinMode(failPin, OUTPUT);
  pinMode(workPin, OUTPUT);
  //RTC
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  Serial.println("RTC found");
  //Amplifier
  thermo.begin(MAX31865_3WIRE);
  //
  startMs = millis(); //starttime
}

void loop() {
  //set indicators
  digitalWrite(failPin, LOW);
  digitalWrite(workPin, LOW);
  digitalWrite(okPin, HIGH);
  //read
  DateTime now = rtc.now();
  digitalWrite(workPin, HIGH);
  uint16_t rtd = thermo.readRTD();
  //read timepoint, translate from millis
  long elapsed = (millis() - startMs);
  int elapsedh = (elapsed/3600000);
  int elapsedm = ((elapsed/60000)-(elapsedh*60));
  int elapseds = ((elapsed/1000)-(elapsedm*60));
  //  Serial.print("RTD value: "); Serial.println(rtd);
  float ratio = rtd;
  ratio /= 32768;
  //  Serial.print("Ratio = "); Serial.println(ratio,8);
  //  Serial.print("Resistance = "); Serial.println(RREF*ratio,8);
  //reporting
  Serial.print("Temperature = "); Serial.println(thermo.temperature(RNOMINAL, RREF));
  String dataString1 = "Temperature = ";
  String dataString2 = "recording since: ";
  float sensor = (thermo.temperature(RNOMINAL, RREF));
  dataString1 += String(sensor);
  delay(1000); //basically for work LED
  digitalWrite(workPin, LOW);
  //write to SD
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.println();
    dataFile.print(now.year(), DEC);
    dataFile.print('/');
    dataFile.print(now.month(), DEC);
    dataFile.print('/');
    dataFile.print(now.day(), DEC);
    dataFile.print(' ');
    dataFile.print(now.hour(), DEC);
    dataFile.print(':');
    dataFile.print(now.minute(), DEC);
    dataFile.print(':');
    dataFile.print(now.second(), DEC);
    dataFile.println();
    dataFile.println(dataString1);
    dataFile.print(dataString2);
    dataFile.print(elapsedh);
    dataFile.print('h');
    dataFile.print(',');
    dataFile.print(elapsedm);
    dataFile.print('m');
    dataFile.print(',');
    dataFile.print(elapseds);
    dataFile.println('s');
    dataFile.close();
    // print to the serial port too:
    Serial.println(elapsedm);
    Serial.println(elapsed);
  }
  else {
    Serial.println("error opening datalog.txt");
    digitalWrite(failPin, HIGH);
  }
  //fault reporting for Amplifier
  uint8_t fault = thermo.readFault();
  if (fault) {
    Serial.print("Fault 0x"); Serial.println(fault, HEX);
    if (fault & MAX31865_FAULT_HIGHTHRESH) {
      Serial.println("RTD High Threshold"); 
    }
    if (fault & MAX31865_FAULT_LOWTHRESH) {
      Serial.println("RTD Low Threshold"); 
    }
    if (fault & MAX31865_FAULT_REFINLOW) {
      Serial.println("REFIN- > 0.85 x Bias"); 
    }
    if (fault & MAX31865_FAULT_REFINHIGH) {
      Serial.println("REFIN- < 0.85 x Bias - FORCE- open"); 
    }
    if (fault & MAX31865_FAULT_RTDINLOW) {
      Serial.println("RTDIN- < 0.85 x Bias - FORCE- open"); 
    }
    if (fault & MAX31865_FAULT_OVUV) {
      Serial.println("Under/Over voltage"); 
    }
    thermo.clearFault();
    digitalWrite(okPin, LOW);
  }
  Serial.println();
  delay(28000);
}
//Wohlmann_09.09.2020
