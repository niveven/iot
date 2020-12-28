/************************************************* ************************************************** ******
* OPEN-SMART Red Serial MP3 Player Lesson 2: Play song with its file name
NOTE!!! First of all you should download the voice resources from our google drive:
https://drive.google.com/drive/folders/0B6uNNXJ2z4CxaXFpakMxR0p1Unc?usp=sharing

Then unzip it and find the 01 and 02 folder and put them into your TF card (should not larger than 32GB). 

* You can learn how to play a song with folder name and file name.

*
* The following functions are available:
*
/--------basic operations---------------/
mp3.play();
mp3.pause();
mp3.nextSong();
mp3.previousSong();
mp3.volumeUp();
mp3.volumeDown();
mp3.forward();    //fast forward
mp3.rewind();     //fast rewind
mp3.stopPlay();  
mp3.stopInject(); //when you inject a song, this operation can stop it and come back to the song befor you inject
mp3.singleCycle();//it can be set to cycle play the currently playing song 
mp3.allCycle();   //to cycle play all the songs in the TF card
/--------------------------------/

mp3.playWithIndex(int8_t index);//play the song according to the physical index of song in the TF card

mp3.injectWithIndex(int8_t index);//inject a song according to the physical index of song in the TF card when it is playing song.

mp3.setVolume(int8_t vol);//vol is 0~0x1e, 30 adjustable level

mp3.playWithFileName(int8_t directory, int8_t file);//play a song according to the folder name and prefix of its file name
                                                            //foler name must be 01 02 03...09 10...99
                                                            //prefix of file name must be 001...009 010...099

mp3.playWithVolume(int8_t index, int8_t volume);//play the song according to the physical index of song in the TF card and the volume set

mp3.cyclePlay(int16_t index);//single cycle play a song according to the physical index of song in the TF

mp3.playCombine(int16_t folderAndIndex[], int8_t number);//play combination of the songs with its folder name and physical index
      //folderAndIndex: high 8bit is folder name(01 02 ...09 10 11...99) , low 8bit is index of the song
      //number is how many songs you want to play combination

 
About SoftwareSerial library:
The library has the following known limitations:
If using multiple software serial ports, only one can receive data at a time.

Not all pins on the Mega and Mega 2560 support change interrupts, so only the following can be used for RX: 
10, 11, 12, 13, 14, 15, 50, 51, 52, 53, A8 (62), A9 (63), A10 (64), A11 (65), A12 (66), A13 (67), A14 (68), A15 (69).

Not all pins on the Leonardo and Micro support change interrupts, so only the following can be used for RX: 
8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI).
On Arduino or Genuino 101 the current maximum RX speed is 57600bps.
On Arduino or Genuino 101 RX doesn't work on Pin 13.

Store: dx.com/440175
https://open-smart.aliexpress.com/store/1199788

************************************************** **************************************************/
#include <SoftwareSerial.h>
#include "RedMP3.h"

#define MP3_RX 4//RX of Serial MP3 module connect to D7 of Arduino
#define MP3_TX 5//TX to D8, note that D8 can not be used as RX on Mega2560, you should modify this if you donot use Arduino UNO
MP3 mp3(MP3_RX, MP3_TX);

int8_t volume = 0x1a;//0~0x1e (30 adjustable level)
int8_t folderName = 0x01;//folder name must be 01 02 03 04 ...
int8_t fileName = 0x01; // prefix of file name must be 001xxx 002xxx 003xxx 004xxx ...
bool flag = false;

int DATA_PIN = A0; 
int CLOCK_PIN = A1;

void transmitState(int* transmit){
    analogWrite(CLOCK_PIN, 0);
    delay(1000);
    for (int i=0; i<4; i++){
      analogWrite(DATA_PIN, (transmit[i])*600);
       delay(5);
      analogWrite(CLOCK_PIN, 600);
      delay(100);
      analogWrite(CLOCK_PIN, 0);
      Serial.println((transmit[i])*600);
      delay(500);
    }
    return;
}

int PLAY_MP3_SIGNAL = 2;
int MAGNETIC_PIN_INPUT_COFFEE = 8;
int MAGNETIC_PIN_INPUT_TEA = 9;
int FLYING_FISH_INPUT_OUTER_BOX = 10;

void setup()
{
  pinMode(PLAY_MP3_SIGNAL,INPUT);
  pinMode(MAGNETIC_PIN_INPUT_COFFEE,INPUT);
  pinMode(MAGNETIC_PIN_INPUT_TEA,INPUT);
  pinMode(FLYING_FISH_INPUT_OUTER_BOX,INPUT);
  Serial.begin(9600);

}

void loop()
{
  if (digitalRead(PLAY_MP3_SIGNAL)==HIGH){
    flag=true;
  }
  if (flag){
    delay(500);//Requires 500ms to wait for the MP3 module to initialize  
    mp3.setVolume(volume);
    delay(50);//you should wait for >=50ms between two commands
    mp3.playWithFileName(folderName,fileName);
    delay(8000);
    flag=false;    
  }

  int state_COFFEE = digitalRead(MAGNETIC_PIN_INPUT_COFFEE);
  int state_TEA = digitalRead(MAGNETIC_PIN_INPUT_TEA);
  int state_OUTER_BOX = digitalRead(FLYING_FISH_INPUT_OUTER_BOX);
  
  int transmit[5] = {0};
  transmit[0] = state_OUTER_BOX;
  transmit[1] = state_TEA;
  transmit[2] = state_COFFEE;
  transmit[3] = 0;
  transmit[4] = 0; //END of string (null)
  for (int i=0 ; i<5 ; i++){
  Serial.print(transmit[i]);
  }
  Serial.println(' ');
  transmitState(transmit);
}

/*********************************************************************************************************
The end of file
*********************************************************************************************************/
