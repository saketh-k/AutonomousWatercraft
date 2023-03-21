#include <HardwareSerial.h>
#include <SoftwareSerial.h>
#include <Servo.h>
#include <DFRobotDFPlayerMini.h>
#include <Pixy2.h>
#include <Pixy2CCC.h>

//Debug mode
#define DEBUG_ARDUINO true

//pin definitions
#define pumpPin 3
#define dfPlayerRX 8
#define dfPlayerTx 9
#define ESC1PIN 10
#define ESC2PIN 11

SoftwareSerial mp3Serial(dfPlayerRX,dfPlayerTx);
DFRobotDFPlayerMini dfPlayer;

Pixy2 pixy;

Servo esc1;
Servo esc2;

void initMp3(int);
void processMp3();
void printDetail(uint8_t, int);

void setup() {
    Serial.begin(9600);
    
    pinMode(pumpPin, OUTPUT);
    pinMode(ESC1PIN, OUTPUT);
    pinMode(ESC2PIN, OUTPUT);

    //Setup ESCs
    esc1.attach(ESC1PIN, 1000, 2000);
    esc1.writeMicroseconds(1000);
    esc2.attach(ESC1PIN, 1000, 2000);
    esc2.writeMicroseconds(1000);
    
    //Initialize MP3 player at max volume.
    initMp3(30);

    pixy.init();

}

void loop() {
    Serial.println("LEmon");
    delay(100);
}

void initMp3(int volume) {
    mp3Serial.begin(9600);
    Serial.println(F("Testing mp3"));
    if (!dfPlayer.begin(mp3Serial)) {  //Use softwareSerial to communicate with mp3.
        Serial.println(F("Unable to begin:"));
        Serial.println(F("1.Please recheck the connection!"));
        Serial.println(F("2.Please insert the SD card!"));
        while(true);
    }
    Serial.println(F("MP3 player online"));
    dfPlayer.setTimeOut(500);
    dfPlayer.volume(volume);
    dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
    dfPlayer.outputDevice(DFPLAYER_DEVICE_SD);
}

void processMp3() {
    //Prints errors of MP3, plays next song, and sets cannons to appropriate state.

    int mp3ReadType = 0, mp3Read = 0;
    
    mp3ReadType = dfPlayer.readType();
    mp3Read = dfPlayer.read();
    
    // if debug mode on, print info about sd card
    if(DEBUG_ARDUINO) {printDetail(mp3ReadType, mp3Read);}

    if (mp3ReadType == DFPlayerPlayFinished) {
        // Player just finished this song, will move on to next:

        dfPlayer.play(mp3Read+1); // Won't work for last song
        if (mp3Read%2 == 1) {
            //If last song was odd, cannon soundeffect should be next.
            digitalWrite(pumpPin, HIGH);
        }
        else {
            //Last song was even, so it must have been a cannon sound effect
            digitalWrite(pumpPin, LOW);
        }

        /*if (mp3Read == LAST SONG) {restart play?}*/
    }

}

void interpretMp3(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
  
}

int findHeadingOfOldestObject() {
    
}