 /*
****************************************************************************************
****************************************************************************************
20231121
Files Required to make a complete program - 
 CLI_V1.0, Quest_CLI.h, Quest_Flight.h, Quest_flight.cpp
 Important Functions and their use:
 cmd_takeSphot
 This function will use the serial camera (the one with the cable) to take a photo (jpg)
 on the transfer of this photo to the Host controller it will create a text file (txt)
 with the same file name as the jpg file of the conditions of when the jpg was taken.
 NOTE: add2text can be used with this command
 cmd_takeSpiphoto
 This function is for the SPI camera (the one that plugges directly on the microlab board)
 it will take a jpg the same as the serial camera, also creating a txt file. Also NOTE: the
 add2text function can be used on the photo text file.
 nophotophoto
 if a camera is not being used, then this will simulate the camera photo operation. The text file 
 is still created, so the add2text function can be used the get data downloaded in the text file
 nophoto30K
 this dois not use a camera, instead it uses the file space for the photo the containe ascii data
 using dataappend function defined below. The data will still have a jpg etension, but the file
 will contain ascii of the data put in buy the dataappend unction. Plus a text file is generated,
 and the add2text function will add more data.
 -----
 add2text(int value1,int value2,int value3)
 this function will take the three varables and place them at the end of the text file (the file is
 limited the 1024 charators), if you look at the function the text output can be formatted is almost
 any way. Look at it and change it to acccomadate your data...
 dataappend(int counts,int ampli,int SiPM,int Deadtime)
 this function will add ascii data to the 30K byte data buffer that was used a a jpg photo. look at
 the data formating and change it necessary for you project information. To view the data, you can
 use a hex/ascii file viewer or change the ext to a txt. then it should be viewable with a text exitor.
.
****************************************************************************************** 
****************************************************************************************** 
*/

#include <Arduino.h>
#include "TCA9548A.h"
#include "Adafruit_AS7341.h"

Adafruit_AS7341 as7341;
TCA9548A I2CMux;  // Multiplexer instance

//////////////////////////////////////////////////////////////////////////
// This defines the timers used to control flight operations
//////////////////////////////////////////////////////////////////////////
// Fast clock --- 1 hour = 5 min = 1/12 of an hour
// one millie -- 1ms
//
#define SpeedFactor 20 // = times faster
//
//
//////////////////////////////////////////////////////////////////////////
//
#define one_sec 1000 //one second = 1000 millis
#define one_min 60*one_sec // one minute of time
#define one_hour 60*one_min // one hour of time
#define one_day 24*one_hour
//
//
#define TimeEvent1_time ((one_min * 60) / SpeedFactor) //take a photo time
#define Pump_time ((one_day * 5) / SpeedFactor)
#define ColorSensorTime ((one_min) / SpeedFactor) 
#define Pump_duration ((one_sec * 5) / SpeedFactor)

const int pump_constant = 5;

//
 int sensor1count = 0; //counter of times the sensor has been accessed
 int sensor2count = 0; //counter of times the sensor has been accessed
 int sensorCount = 0;
 int State = 0; //FOR TESTING ONLY WILL SWITCH FROM SPI CAMERA TO SERIAL CAMERA EVERY HOUR
//
///////////////////////////////////////////////////////////////////////////
/**
 @brief Flying function is used to capture all logic of an experiment during flight.
*/
//************************************************************************
// Beginning of the flight program setup
//
////


//Justin Shi
//for testing purposes only, print all readings
int pumpcounter=0;
bool runpumps=false;
void Flying() {
  Serial.begin(115200);
  Serial.println("Initializing AS7341 Sensor...");

  I2CMux.begin(Wire);  // Initialize the I2C multiplexer
  I2CMux.closeAll();   // Close all channels first
  I2CMux.openChannel(1);  // Open channel 1 for AS7341

  while (!Serial) {
    delay(1);
  }
  
  if (!as7341.begin()){
    Serial.println("Could not find AS7341");
    while (1) { delay(10); }
  }
  
  as7341.setATIME(100);
  as7341.setASTEP(999);
  as7341.setGain(AS7341_GAIN_256X);

  Serial.println("\n\rRun flight program\n\r");
  //
  uint32_t TimeEvent1 = millis(); //set TimeEvent1 to effective 0
  uint32_t pump = millis(); //set TimeEvent1 to effective 0
  uint32_t ColorSensorTimer = millis(); 
  uint32_t Sensor1Timer = millis(); 
  uint32_t Sensor2Timer = millis(); //clear sensor1Timer to effective 0
  uint32_t Sensor2Deadmillis = millis(); //clear mills for difference
  uint32_t PumpTimer = millis();
  uint32_t IndividualPumps = millis();
  //
  uint32_t one_secTimer = millis(); //set happens every second
  uint32_t sec60Timer = millis(); //set minute timer

  //*****************************************************************
  // Here to set up flight conditions i/o pins, atod, and other special condition
  // of your program
  //
  //
  //
  //******************************************************************

  //------------ flying -----------------------

  Serial.println("Flying NOW - x=abort"); //terminal output for abort back to test
  Serial.println("Terminal must be reset after abort"); //terminal reset requirement upon soft reset

  missionMillis = millis(); //Set mission clock millis, you just entered flight conditions
  //
  //
  /////////////////////////////////////////////////////
  //----- Here to start a flight from a reset ---------
  /////////////////////////////////////////////////////
  //
  DateTime now = rtc.now(); //get time now
  currentunix = (now.unixtime()); //get current unix time, don't count time not flying
  writelongfram(currentunix, PreviousUnix); //set fram Mission time start now counting seconds unix time
  //
  //***********************************************************************
  //***********************************************************************
  // All Flight conditions are now set up, NOW to enter flight operations
  //
  //***********************************************************************
  //***********************************************************************

  IndividualPumps = millis();
  while (1) {
  //
  //----------- Test for terminal abort command (x) from flying ----------------------
  //
  while (Serial.available()) { //Abort flight program progress
    byte x = Serial.read(); //get the input byte
    if (x == 'x') { //check the byte for an abort x
      return ; //return back to poeration sellection
    } //end check
  } //end abort check
  //------------------------------------------------------------------
  //
  //*********** Timed Event 1 test ***************************************
  //
  // this test if TimeEvent1 time has come
  // See above for TimeEvent1_time settings between this event
  // violet blue green yellow orange red
  //
  
  if((millis() - PumpTimer) > Pump_time) {
    PumpTimer = millis();
    runpumps=true;
  }
  if(((millis() - IndividualPumps) > Pump_duration) && runpumps) {
    IndividualPumps = millis();
      if(pumpcounter % 4 == 0) {
        digitalWrite(IO3, HIGH);
        pumpcounter++;
      } else if(pumpcounter % 4 == 1) {
        digitalWrite(IO3, LOW);
        digitalWrite(IO4, HIGH);
        pumpcounter++;
      } else if(pumpcounter % 4 == 2) {
        digitalWrite(IO4, LOW);
        digitalWrite(IO5, HIGH);
        pumpcounter++;
      }  else {
        digitalWrite(IO5, LOW);
        digitalWrite(IO7, HIGH); // turn da vibrational motors on
        digitalWrite(IO7, LOW);
        runpumps = false;
        pumpcounter++;
      }
    }

  if((millis() - ColorSensorTimer) > ColorSensorTime){
    ColorSensorTimer = millis();
    Serial.println("turning LED on");
    digitalWrite(IO2, HIGH);

    uint16_t readings[12];
    float counts[12];

    if (!as7341.readAllChannels(readings)){
      Serial.println("Error reading all channels!");
      return;
    }

    for(uint8_t i = 0; i < 12; i++) {
      if(i == 4 || i == 5) continue;
      // we skip the first set of duplicate clear/NIR readings
      // (indices 4 and 5)
      counts[i] = as7341.toBasicCounts(readings[i]);
    }

    Serial.print("F1 415nm : ");
    Serial.println(counts[0]);
    Serial.print("F2 445nm : ");
    Serial.println(counts[1]);
    Serial.print("F3 480nm : ");
    Serial.println(counts[2]);
    Serial.print("F4 515nm : ");
    Serial.println(counts[3]);
    Serial.print("F5 555nm : ");
    // again, we skip the duplicates  
    Serial.println(counts[6]);
    Serial.print("F6 590nm : ");
    Serial.println(counts[7]);
    Serial.print("F7 630nm : ");
    Serial.println(counts[8]);
    Serial.print("F8 680nm : ");
    Serial.println(counts[9]);
    Serial.print("Clear    : ");
    Serial.println(counts[10]);
    Serial.print("NIR      : ");
    Serial.println(counts[11]);

    Serial.println();
    
    delay(500);
    Serial.println("turning LED off");
    digitalWrite(IO2, LOW);
  }

  //end of TimeEvent1_time
  //------------------------------------------------------------------
  //
  //*******************************************************************************
  //*********** One second counter timer will trigger every second ****************
  //*******************************************************************************
  // Here one sec timer - every second
  //
  if ((millis() - one_secTimer) > one_sec) { //one sec counter
    one_secTimer = millis(); //reset one second timer
    DotStarYellow(); //turn on Yellow DotStar to Blink for running
    //
    //****************** NO_NO_NO_NO_NO_NO_NO_NO_NO_NO_NO_ *************************
    // DO NOT TOUCH THIS CODE IT IS NECESARY FOR PROPER MISSION CLOCK OPERATIONS
    // Mission clock timer
    // FRAM keep track of cunlitive power on time
    // and RTC with unix seconds
    //------------------------------------------------------------------------------
    DateTime now = rtc.now(); //get the time time,don't know how long away
    currentunix = (now.unixtime()); //get current unix time
    Serial.print(currentunix); Serial.print(" "); //testing print unix clock
    uint32_t framdeltaunix = (currentunix - readlongFromfram(PreviousUnix)); //get delta sec of unix time
    uint32_t cumunix = readlongFromfram(CumUnix); //Get cumulative unix mission clock
    writelongfram((cumunix + framdeltaunix), CumUnix); //add and Save cumulative unix time Mission
    writelongfram(currentunix, PreviousUnix); //reset PreviousUnix to current for next time
    //
    //********* END_NO_END_NO_END_NO_END_NO_END_NO_END_NO_ **************************
    //
    // This part prints out every second
    //
    Serial.print(": Mission Clock = "); //testing print mission clock
    Serial.print(readlongFromfram(CumUnix)); //mission clock
    Serial.print(" is "); //spacer
    //
    //------Output to the terminal days hours min sec
    //
    getmissionclk();
    Serial.print(xd); Serial.print(" Days ");
    Serial.print(xh); Serial.print(" Hours ");
    Serial.print(xm); Serial.print(" Min ");
    Serial.print(xs); Serial.println(" Sec");
    //
    //
    DotStarOff();
  } // end of one second routine
  //
  //**********************************************************************
  //*********** Read Sensor1 Event read and add to text buffer************
  //**********************************************************************
  //
  // End of Sensor1 time event
  //
  //**********************************************************************
  //*********** Read Sensor2 Event read and add to text buffer************
  //*********** Test of filling the 30K data buffer for lots of data *****
  //********************************************************************** 
  // If it is event driven then remove the Sensor2Timer evvvvvend 
  // here to get the data for the event 
  //
  
  } // End of while 
} //End nof Flighting
//
//
//FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
// This is a function to adds three values to the user_text_buffer
// Written specificy for 2023-2024 Team F, Team B,
// Enter the function with "add2text(1st interger value, 2nd intergre value, 3rd intergervalue);
// the " - value1 " text can be changed to lable the value or removed to same space
// ", value2 " and ", value 3 " masy also be removed or changed to a lable.
// Space availiable is 1024 bytes, Note- - each Data line has a ncarrage return and a line feed
//
//example of calling routine:
// //
// int value1 = 55;
// int value2 = 55000;
// int value3 = 14;
// add2text(value1, value2, value3);
// //
//EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE 
//
void add2text(int value1,int value2,int value3, int value4, int value5, int value6){ //Add value to text file
  if (strlen(user_text_buf0) >= (sizeof(user_text_buf0)-100)){ //Check for full
    Serial.println("text buffer full"); //yes, say so
    return; //back to calling
  }
  // Serial.print("test");
  char temp[11]; // Maximum number of digits for a 32-bit integer 
  int index = 10; //Start from the end of the temperary buffer 
  char str[12]; //digits + null terminator 
  //--------- get time and convert to str for entry into text buffer ----
  // Serial.print("test2"); 
  DateTime now = rtc.now(); //get time of entry
  uint32_t value = now.unixtime(); //get unix time from entry
  do {
    delay(0);
    // Serial.print("a");
    
    temp[index--] = '0' + (value % 10); // Convert the least significant digit to ASCII
    // Serial.print(value);

    value /= 10; // Move to the next digit
  } while (value != 0);
  strcpy(str, temp + index +1); 
  // Serial.print("test3"); // Copy the result to the output string
  //---------- end of time conversion uni time is now in str ------------- 
  strcat(user_text_buf0, (str)); //write unix time
  //
  // unit time finish entry into this data line
  //
  strcat(user_text_buf0, (" - R= ")); // seperator
  strcat(user_text_buf0, (itoa(value1, ascii, 10)));
  strcat(user_text_buf0, (", S= "));
  strcat(user_text_buf0, (itoa(value2, ascii, 10)));
  strcat(user_text_buf0, (", T= "));
  strcat(user_text_buf0, (itoa(value3, ascii, 10)));
  strcat(user_text_buf0, (", U= "));
  strcat(user_text_buf0, (itoa(value4, ascii, 10)));
  strcat(user_text_buf0, (", V= "));
  strcat(user_text_buf0, (itoa(value5, ascii, 10)));
  strcat(user_text_buf0, (", W= "));
  strcat(user_text_buf0, (itoa(value6, ascii, 10)));
  strcat(user_text_buf0, ("\r\n"));

  //Serial.println(strlen(user_text_buf0)); //for testing
}
//------end of Function to add to user text buffer ------ 
//
//=============================================================================
//
////FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
// Function to write into a 30K databuffer
// char databuffer[30000]; // Create a character buffer with a size of 2KB
// int databufferLength = 0; // Initialize the buffer length
// Append data to the large data buffer buffer always enter unit time of data added
// enter: void dataappend(int counts, int ampli, int SiPM, int Deadtime) (4 values)
//
void dataappend(int counts, int vals1[], int vals2[], int Deadtime) { //entry, add line with values to databuffer, garenteed to be of size 6 
  //----- get and set time to entry -----
  const char colors[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2'};
  DateTime now = rtc.now(); //get time of entry
  String stringValue = String(now.unixtime()); //convert unix time to string
  //----- add formated string to buffer -----
  String results = " - " + String(counts) + String (Deadtime) + " \n"; //format databuffer entry
  for(int i = 0; i < 12; i++){
    results = results + " " + colors[i] + "[sensor1: " + vals1[i] + ", sensor2: " + vals2[i] + "] \n";
  }
  results += "\r\n";
  const char* charValue1 = results.c_str(); //convert to a C string value
  appendToBuffer(charValue1); //Send formated string to databuff
  //
  // Serial.println(databufferLength); //print buffer length for testing only
}
/*
void dataappend(int counts,int ampli,int SiPM,int Deadtime) { //entry, add line with values to databuffer
 //----- get and set time to entry -----
 DateTime now = rtc.now(); //get time of entry
 String stringValue = String(now.unixtime()); //convert unix time to string
 const char* charValue = stringValue.c_str(); //convert to a C string value
 appendToBuffer(charValue); //Sent unix time to databuffer
 //----- add formated string to buffer -----
 String results = " - " + String(counts) + " " + String(ampli) + " " + String(SiPM) + " " + String (Deadtime) + "\r\n"; //format databuffer entry
 const char* charValue1 = results.c_str(); //convert to a C string value
 appendToBuffer(charValue1); //Send formated string to databuff
 //
 // Serial.println(databufferLength); //print buffer length for testing only
}
*/
//----------------------- //end dataappend
//----- sub part od dataappend -- append to Buffer -----
//-----------------------
void appendToBuffer(const char* data) { //enter with charator string to append
  int dataLength = strlen(data); //define the length of data to append
  // ----- Check if there is enough space in the buffer //enough space?
  if (databufferLength + dataLength < sizeof(databuffer)) { //enouth space left in buffer
    // ----- Append the data to the buffer
    strcat(databuffer, data); //yes enough space, add data to end of buffer
    databufferLength += dataLength; //change to length of the buffer
  } else {
    Serial.println("Buffer is full. Data not appended."); //Not enough space, say so on terminal
  } //end not enough space
} //end appendToBuffer
// Justin Shi - this is meant for testing purposes, retard not working
// void sensorPrint(AS726X& obj, String name){
// obj.takeMeasurements();
// Serial.print(name + "readings: ");
// Serial.print(" Reading: R[");
// Serial.print(obj.getCalibratedR(), 2);
// Serial.print("] S[");
// Serial.print(obj.getCalibratedS(), 2);
// Serial.print("] T[");
// Serial.print(obj.getCalibratedT(), 2);
// Serial.print("] U[");
// Serial.print(obj.getCalibratedU(), 2);
// Serial.print("] V[");
// Serial.print(obj.getCalibratedV(), 2);
// Serial.print("] W[");
// Serial.print(obj.getCalibratedW(), 2); 
// Serial.println();
// }
//=================================================================================================================
//
