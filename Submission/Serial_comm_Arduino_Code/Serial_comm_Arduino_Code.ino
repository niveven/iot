#include <SoftwareSerial.h>
#include "RedMP3.h"

/////////////////////////////////////////////////////
//Variables and definitions for playing MP3 files:
/////////////////////////////////////////////////////

#define MP3_RX 4 //RX of Serial MP3 module connect to D7 of Arduino
#define MP3_TX 5 //TX to D8, note that D8 can not be used as RX on Mega2560, you should modify this if you donot use Arduino UNO
MP3 mp3(MP3_RX, MP3_TX);

int8_t volume = 0x1a;     //0~0x1e (30 adjustable level)
int8_t folderName = 0x01; //folder name must be 01 02 03 04 ...
int8_t fileName = 0;      // prefix of file name must be 001xxx 002xxx 003xxx 004xxx ...

////////////////////////////////////////////////////
// Pins Used in the Arduino:
////////////////////////////////////////////////////

// Sensor pins:
int MAGNETIC_PIN_INPUT_COFFEE = 8;
int MAGNETIC_PIN_INPUT_TEA = 9;
int FLYING_FISH_INPUT_OUTER_BOX = 10;

//////////////////////////////////////////////////
//////////////////////////////////////////////////

// software serial #1: RX = 7(only for syntax), TX = digital 6
SoftwareSerial portDebug(7, 6);

// input as string *from* ESP:
String string;

// output *to* ESP:
int transmit = 0;
int old_transmit = -1;

//////////////////////////////////////////////////
//////////////////////////////////////////////////

void setup()
{
  pinMode(MAGNETIC_PIN_INPUT_COFFEE, INPUT);
  pinMode(MAGNETIC_PIN_INPUT_TEA, INPUT);
  pinMode(FLYING_FISH_INPUT_OUTER_BOX, INPUT);


  Serial.begin(9600);
  portDebug.begin(9600);

}

/////////////////////////////////////////////////

void loop()
{
  // Getting info about the physical state of the sensors:
  int state_COFFEE = digitalRead(MAGNETIC_PIN_INPUT_COFFEE);
  int state_TEA = digitalRead(MAGNETIC_PIN_INPUT_TEA);
  int state_OUTER_BOX = digitalRead(FLYING_FISH_INPUT_OUTER_BOX);
  
  transmit = state_OUTER_BOX * 100 + state_COFFEE * 10 + state_TEA;
  if (transmit != old_transmit) {
    portDebug.print("Sending to ESP, print: ");
    portDebug.print(transmit);
    portDebug.print(" , write: ");
    portDebug.write(transmit);
    portDebug.println();
    Serial.print(transmit);
    delay(1000);
  }
  old_transmit = transmit;
  
  //Accepting data about sound to play from the ESP:
 //portDebug.println("checking input");
  if (Serial.available() > 0) {
    string = Serial.readString();
    portDebug.println();
    portDebug.println();
    portDebug.print("got input from ESP,  print: ");
    portDebug.print(string);
    portDebug.println();
    // Changing sound-filename according to the recieved code:
    if (string == "10"){
        portDebug.println("got 10 - Playing Greeting");
        fileName = 0x01;
        }
    else if (string == "11"){
        portDebug.println("got 11 - Playing Face OK");
        fileName = 0x02;
    }
    else if (string == "12"){
        portDebug.println("got 12 - Playing Face Fail");
        fileName = 0x03;
    }
    else if (string == "13"){
        portDebug.println("got 13 - Playing Goodbye");
        fileName = 0x04;
    }
        else if (string == "14"){
        portDebug.println("got 14 - Playing No Charge");
        fileName = 0x05;
    }
    
      // Debug prints:
    
    else if (string == "1"){
        portDebug.println("Case 0 in ESP");
        fileName = 0;
    }
    else if (string == "2"){
        portDebug.println("Case 1 in ESP");
        fileName = 0;
    }
    else if (string == "3"){
        portDebug.println("Case 2 in ESP");
        fileName = 0;
    }
    else if (string == "4"){
        portDebug.println("Case 3 in ESP");
        fileName = 0;
    }
    else if (string == "5"){
        portDebug.println("Case 4 in ESP");
        fileName = 0;
    }
    else if (string == "6"){
        portDebug.println("Case 5 in ESP");
        fileName = 0;
    }
    else if (string == "7"){
        portDebug.println("Case 6 in ESP");
        fileName = 0;
    }
    
    // Playing Sound:
    if (fileName)
    {
      delay(500); //Requires 500ms to wait for the MP3 module to initialize
      mp3.setVolume(volume);
      delay(50); //you should wait for >=50ms between two commands
      mp3.playWithFileName(folderName, fileName);
      //delay(2000);
      fileName = 0;
    }
  }
  delay(1000);
}



/*********************************************************************************************************
  The end of file
*********************************************************************************************************/
