////////////////////////////////////////////////////////////////////////////////
//                 *** Motion Detector Shield Arduino Code ***                //
//                                                                            //
// This programm should be used together with Motion Detector Shield and an   // 
// Arduino Uno > Rev.3. as well as an Adafruit Data Logging Shield. Much of   // 
// the code is inspired by Adafruits excellent documentation and tutorials.   //
// See https://www.adafruit.com/product/1141                                  //
//                                                                            //
//                                                                            //  
//                            Andreas Genewsky (2016)                         //
//              Max-Planck Institute for Psychiatry, Munich, Germany          //
//                                                                            //
//    This program is free software: you can redistribute it and/or modify    //
//    it under the terms of the GNU General Public License as published by    //
//    the Free Software Foundation, either version 3 of the License, or       //
//    (at your option) any later version.                                     //
//                                                                            //  
//    This program is distributed in the hope that it will be useful,         //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of          //  
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           //
//    GNU General Public License for more details.                            //
//                                                                            //
//    You should have received a copy of the GNU General Public License       //
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.   // 
//                                                                            //
////////////////////////////////////////////////////////////////////////////////



#include <SD.h>
#include <Wire.h>
#include "RTClib.h"

#define ECHO_TO_SERIAL   1 // echo data to serial port
#define WRITE_TO_SD   1 // initialize SD and write data to SD

int flopRST = 9;  // SN74LS423 RESET Pin ... read SN74LS423 documentation
int SENS_A = 3;   // channel A 
int SENS_B = 4;   // channel B
int SENS_C = 5;   // channel C
int SENS_D = 6;   // channel D
int SENS_E = 7;   // channel E
int SENS_F = 8;   // channel F
int AMBIENT = 14; // corresponds to A0 on Arduino UNO, TEMT6000 lightsensor
int AD1 = 15;     // optional analog input
int AD2 = 16;     // optional analog input
int AD3 = 17;     // optional analog input
float lux = 0.0;  // value necessary for lux calculation
float lplux = 0.0;  // value necessary for lux calculation
int sensors[6];   // motion sensor array
volatile byte state = LOW;  // a variable which changes if motion was detected
int ledPin = 13;  // the Arduino onboard LED
int bootup = 0; // a placeholder for the time in milliseconds at bootup
unsigned int ms = 0;  // a placeholder for the milliseconds between the RTC seconds


//RTC_DS1307 RTC; // define the Real Time Clock object, use the RT which
RTC_PCF8523 RTC;  // corresponds to the one present on your board

// for the data logging shield, we use digital pin 10 for the SD cs line
const int chipSelect = 10;
// the logging file
File logfile;

void error(char *str) // the definition of an error function
{
  Serial.print("error: ");
  Serial.println(str);
  while(1);
}

// here we set the date- and timestamp the logging file
void dateTime(uint16_t* date, uint16_t* time) 
{
  DateTime now = RTC.now();
  *date = FAT_DATE(now.year(), now.month(), now.day());
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}

void setup() {  // the setup function begins

  Serial.begin(57600); // for some debugging purposes
 
  if (! RTC.begin()) { // starting the RealTimeClock (RTC)
    Serial.println("Couldn't find RTC");
    while (1);
  }
  // Here we set the clock according to the CPU Time of the programing PC
  RTC.adjust(DateTime(F(__DATE__), F(__TIME__))); 
  // immediately afterwards we get the milliseconds since bootup 
  bootup = millis(); 
  
  // no we set the INPUT & OUTPUT Pins
  pinMode(flopRST, OUTPUT);
  pinMode(SENS_A, INPUT);
  pinMode(SENS_B, INPUT);
  pinMode(SENS_C, INPUT);
  pinMode(SENS_D, INPUT);
  pinMode(SENS_E, INPUT);
  pinMode(SENS_F, INPUT);
  pinMode(AMBIENT, INPUT);
  pinMode(AD1, INPUT);
  pinMode(AD2, INPUT);
  pinMode(AD3, INPUT);
 
  attachInterrupt(0, detected, FALLING); 
  // this links a +5V voltage level at pin 2 (Arduino Interrupt Pin = pinnumber 0) 
  // to the execution of the function 'detected'
  digitalWrite(flopRST, HIGH);

#if WRITE_TO_SD
  // we initialize the SD card, and check if we can write on it
  Serial.print("Initializing SD card...");
  pinMode(chipSelect, OUTPUT);
  if (!SD.begin(chipSelect)) {
    error("Card failed, or not present");
  }
  Serial.println("card initialized.");

  // this function generates filenames with incrementing numbers
  char filename[] = "MOTION00.CSV"; 
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      SdFile::dateTimeCallback(dateTime);
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // leave the loop!
    }
  }
  
  if (! logfile) {
    error("couldnt create file");
  }
  
  Serial.print("Logging to: ");
  Serial.println(filename);
  // the next line writes the column descriptors to the logging file
  logfile.println("MONTH,DAY,YEAR,HH,MM,SS,mmm,CH1,CH2,CH3,CH4,CH5,CH6,LUX");    
#endif WRITE_TO_SD

// in order to blank any strange behavior of the SN74LS423 we RESET all of them before we log
digitalWrite(flopRST, LOW);
digitalWrite(ledPin, HIGH);
delay(50);
digitalWrite(ledPin, LOW);
digitalWrite(flopRST, HIGH);
delay(50);

} //end of SETUP

// the loop routine runs over and over again forever:
void loop() {
  DateTime now; // here we get the time every loop
  // we calculate the light intensity in lux
  // the conversion from voltage to microamps to lux can be found in the datasheet of the TEMT6000
  lux = (analogRead(AMBIENT) * 0.9765625) * 0.1 + lplux * 0.9; 
  lplux = lux;
  now = RTC.now();
  ms = (millis()-bootup)%1000; //here we calculate the <1000 milliseconds 

  if (state == HIGH) {          // we enter this loop if an interrupt event has happend 
    detachInterrupt(0);         // ('detected function has run)
    digitalWrite(flopRST, LOW); // to avoid problems we detach the interrupts
    digitalWrite(ledPin, HIGH);

    #if ECHO_TO_SERIAL
      // we will print CommaSeparatedValues (CSV) at the Serial Monitor for:
      // MONTH,DAY,YEAR,HOUR,MINUTES,SECONDS,MILLISECONDS,CH1,CH2,CH3,CH4,CH5,CH6,LUX
      if (now.month() < 10) {
        Serial.print(0);
      } 
      Serial.print(now.month(), DEC);
      Serial.print(",");
      if (now.day() < 10) {
        Serial.print(0);
      }
      Serial.print(now.day(), DEC);
      Serial.print(",");
      Serial.print(now.year(), DEC);
      Serial.print(",");
      if (now.hour() < 10) {
        Serial.print(0);
      }
      Serial.print(now.hour(), DEC);
      Serial.print(",");
      if (now.minute() < 10) {
        Serial.print(0);
      }
      Serial.print(now.minute(), DEC);
      Serial.print(",");
      if (now.second() < 10) {
        Serial.print(0);
      }
      Serial.print(now.second(), DEC);
      Serial.print(",");
      if (ms < 10) {
        Serial.print("00");
      }
      if ( (ms >= 10) && (ms < 100) ) {
        Serial.print(0);
      }
      Serial.print(ms, DEC);
      
      for (int i = 0; i < 6; i++) {
        Serial.print(",");
        Serial.print(sensors[i]);
      }
      Serial.print(',');
      Serial.println(lux, 1);
    #endif ECHO_TO_SERIAL

    #if WRITE_TO_SD
       // we will write some CommaSeparatedValues (CSV) to the logfile for:
      // MONTH,DAY,YEAR,HH,MM,SS,mmm,CH1,CH2,CH3,CH4,CH5,CH6,LUX
      if (now.month() < 10) {
        logfile.print(0);
      }
      logfile.print(now.month(), DEC);
      logfile.print(",");
      if (now.day() < 10) {
        logfile.print(0);
      }
      logfile.print(now.day(), DEC);
      logfile.print(",");
      logfile.print(now.year(), DEC);
      logfile.print(",");
      if (now.hour() < 10) {
        logfile.print(0);
      }
      logfile.print(now.hour(), DEC);
      logfile.print(",");
      if (now.minute() < 10) {
        logfile.print(0);
      }
      logfile.print(now.minute(), DEC);
      logfile.print(",");
      if (now.second() < 10) {
        logfile.print(0);
      }
      logfile.print(now.second(), DEC);
      logfile.print(",");
      if (ms < 10) {
        logfile.print("00");
      }
      if ( (ms >= 10) && (ms%1000 < 100) ) {
        logfile.print(0);
      }
      logfile.print(ms, DEC);
      for (int i = 0; i < 6; i++) {
        logfile.print(",");
        logfile.print(sensors[i]);
        sensors[i] = 0;
      }
      logfile.print(',');
      logfile.println(lux, 1);

      // flush() actually writes the data to the SD card and is very important
      logfile.flush();
    #endif WRITE_TO_SD

    // here we basically write 0's in our sensor array to be able to store new events 
    sensors[6];

    // than we RESET the SN74LS423 IC's
    delay(10);
    digitalWrite(ledPin, LOW);
    digitalWrite(flopRST, HIGH);
    delay(10);
    // now we arm our interrupt routine again
    state = LOW; 
    attachInterrupt(0, detected, FALLING);
  }
}

void detected() {
  // the interrupt routine simply checks all the sensor ports if something happend (0-5V)
  if (state == LOW) {
    detachInterrupt(0);
    sensors[0] = digitalRead(SENS_A);
    sensors[1] = digitalRead(SENS_B);
    sensors[2] = digitalRead(SENS_C);
    sensors[3] = digitalRead(SENS_D);
    sensors[4] = digitalRead(SENS_E);
    sensors[5] = digitalRead(SENS_F);
    state = HIGH;

    int sum = 0;
    for (int i = 0; i < 6; i++) {
      sum = sum + sensors[i];
    }
    if (sum == 0) {
      state = LOW;
      attachInterrupt(0, detected, FALLING);
    }
  }
}




