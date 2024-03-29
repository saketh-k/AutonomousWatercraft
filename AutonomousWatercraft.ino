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
#define dfPlayerTx 7
#define ESC1PIN 9
#define ESC2PIN 10
#define RUDDERPIN 5

//Motion Definitions
#define STEER_MAX 90
#define STEER_MIN 170

//Differential Steering Definitions
#define DIFF_STEERING true
#define DIFF_STEER_DIFFERENCE 20
// #define DIFF_STEER_MIN_SPEED 20
// #define DIFF_STEER_MAX_SPEED 40
#define ESC_SPEED 30

#define CAN_CYCLES 10

SoftwareSerial mp3Serial(dfPlayerRX,dfPlayerTx);
DFRobotDFPlayerMini dfPlayer;

Pixy2 pixy;

Servo esc1;
Servo esc2;
Servo rudder;

void initMp3(int);
void processMp3();
void interpretMp3(uint8_t type, int value);
int findServoHeadingOfBlock(Block);
Block getOldestFromPixy();

int rudderAngle = 0;
int desiredRudderAngle = 0;
float error = 0;
int cannonState = 0;
int cannonCount = 0;

void setup() {
    //Begin serial communication for debugging
    Serial.begin(115200);
    
    //Setup Cannons
    pinMode(pumpPin, OUTPUT);

    //Setup + Initialize ESCs
    esc1.attach(ESC1PIN, 1000, 2000);
    esc1.writeMicroseconds(1000);
    esc2.attach(ESC2PIN, 1000, 2000);
    esc2.writeMicroseconds(1000);    
    delay(1000);
    esc1.write(90);
    esc2.write(90);
    delay(100);
    esc1.write(0);
    esc2.write(0);

    //Setup Rudder
    rudder.attach(RUDDERPIN);
    rudder.write((STEER_MAX + STEER_MIN)/2);
    //Initialize MP3 player at max volume.
    // initMp3(30);

    //Initialize Pixy Cam 2
    pixy.init();
    
    

}

void loop() {

    pixy.ccc.getBlocks();
    if (pixy.ccc.numBlocks > 0) {
        //If pixy detects a red object, make sure it's the oldest object
        desiredRudderAngle = findServoHeadingOfBlock(getOldestFromPixy());
        // Apply a digital filter to prevent servo from moving too quickly
        rudderAngle = rudderAngle * 0.8 + desiredRudderAngle * 0.2;
        
        rudder.write(rudderAngle);
        //Set ESC speed to move forward and (if Diffsteer is on) differential thrust
        controlESCs(desiredRudderAngle);
        Serial.println(rudderAngle);
        
    }
    else {
        //If nothing is detected, turn off the motors and set the rudder to be straight
        esc1.write(0);
        esc2.write(0);
        desiredRudderAngle = (STEER_MAX + STEER_MIN)/2;
        Serial.println("Nothing Detected");
    }
    //processMp3();
    cannonCount++;
    if (cannonCount > CAN_CYCLES) {
        //After ~1 second toggle the cannon either on or off
        cannonCount = 0;
        cannonState = !cannonState;
        Serial.print("Cannon Set to");
        Serial.print(cannonState);
    }
    digitalWrite(pumpPin, cannonState);
    delay(200);
}

void initMp3(int volume) {
    //Initialize communication with MP3 Module
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
    
    if (dfPlayer.available()) {
        mp3ReadType = dfPlayer.readType();
        mp3Read = dfPlayer.read();
        
        // if debug mode on, print info about sd card
        if(DEBUG_ARDUINO) {interpretMp3(mp3ReadType, mp3Read);}

        if (mp3ReadType == DFPlayerPlayFinished) {
            // Player just finished this song, will move on to next:

            dfPlayer.play(mp3Read+1%9); // Won't work for last song
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

}

void interpretMp3(uint8_t type, int value){
    //Unused helper function to communicate with MP3 Module
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

void controlESCs(int servoAngle) {
    //Convert a desired heading into a differential steering input for the boat to follow.
    //If DIFFSTEERING is False will only power servos when seeing something
    int diffAmt = 0;
    if (DIFF_STEERING) {
        diffAmt = map(servoAngle, STEER_MIN, STEER_MAX, -DIFF_STEER_DIFFERENCE, DIFF_STEER_DIFFERENCE);
    }
    esc1.write(ESC_SPEED + diffAmt);
    esc2.write(ESC_SPEED - diffAmt);
}

int findServoHeadingOfBlock(Block oldestBlock) {
    //Given a pixy block use feed forward control to convert that to a servo heading
    int ctr = 0;
    ctr = oldestBlock.m_x + oldestBlock.m_width/2;
    return map(ctr, 0, 320, STEER_MIN, STEER_MAX); 
}

Block getOldestFromPixy() {
    // Get all detected blocks from pixy and return oldest block
    int oldestObjectAge=-1;
    Block oldestBlock;
    Block currBlock;

    for (int i=0; i<pixy.ccc.numBlocks; i++) {
        currBlock = pixy.ccc.blocks[i];
        if (currBlock.m_age > oldestObjectAge) {
            oldestBlock = currBlock;
            oldestObjectAge = currBlock.m_age;
        }
    }    
    if (DEBUG_ARDUINO) {oldestBlock.print();}

    return oldestBlock;
}
