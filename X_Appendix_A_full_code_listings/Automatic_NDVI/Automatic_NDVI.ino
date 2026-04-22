//Automatic Wall-E 2.0 script version 1.0 (by Mathias Hoffmann)

#include "AS726X.h"
#include<Wire.h>
#define AS7263_I2C_ADDRESS 0x49
#define TCA_I2C_ADDRESS    0x70
#include <SoftwareSerial.h>
#define NUMBER_OF_SENSORS 2
#define DONEPIN 9
#include <SPI.h>
#include "SdFat.h"
#include "RTClib.h"
SdFat sd;
SdFile myFile;

const int chipSelect = 10;

int cycles = 0;
float up_ir;
float up_vis;
float down_ir;
float down_vis;
int i =0;
float NDVI;
float NIRup;
float VISup;
float NIRdown;
float VISdown;

AS726X accel;   // NIR Sensor
RTC_DS3231 rtc;

void setup() {
delay(1500);
pinMode(DONEPIN, OUTPUT);
  digitalWrite(DONEPIN, LOW);              // LOW until its time to cut power

  Wire.begin();
  Serial.begin(9600);
    setTCAChannel(1);
    delay(1500);
accel.begin(); // Init the sensor connected to this0 port
    setTCAChannel(7);
    delay(1500);
accel.begin(); // Init the sensor connected to this port
    sd.begin(chipSelect, SPI_HALF_SPEED);
    setTCAChannel(5);
    rtc.begin();
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void loop() {  
    while(i<31){
    
    setTCAChannel(5);                                                                                                                                        
    DateTime now = rtc.now();
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" ");
    Serial.print(now.hour(), DEC);
    Serial.print(':'); 
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
  
   myFile.open("test.txt", FILE_WRITE);
     setTCAChannel(5);
     myFile.print(now.year(), DEC);
     myFile.print('/');
     myFile.print(now.month(), DEC);
     myFile.print('/');
     myFile.print(now.day(), DEC);
     myFile.print(" ");
     myFile.print(now.hour(), DEC);
     myFile.print(':');
     myFile.print(now.minute(), DEC);
     myFile.print(':');
     myFile.print(now.second(), DEC);
  
     setTCAChannel(1);
     accel.takeMeasurements();
     up_ir=accel.getCalibratedS();
     delay(15);
     myFile.print(";");
     myFile.print(accel.getCalibratedR(), 2);
     delay(15);
     myFile.print(";");
     myFile.print(accel.getCalibratedS(), 2);
     delay(15);
     myFile.print(";");
     myFile.print(accel.getCalibratedT(), 2);
     delay(15);
     myFile.print(";");
     myFile.print(accel.getCalibratedU(), 2);
     delay(15);
     myFile.print(";");
     myFile.print(accel.getCalibratedV(), 2);
     delay(15);
     myFile.print(";");
     myFile.print(accel.getCalibratedW(), 2);

     VISup=accel.getCalibratedS();
     NIRup=accel.getCalibratedU()+accel.getCalibratedV();
    
     setTCAChannel(7);
     accel.takeMeasurements();
     down_ir=accel.getCalibratedS();
     delay(15);
     myFile.print(";");
     myFile.print(accel.getCalibratedR(), 2);
     delay(15);
     myFile.print(";");
     myFile.print(accel.getCalibratedS(), 2);
     delay(15);
     myFile.print(";");
     myFile.print(accel.getCalibratedT(), 2);
     delay(15);
     myFile.print(";");
     myFile.print(accel.getCalibratedU(), 2);
     delay(15);
     myFile.print(";");
     myFile.print(accel.getCalibratedV(), 2);
     delay(15);
     myFile.print(";");
     myFile.print(accel.getCalibratedW(), 2);
     delay(15);

     VISdown=accel.getCalibratedS();
     NIRdown=accel.getCalibratedU()+accel.getCalibratedV();
     NDVI=((NIRdown/NIRup)-(VISdown/VISup))/((NIRdown/NIRup)+(VISdown/VISup));
     myFile.print(";");
     myFile.println(NDVI, 2);
     Serial.print(";");
     Serial.println(NDVI);
    
   myFile.close(); 
  delay(900); 
 
 i++;

}  
  delay(15); 

  digitalWrite(DONEPIN, HIGH); // Restore power
}

void setTCAChannel(byte i){
  Wire.beginTransmission(TCA_I2C_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();  
}
