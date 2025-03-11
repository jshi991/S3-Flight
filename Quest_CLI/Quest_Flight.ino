#include "Quest_Flight.h"
#include "Quest_CLI.h"
#include "Quest_fram.h"         
#include "TCA9548A.h"
#include "Adafruit_AS7341.h"

Adafruit_AS7341 as7341;
TCA9548A I2CMux;  

//////////////////////////////////////////////////////////////////////////
// Timers used to control flight operations
//////////////////////////////////////////////////////////////////////////
#define PUMP_FLAG_ADDRESS FRAM_ADDRESS
#define PUMP_FLAG_ADRESS_SIZE 8 
#define SpeedFactor 30    

#define one_sec   1000                    
#define one_min   60 * one_sec             
#define one_hour  60 * one_min             

#define ON_TRUE ((uint8_t) 1)

#define WriteToBuffer       ((one_min * 15) / SpeedFactor)
#define NoPhoto             (one_hour / SpeedFactor)
#define Pump                ((one_hour * 15) / SpeedFactor)
#define PUMP_DURATION       (one_second * 60) //bogus value please test
int sensor1count = 0;  
int sensor2count = 0;  
int State = 0;  

void Flying() {
  Serial.println("\n\rRun flight program\n\r");

  Serial.begin(115200);  // Fixed incorrect baud rate
  Wire.begin();

  Serial.println("AS7341 initialization sequence");

  I2CMux.begin(Wire);
  I2CMux.closeAll();

  I2CMux.openChannel(1);
  if (!as7341.begin()) {
    Serial.println("Could not find AS7341 object on channel 1, aborting.");
  }

  I2CMux.closeAll();
  delay(200);
  I2CMux.openChannel(0);

  if (!as7341.begin()) {
    Serial.println("Could not find AS7341 object on channel 0, aborting.");
  }

  delay(200);
  I2CMux.closeAll();

  as7341.setAtime(29);  // Integration time parameter
  as7341.setAstep(599); // Another integration time parameter
  as7341.setAGAIN(7);   // Gain setting (0~10 corresponds to X0.5, X1, X2, ... X512)
  
  uint32_t WriteToBuffer_Time = millis();
  uint32_t NoPhoto_Time = millis();
  uint32_t Pump_Time = millis();
  uint32_t one_secTimer = millis();

  Serial.println("Flying NOW  -  x=abort");
  Serial.println("Terminal must be reset after abort");

  while (1) {
    while (Serial.available()) {     
      byte x = Serial.read();
      if (x == 'x') { 
        return;                     
      }                              
    }                                

    if ((millis() - Pump_Time) > Pump) {
      if (readbyteFromfram(PUMP_FLAG_ADDRESS) != ON_TRUE) {
        writebytefram(ON_TRUE, PUMP_FLAG_ADDRESS);
        Serial.write(IO6, HIGH);  //TODO PLEASE FIX
        Serial.write(IO5, HIGH);
        delay(PUMP_DURATION);
        Serial.write(IO6, LOW);  //TODO PLEASE FIX
        Serial.write(IO5, LOW);
      } else {
        Serial.println("Pumps have already been activated.");
      }
    }

    if ((millis() - NoPhoto_Time) > NoPhoto) {
      nophoto30K();
    }

    if ((millis() - WriteToBuffer_Time) > WriteToBuffer) {
      WriteToBuffer_Time = millis();  

      Serial.println("Turning on LEDs");
      digitalWrite(IO2, HIGH);
      delay(3000);
      uint16_t readingsChannel0[12] = {0};
      float countsChannel0[12] = {0};

      uint16_t readingsChannel1[12] = {0};
      float countsChannel1[12] = {0};

      I2CMux.openChannel(0);

      if (!as7341.readAllChannels(readingsChannel0)) {
        Serial.println("Error reading on channel 0");
      } else {
        for (uint8_t i = 0; i < 12; i++) {
          if (i == 4 || i == 5) continue;
          countsChannel0[i] = as7341.toBasicCounts(readingsChannel0[i]);
        }
      }

      delay(200);
      I2CMux.closeAll();
      delay(200);
      I2CMux.openChannel(1);
      delay(200);

      if (!as7341.readAllChannels(readingsChannel1)) {
        Serial.println("Error reading channel 1");
      } else {
        for (uint8_t i = 0; i < 12; i++) {
          if (i == 4 || i == 5) continue;
          countsChannel1[i] = as7341.toBasicCounts(readingsChannel1[i]);
        }
      }

      delay(200);
      I2CMux.closeAll();
      digitalWrite(IO2, LOW);
      dataappend(1, countsChannel0, countsChannel1, 1);
    }
  }
}

void dataappend(int counts, float vals1[], float vals2[], int Deadtime) {
  const char colors[] = {'1', '2', '3', '4', '5', '6', '7', '8'};
  DateTime now = rtc.now();
    char dateString[20]; 
    sprintf(dateString, "%04d-%02d-%02d %02d:%02d:%02d", 
            now.year(), now.month(), now.day(), 
            now.hour(), now.minute(), now.second());

  String results = " - new entry " + String(dateString) + String(counts) + " " + String(Deadtime) + "\n";

  for (int i = 0; i < 8; i++) {
    results += String(colors[i]) + "[sensor1: " + String(vals1[i], 2) + 
               ", sensor2: " + String(vals2[i], 2) + "]\n";
  }
  Serial.println("results" + results);
  results += "\r\n";
  appendToBuffer(results.c_str());
}

void appendToBuffer(const char* data) {
  int dataLength = strlen(data);
  
  if (databufferLength + dataLength < sizeof(databuffer)) {  
    strcat(databuffer, data);
    databufferLength += dataLength;
  } else {
    Serial.println("Buffer is full. Data not appended.");
  }
}
