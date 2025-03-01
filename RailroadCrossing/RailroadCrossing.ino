#include <Servo.h>


#define DURATION_ENTRY 0 //Array entry location.  Don't change.
#define POSITION_ENTRY 1//Array entry location.  Don't change.


// For each servo create a uint32_t positionTimestamp variable, an int positionIndex variable, and a 
// uint32_t positionTable [][2] array.  The first number on each line is a duration, and the 2nd number on each line is the angle
// the last entry is 0 duration and the final position of the servo.

// First servo
Servo servo0;
uint32_t positionTimestamp_0 = 0;
int positionIndex_0= 0;
uint32_t positionTable_0_down[][2] = {  // First entry is duration in ms, second is postion in degrees.  Final entry has duration 0.
{  4000  , 0 },
{ 250 , 70  },
{ 250 , 65  },
{ 1000  , 70  },
{ 1000 , 70  },
{250 , 90  },
{ 250 , 85  },
{ 250 , 90  },
{ 250 , 85  },
{ 0 , 90  }
};

uint32_t positionTable_0_up[][2] = {  // First entry is duration in ms, second is postion in degrees.  Final entry has duration 0.
{  4000  , 90 },
{ 250 , 0  },
{ 250 , 5  },
{ 0  , 0  },
};


// First servo
Servo servo1;
uint32_t positionTimestamp_1 = 0;
int positionIndex_1= 0;
uint32_t positionTable_1_down[][2] = {  // First entry is duration in ms, second is postion in degrees.  Final entry has duration 0.
{  6000  , 0 },
{ 250 , 70  },
{ 250 , 60  },
{ 250 , 70  },
{ 250 , 60  },
{ 1000  , 70  },
{ 1000 , 70  },
{250 , 90  },
{ 250 , 80  },
{ 250 , 90  },
{ 250 , 80  },
{ 0 , 90  }
};

uint32_t positionTable_1_up[][2] = {  // First entry is duration in ms, second is postion in degrees.  Final entry has duration 0.
{  5000  , 90 },
{ 250 , 0  },
{ 250 , 5  },
{ 0  , 0  },
};


// Call start animiation to intialize the animation.  Pass it the position table, the timestamp by pointer, the position index by pointer, and the servo by pointer
void startAnimation(uint32_t positionTable[][2], uint32_t* positionTimestamp, int* positionIndex, Servo* servo)
{
  *positionIndex = 0;
  *positionTimestamp = millis() + positionTable[*positionIndex][DURATION_ENTRY];
  servo->write(positionTable[*positionIndex][POSITION_ENTRY]);
  Serial.begin(115200);
}


// Then continously call updateAnimation.  set interpolate values to false if you want delays then rapid movement from point to point or true
// if you want smooth movement.  The function does not block because it relies on millis().  calling it often enough that each degree of 
// servo rotation is specified will make things smoother.   If you go a long time without calling it it can skip steps to get caught up.
// it returns true for "keep calling me"  each time you call it until it reaches the end of the table, 
//  then it will return false and not modify the servo.  You can therefore call multiple servos in parallel in loop().


bool updateAnimation(uint32_t positionTable[][2], uint32_t* positionTimestamp, int* positionIndex, Servo* servo, bool interpolateValues)
{
  if (positionTable[*positionIndex][DURATION_ENTRY] == 0)
  {
    return false;  // Done.  Nothing to do
  }


  bool done = false;

  while (! done)
  {
    if (millis() > *positionTimestamp)
    {
      // Move to next position
      ++ *positionIndex;
      servo->write(positionTable[*positionIndex][POSITION_ENTRY]);
      *positionTimestamp += positionTable[*positionIndex][DURATION_ENTRY];
      
      if (positionTable[*positionIndex][DURATION_ENTRY] == 0)
      {
        return false;
      }
    }
    else
    {
      done = true;
      if (interpolateValues)
      {
        uint32_t startTime = *positionTimestamp - positionTable[*positionIndex][DURATION_ENTRY];
        uint32_t elapsedTime = millis() - startTime;
        int32_t delta = (int32_t)positionTable[*positionIndex + 1][POSITION_ENTRY] - (int32_t)positionTable[*positionIndex][POSITION_ENTRY];
        int32_t deltan =  delta * elapsedTime;
        int32_t deltad = deltan /  (int32_t) positionTable[*positionIndex][DURATION_ENTRY];
      
        servo->write(positionTable[*positionIndex][POSITION_ENTRY] + deltad);
        char s[80];
        sprintf(s,"%lu %lu %ld, %ld, %ld, %ld ",positionTable[*positionIndex][DURATION_ENTRY], elapsedTime,delta,deltan,deltad, positionTable[*positionIndex][POSITION_ENTRY] + deltad);
         Serial.println(s);
      }
    }
  }
  return(true); // Still in progress

}

void updateLEDs()
{
  if (millis() & 0x200)//Alternate every 1024mS
  {
    digitalWrite(4,LOW);
    digitalWrite(5,HIGH);
  }
  else
  {
    digitalWrite(4,HIGH);
    digitalWrite(5,LOW);
    
  }
  
}

void LEDsOff()
{
  digitalWrite(4,LOW);
  digitalWrite(5,LOW);
}


void setup() {
  // put your setup code here, to run once:

  servo0.attach(2);
  servo1.attach(3);
  pinMode(4,OUTPUT);
  pinMode(5,OUTPUT);
  LEDsOff();
}

void loop() {

 {
    uint32_t timeStamp = millis() + 2000;
    while (timeStamp > millis())
    {
      updateLEDs();
    }
 }
  startAnimation(positionTable_0_down, &positionTimestamp_0, &positionIndex_0, &servo0);
  startAnimation(positionTable_1_down, &positionTimestamp_1, &positionIndex_1, &servo1);
  bool notDoneYet = true;
  while( notDoneYet )
  {
   notDoneYet = updateAnimation(positionTable_0_down, &positionTimestamp_0, &positionIndex_0, &servo0,true);
   notDoneYet |= updateAnimation(positionTable_1_down, &positionTimestamp_1, &positionIndex_1, &servo1,true);
     updateLEDs();
  }
  {
    uint32_t timeStamp = millis() + 3000;
    while (timeStamp > millis())
    {
      updateLEDs();
    }
  }


   startAnimation(positionTable_0_up, &positionTimestamp_0, &positionIndex_0, &servo0);
  startAnimation(positionTable_1_up, &positionTimestamp_1, &positionIndex_1, &servo1);
  
  notDoneYet = true;
  while( notDoneYet )
  {
   notDoneYet = updateAnimation(positionTable_0_up, &positionTimestamp_0, &positionIndex_0, &servo0,true);
   notDoneYet |= updateAnimation(positionTable_1_up, &positionTimestamp_1, &positionIndex_1, &servo1,true);
     updateLEDs();
  }
  LEDsOff();
  delay(5000);

}
