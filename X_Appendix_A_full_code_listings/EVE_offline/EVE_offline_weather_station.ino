#include <Wire.h>
#include <RTClib.h>
//#include <Adafruit_SHT31.h>
#include "Adafruit_SHT4x.h"
#include <Adafruit_BMP280.h>

#define FRAM_I2C_ADDR  0x50
#define ADDRESS_STORE  0x0000   // Where the write pointer is stored
#define DATA_START     0x0010   // Leave first 16 bytes empty to avoid overlap
#define RECORD_SIZE    20        // 4 (timestamp) + 4 (air temperature) + 4 (humidity) + 4 (PAR) + 4 (air pressure)
#define DONE_PIN 9  // Pin connected to TPL5110 DONE

RTC_DS3231 rtc;
//Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_SHT4x sht4 = Adafruit_SHT4x();
String inputBuffer;
Adafruit_BMP280 bmp; // I2C

void setup() {
  Wire.begin();
  Serial.begin(9600);
  pinMode(DONE_PIN, OUTPUT);
  digitalWrite(DONE_PIN, LOW);  // Ensure it's low initially

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  Serial.println(rtc.lostPower());

 if (!bmp.begin(0x76)) {  // Or 0x77 depending on your module
    Serial.println("Could not find BMP280 sensor!");
    while (1);
  }
  
  //if (!sht31.begin(0x44)) {
  //  Serial.println("Couldn't find SHT31 sensor");
  //  while (1);
  //}
  if (! sht4.begin()) {
    Serial.println("Couldn't find SHT4x");
    while (1) delay(1);
  }
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);
  //Serial.println("System ready. Type 'read' to dump FRAM, 'reset' to clear pointer.");
}

void loop() {
unsigned long start = millis();
while (millis() - start < 20000) {  // Wait up to 15 seconds
  if (Serial.available()) {
    inputBuffer = Serial.readStringUntil('\n');
    inputBuffer.trim();

    if (inputBuffer == "read") {
      readFRAM();
      return;
    } else if (inputBuffer == "reset") {
      setAddressPointer(DATA_START);
      Serial.println("Pointer reset to DATA_START (0x0010).");
      return;
    }
  }
}

  // Take 5-second averaged readings
  const unsigned long sampleDuration = 5000;
  const unsigned long sampleInterval = 500;
  const int numSamples = sampleDuration / sampleInterval;

  float tempSum = 0, humSum = 0, parSum = 0, presSum = 0;
  int validSamples = 0;

  for (int i = 0; i < numSamples; i++) {
    //float t = sht31.readTemperature();
    //float h = sht31.readHumidity();
    sensors_event_t humidity, temp;
    sht4.getEvent(&humidity, &temp); // Populate temp and humidity objects

    float t = temp.temperature;   // in °C
    float h = humidity.relative_humidity; // in %
    float p = analogRead(A3);
    float pres = bmp.readPressure() / 100.0; // hPa
    
    if (!isnan(t) && !isnan(h)) {
      tempSum += t;
      humSum += h;
      parSum += p;
      presSum += pres;
      validSamples++;
    }
    delay(sampleInterval);
  }
  delay(1000);                    // Give TPL time to react



  float temp = validSamples > 0 ? tempSum / validSamples : NAN;
  float hum = validSamples > 0 ? humSum / validSamples : NAN;
  float par = validSamples > 0 ? parSum / validSamples : NAN;
  float pressure = validSamples > 0 ? presSum / validSamples : NAN;

  uint32_t timestamp = rtc.now().unixtime();

  if (!isnan(temp) && !isnan(hum) && !isnan(par) && !isnan(pressure)) {
    uint16_t addr = getAddressPointer();
    writeRecord(addr, timestamp, temp, hum, par, pressure);
    setAddressPointer(addr + RECORD_SIZE);

    Serial.print(addr); Serial.print(";");
    Serial.print(addr + RECORD_SIZE); Serial.print(";");
    Serial.print(timestamp); Serial.print(";");
    Serial.print(temp); Serial.print(";");
    Serial.print(hum); Serial.print(";");
    Serial.print(pressure); Serial.print(";");
    Serial.println(par);
  } else {
    Serial.println("Sensor read failed");
  }

  delay(1000);                    // Give TPL time to react
  digitalWrite(DONE_PIN, HIGH);  // Signal to TPL5110 that we're done
  delay(100);                    // Give TPL time to react
}

void writeRecord(uint16_t addr, uint32_t ts, float temp, float hum, float par, float pres) {
  union { float f; uint8_t b[4]; } tf, hf, pf, prf;
  tf.f = temp;
  hf.f = hum;
  pf.f = par;
  prf.f = pres;

  Wire.beginTransmission(FRAM_I2C_ADDR);
  Wire.write(addr >> 8);
  Wire.write(addr & 0xFF);
  Wire.write(ts & 0xFF);
  Wire.write((ts >> 8) & 0xFF);
  Wire.write((ts >> 16) & 0xFF);
  Wire.write((ts >> 24) & 0xFF);
  for (int i = 0; i < 4; i++) Wire.write(tf.b[i]);
  for (int i = 0; i < 4; i++) Wire.write(hf.b[i]);
  for (int i = 0; i < 4; i++) Wire.write(pf.b[i]);
  for (int i = 0; i < 4; i++) Wire.write(prf.b[i]);
  Wire.endTransmission();
}

void readFRAM() {
  //Serial.println("Dumping FRAM records (timestamp, temp °C, hum %, PAR, air pressure):");

  uint16_t endAddr = getAddressPointer();
  for (uint16_t addr = DATA_START; addr + RECORD_SIZE <= endAddr; addr += RECORD_SIZE) {
    Wire.beginTransmission(FRAM_I2C_ADDR);
    Wire.write(addr >> 8);
    Wire.write(addr & 0xFF);
    Wire.endTransmission();

    Wire.requestFrom(FRAM_I2C_ADDR, RECORD_SIZE);
    while (Wire.available() < RECORD_SIZE);

    uint32_t ts = Wire.read();
    ts |= (uint32_t)Wire.read() << 8;
    ts |= (uint32_t)Wire.read() << 16;
    ts |= (uint32_t)Wire.read() << 24;

  union { float f; uint8_t b[4]; } tf, hf, pf, prf;
   for (int i = 0; i < 4; i++) tf.b[i] = Wire.read();
   for (int i = 0; i < 4; i++) hf.b[i] = Wire.read();
   for (int i = 0; i < 4; i++) pf.b[i] = Wire.read();
   for (int i = 0; i < 4; i++) prf.b[i] = Wire.read();

  Serial.print(ts); Serial.print(";");
  Serial.print(tf.f); Serial.print(";");
  Serial.print(hf.f); Serial.print(";");
  Serial.print(pf.f); Serial.print(";");
  Serial.println(prf.f);
  }
}

uint16_t getAddressPointer() {
  Wire.beginTransmission(FRAM_I2C_ADDR);
  Wire.write(ADDRESS_STORE >> 8);
  Wire.write(ADDRESS_STORE & 0xFF);
  Wire.endTransmission();

  Wire.requestFrom(FRAM_I2C_ADDR, 2);
  while (Wire.available() < 2);
  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();

  uint16_t ptr = ((uint16_t)msb << 8) | lsb;
  return ptr < DATA_START ? DATA_START : ptr;
}

void setAddressPointer(uint16_t addr) {
  Wire.beginTransmission(FRAM_I2C_ADDR);
  Wire.write(ADDRESS_STORE >> 8);
  Wire.write(ADDRESS_STORE & 0xFF);
  Wire.write(addr >> 8);
  Wire.write(addr & 0xFF);
  Wire.endTransmission();
}
