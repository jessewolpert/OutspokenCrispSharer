
// MUSIC
#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>
#define BREAKOUT_RESET  9
#define BREAKOUT_CS     10
#define BREAKOUT_DCS    8
// These are the pins used for the music maker shield
#define SHIELD_RESET  -1
#define SHIELD_CS     7
#define SHIELD_DCS    6
#define CARDCS 4
#define DREQ 3
Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

//MOTOR
#include <Wire.h>
#include <Adafruit_MotorShield.h>
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_StepperMotor *myMotor = AFMS.getStepper(200, 2);

//SCALE
#include "HX711.h"
#define DOUT  8
#define CLK  5
HX711 scale(DOUT, CLK);

//CRISPSHARER
float calibration_factor = 6000; //calibration factor for the scale
long setupTime = 0; // used to allow time to calibrate
bool crispSharingStarted = false; // indicates when the bowl is empty
float currentWeight = 0; // weight of bowl and crisps
long lastTimeICalled = 0; //time counter for mp3 playing
int maxCanBe; // the maximum the scale can weigh - used to account for pushing bowl when taking crisps - giving wrong readings
int totalCrispsTaken; // sum of crisps taken in one go
int minimumCrispsHaveToTake = 0; // the amount of crisps taken by the previous player - so the least you can take in your go
bool needToResetTime = true; // used for timing of mp3s in
int crispsJustTaken = 0; // crisps taken since last crisps taken
bool forward = false; //direction of cage movement


//*****************************************************
void setup() {

  Serial.begin(9600);

  //SET UP SCALE
  scale.set_scale(calibration_factor); //set the scale to the calibration factor
  scale.tare();  //Reset the scale to 0

  //SET UP MOTOR
  AFMS.begin();
  myMotor->setSpeed(10);  // 10 rpm

  //ALLOW TIME FOR CALIBRATION - 10 SECONDS
  setupTime = millis();
  while (millis() - setupTime < 10000)
  {}

  //CHECK MUSIC SET UP
  if (! musicPlayer.begin()) { // initialise the music player
    Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
    while (1);
  }
  Serial.println(F("VS1053 found"));

  if (!SD.begin(CARDCS)) {
    Serial.println(F("SD failed, or not present"));
    while (1);  // don't do anything more
  }
  musicPlayer.setVolume(20, 20); // the higher the number the lower the volume
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int
  lastTimeICalled = millis();
  randomSeed(analogRead(0));


}//********************************************************

void loop() {

  if (!crispSharingStarted &&  currentWeight > 1328) //call for bowl to be filled if empty bowl on plinth
  {
    bowlFillingRequest();
  }

  if ((currentWeight > 1328) && crispSharingStarted) // start sharing after first crisp taken
  {
    shareCrisps();
  }

  if ((currentWeight <= 1327) && (currentWeight > 0) && crispSharingStarted) // start again as crisps are finished - 1327 is weight of bowl
    // have to check greater than 0 as sometimes goes negative values
  {
    crispsFinished();
  }

}//end of loop
//*******************************************************

void bowlFillingRequest()
{
  //increasingly desperate request to fill bowl over time.
  needToResetTime = true;
  if ((millis() - lastTimeICalled) > 10000 && (millis() - lastTimeICalled < 10500))
  {
    musicPlayer.playFullFile("Fill1.mp3"); //fill my bowl
  }

  if ((millis() - lastTimeICalled) > 15000 && (millis() - lastTimeICalled < 15500))
  {
    musicPlayer.playFullFile("Fill2.mp3"); //fill insistently
  }

  if ((millis() - lastTimeICalled) > 20000 && (millis() - lastTimeICalled < 20500))
  {
    musicPlayer.playFullFile("Fill3.mp3"); //come on
  }
  if ((millis() - lastTimeICalled) > 25000 && (millis() - lastTimeICalled < 25500))
  {
    musicPlayer.playFullFile("Fill4.mp3"); //hurry up
  }
  if ((millis() - lastTimeICalled) > 30000 && (millis() - lastTimeICalled < 30500))
  {
    musicPlayer.playFullFile("Fill5.mp3"); //get a move on
  }
  if ((millis() - lastTimeICalled) > 35000 && (millis() - lastTimeICalled < 35500))
  {
    musicPlayer.playFullFile("Fill6.mp3"); //fucking bowl
    lastTimeICalled = millis(); // reset the timer if no filling has taken place
  }
  //check if the bowl has been filled
  currentWeight = int(scale.get_units() * 100 ); // weight the bowl and crisps
  if (currentWeight > 1375) //if crisps have been put on
  {
    crispSharingStarted = true; // get out of bowl filling part of loop
    minimumCrispsHaveToTake = 0; // no crisps have been taken yet
    totalCrispsTaken = 0; // no crisps have been taken yet
    musicPlayer.playFullFile("Cageon.mp3"); // request for cage to be put on
    delay(6000); //allow tiem for cage to be put on
    musicPlayer.playFullFile("Turning.mp3"); // tell the user you are turning
    myMotor->step(305, FORWARD, SINGLE); // move forward 180 degrees
    maxCanBe = int(scale.get_units() * 100 ); // check weight again in case didnt get final weight before turning and more crisps were taken
    if (maxCanBe > 3) // if there are still crisps in the bowl
    { musicPlayer.playFullFile("Crisp1A.mp3"); //tell them to take first crisp
    }
  }
}

//*******************************************************

void shareCrisps()
{ int randomNum; // get a random number
  randomNum = random(1, 6); // get a random number between one and six
  currentWeight = int(scale.get_units() * 100 ); //check weight of crisps in bowl
  crispsJustTaken = maxCanBe - currentWeight; // compare to weight before crisps taken
  if (crispsJustTaken > 0) //if some crisps have been taken
  { totalCrispsTaken = totalCrispsTaken + crispsJustTaken; // start summing the crisps taken
    maxCanBe = currentWeight; // set the maximum weight it can be to the current weight
  }
  if ((crispsJustTaken > 90 ) && (currentWeight > 1328)) // too many taken
  {
    musicPlayer.playFullFile("Back.mp3");//put crisps back
  }
  else if ((crispsJustTaken >= 3) && (currentWeight > 1328)) //crisps taken and the bowl is not empty
  {
    // if you haven't taken enough crisps, but you have taken a lot and there are still crisps in bowl
    if ((totalCrispsTaken < minimumCrispsHaveToTake) && (crispsJustTaken > 4) && (int(scale.get_units() * 100) > 1328))
    {

      if (randomNum == 1)
      {
        musicPlayer.playFullFile("Greedy1.mp3");
      }
      if (randomNum == 2)
      {
        musicPlayer.playFullFile("Greedy2.mp3");
      }
      if (randomNum == 3)
      {
        musicPlayer.playFullFile("Greedy3.mp3");
      }
      if (randomNum == 4)
      {
        musicPlayer.playFullFile("Greedy4.mp3");
      }

    }
    // if you haven't taken enough crisps, but you have taken only a few  and there are still crisps in bowl
    else if  ((totalCrispsTaken < minimumCrispsHaveToTake) && (crispsJustTaken <= 4 ) && (int(scale.get_units() * 100) > 1328))
    {

      if (randomNum == 1)
      {
        musicPlayer.playFullFile("Moderate.mp3");
      }
      if (randomNum == 2)
      {
        musicPlayer.playFullFile("Crisp2.mp3");
      }
      if (randomNum == 3)
      {
        musicPlayer.playFullFile("Crisp3.mp3");
      }
      if (randomNum == 4)
      {
        musicPlayer.playFullFile("Cough1.mp3");
      }
    }
    // if you HAVE taken enough crisps and there are still crisps in bowl
    else if ((totalCrispsTaken - minimumCrispsHaveToTake >= 3 ) && (int(scale.get_units() * 100) > 1328)) //here we want to give another turn to the other person
    {

      if (randomNum == 1)
      {
        musicPlayer.playFullFile("Enough.mp3");
      }
      if (randomNum == 2)
      {
        musicPlayer.playFullFile("Dont.mp3");
      }
      if (randomNum == 3)
      {
        musicPlayer.playFullFile("Wow.mp3");
      }
      if (randomNum == 4)
      {
        musicPlayer.playFullFile("Leave.mp3");
      }


      musicPlayer.playFullFile("Turning.mp3");//move hands away as turning cage
      //alternate direction of motor
      if (forward)
      { myMotor->step(305, FORWARD, SINGLE);
        forward = false;
      }
      else if (!forward)
      { myMotor->step(305, BACKWARD, SINGLE);
        forward = true;
      }

      minimumCrispsHaveToTake = totalCrispsTaken;
      totalCrispsTaken = 0;
      currentWeight = int(scale.get_units() * 100 );//double checking we have the right weight
      maxCanBe = currentWeight; //setting this to be the maximum weight
      musicPlayer.playFullFile("Crisp1B.mp3");//take a crisp second time
      lastTimeICalled = millis();
    } //end of if enough crisps have been taken
  }
  else { // if no crisps have been taken
    if (needToResetTime) //reset the lasttimeicalled if first time in here
    { needToResetTime = false;
      lastTimeICalled = millis();
    }

    //do some funny things and cajole them into taking crisps if none have been taken - check bowl is still there and crisps are still there
    if ((millis() - lastTimeICalled) > 5000 && (millis() - lastTimeICalled < 5200) && (int(scale.get_units() * 100) > 1328))
    {
      musicPlayer.playFullFile("Sneeze.mp3");
    }

    if ((millis() - lastTimeICalled) > 10000 && (millis() - lastTimeICalled < 10200) && (int(scale.get_units() * 100) > 1328))
    {
      musicPlayer.playFullFile("Hungry.mp3");
    }

    if ((millis() - lastTimeICalled) > 15000 && (millis() - lastTimeICalled < 15200)  && (int(scale.get_units() * 100) > 1328))
    {
      musicPlayer.playFullFile("Dontlike.mp3");
    }

    if ((millis() - lastTimeICalled) > 20000 && (millis() - lastTimeICalled < 20200)  && (int(scale.get_units() * 100) > 1328))
    {
      musicPlayer.playFullFile("Cough.mp3");

    }

    if ((millis() - lastTimeICalled) > 25000 && (millis() - lastTimeICalled < 25200) && (int(scale.get_units() * 100) > 1328))
    {
      musicPlayer.playFullFile("Sniff.mp3");
      lastTimeICalled = millis();
    }

  }

}


//***************************************************

void crispsFinished()
{
  musicPlayer.playFullFile("All.mp3");//all the crisps have been taken
  musicPlayer.playFullFile("Crying.mp3"); //start crying
  musicPlayer.playFullFile("Tea.mp3"); //request some tea
  crispSharingStarted = false; // crisps finished so crisp sharing hasnt started
  currentWeight = 0; // set the current weight to zero
  lastTimeICalled = millis(); //reset the timer for bowl filling
  maxCanBe = 0; // reset the maximum weight
  totalCrispsTaken = 0; // reset the total crisps taken to 0
  minimumCrispsHaveToTake = 0; // reset the minimum crisps to take to 0
  needToResetTime = true; // reset the timer for when no crisps are taken during the sharing.

}


