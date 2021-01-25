#include <SoftwareSerial.h>
#include "RedMP3.h"


/////////////////////////////////////////////////////
//Variables and definitions for playing MP3 files:
/////////////////////////////////////////////////////

#define MP3_RX 4//RX of Serial MP3 module connect to D7 of Arduino
#define MP3_TX 5//TX to D8, note that D8 can not be used as RX on Mega2560, you should modify this if you donot use Arduino UNO
MP3 mp3(MP3_RX, MP3_TX);

int8_t volume = 0x1a;//0~0x1e (30 adjustable level)
int8_t folderName = 0x01;//folder name must be 01 02 03 04 ...
int8_t fileName = 0; // prefix of file name must be 001xxx 002xxx 003xxx 004xxx ...
//bool flag = false;

int ESP_code[5] = {0};
char PLAY_GREETING = 0;
char PLAY_FACE_OK = 0;
char PLAY_FACE_FAIL = 0;
char PLAY_GOODBYE = 0;

//Not in use anymore;
//int PLAY_MP3_SIGNAL = 2;

////////////////////////////////////////////////////
// Pins Used in the Arduino:
////////////////////////////////////////////////////

//comm *FROM* Arduino to ESP:
int DATA_PIN = A0; 
int CLOCK_PIN = A1;

//comm *FROM* ESP to Arduino: 
int CLOCK_MP3_PIN = A4;
int ESP_PIN = A5;

// Sensor pins:
int MAGNETIC_PIN_INPUT_COFFEE = 8;
int MAGNETIC_PIN_INPUT_TEA = 9;
int FLYING_FISH_INPUT_OUTER_BOX = 10;


//////////////////////////////////////////////////
//////////////////////////////////////////////////


// Function transmits to ESP the state of the sensors according to the "transmit array":
void transmitState(int* transmit){
    analogWrite(CLOCK_PIN, 0);
    while(analogRead(CLOCK_MP3_PIN)> 450){}
    for (int i=0; i<4; i++){
      analogWrite(DATA_PIN, (transmit[i])*600);
      delay(25);
      analogWrite(CLOCK_PIN, 600);
      delay(100);
      analogWrite(CLOCK_PIN, 0);
      Serial.println((transmit[i])*600);
      delay(300);
    }
    analogWrite(CLOCK_PIN, 600);
    delay(25);
    return;
}


//////////////////////////////////////////////////
//////////////////////////////////////////////////

void setup()
{
  //pinMode(PLAY_MP3_SIGNAL,INPUT);
  pinMode(MAGNETIC_PIN_INPUT_COFFEE,INPUT);
  pinMode(MAGNETIC_PIN_INPUT_TEA,INPUT);
  pinMode(FLYING_FISH_INPUT_OUTER_BOX,INPUT);
    
  pinMode(CLOCK_MP3_PIN,INPUT);
  pinMode(ESP_PIN,INPUT);
  pinMode(DATA_PIN,OUTPUT);
  pinMode(CLOCK_PIN,OUTPUT);
   
  analogWrite(CLOCK_PIN,600);
  
  
  Serial.begin(9600);

}


/////////////////////////////////////////////////


void loop()
{
// Getting information from ESP about what sound to play: 
  //Serial.print("CLOCK - ");
  //Serial.println(analogRead(CLOCK_MP3_PIN));
  if (analogRead(CLOCK_MP3_PIN) < 500){
    //Serial.print("Sound Code: ");
    for (int i=0; i<4 ; i++){
      if(analogRead(CLOCK_MP3_PIN) > 450){
        //Serial.print(i);
        //Serial.print(" ");
        //Serial.print(analogRead(ESP_PIN));
        //Serial.print(" ");
        ESP_code[i] = analogRead(ESP_PIN);
        delay(200);
      }
      else{
        i--;
      }
    }

// Playing the sound if needed:
    
  //Serial.println(" ");
  }
  if (ESP_code[0] > 400){
    fileName = 0x01;
    Serial.println("playing Greeting");
  }
  if (ESP_code[1] > 400 ){
    fileName = 0x02;
    Serial.println("playing Face OK");
  }
  if (ESP_code[2]> 400){
    fileName = 0x03;
    Serial.println("playing Face Fail");
  }
  if (ESP_code[3]> 400){
    fileName = 0x04;
    Serial.println("playing Goodbye");
  }
  if (fileName){
    delay(500);//Requires 500ms to wait for the MP3 module to initialize  
    mp3.setVolume(volume);
    delay(50);//you should wait for >=50ms between two commands
    mp3.playWithFileName(folderName,fileName);
    delay(8000);
    fileName = 0;  
    ESP_code[0] = 0;  
    ESP_code[1] = 0;
    ESP_code[2] = 0;
    ESP_code[3] = 0;
  }

// Getting info about the physical state of the sensors:
  int state_COFFEE = digitalRead(MAGNETIC_PIN_INPUT_COFFEE);
  int state_TEA = digitalRead(MAGNETIC_PIN_INPUT_TEA);
  int state_OUTER_BOX = digitalRead(FLYING_FISH_INPUT_OUTER_BOX);

// Transmitting the sensors' state to the ESP:
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
