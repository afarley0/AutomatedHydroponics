/* MAIN CODE FOR ARDUINOS: 
*/

#include "HX711.h"
#include <EEPROM.h>
#include <config.h>
#include <HX711_ADC.h>
#include <SparkFun_RHT03.h>

/* The Valve class encorporates all of the valves on the manifold, can also work for pump*/
class Valve{
  int pinOutput; //which pin will be toggled
  boolean stat; //is the valve on?
  char valveChar; //is it a tank or emitter valve
  int subzone; //which subzone
  bool reverse;
  public:
  Valve( bool isReverse, int pin, int inputSubzone = 0, char inputChar = ';'){
    pinOutput = pin; //sets the pin
    valveChar = inputChar; //sets the specific valve
    stat = false; //it starts off
    subzone = inputSubzone;
    reverse = isReverse; //For valves using the switch board, reverse == true. This just flips the LOW and HIGH ouputs
    pinMode(pinOutput, OUTPUT); //that pin outputs
    if(reverse){ //reverses output if reverse
      digitalWrite(pinOutput,HIGH);
    }
    else{
      digitalWrite(pinOutput,LOW);
    }
  }
  void on(){ 
    if(reverse){ //reverses output if reverse
      digitalWrite(pinOutput,LOW); //turns pin off for reversed valves
    }
    else{
      digitalWrite(pinOutput,HIGH); //turns pin on for reversed valves
    } //turns the valve on
    stat = true;
  }
  void off(){
    if(reverse){ //reverses output if reverse
      digitalWrite(pinOutput,LOW);
    }
    else{
      digitalWrite(pinOutput,HIGH);
    } //turns the valve off
    stat = false;
  }
  boolean getStat(){ //gets the status of valve
    return stat;
  }
  int getSubzone(){ //returns subzone
    return subzone;
  }
}; 
/*temporarily hard coded loadcell parameters*/
static byte loadCellClockInput = 14; //which clock pin will the load cells use
//static int loadCellPinInputs[] = {26, 27, 28, 29, 30, 31, 32, 33}; //what pins correspond with the load cells; CHECK THE LOAD CELL EXCEL SHEET FOR PINS, LOAD CELL NUMBERS, AND WIRING
//static long offsetCellPinInputs[] = {-8745, 49078,-8745, 49078,-8745, 49078,-8745, 49078}; //The offset values for the load cells; CHECK THE LOAD CELL EXCEL SHEET
//static float caliCellPinInputs[] = {-440.94, -420.80,-440.94, -420.80,-440.94, -420.80,-440.94, -420.80};//The calibration values for the load cells; CHECK THE LOAD CELL EXCEL SHEET

static byte loadCellPinInputs[] = {26,28,30,32,34,36,38,27,29,31,33,35,37,39,40,42,44,46,48,50,52,41,43,45,47,49,51,53};
static long offsetCellPinInputs[28]; //might not need this
static float caliCellPinInputs[28];  //or this


#define loadCellAmount 28 // How many load cells are you using
HX711 scale; //creates scale object
//  Temp and humidity Sensor: Looking at the front (grated side) of the RHT03, the pinout is as follows:
//   1     2        3       4
//  VCC  DATA  No-Connect  GND
const int RHT03_DATA_PIN = 25; // RHT03 data pin
RHT03 rht; // This creates a RTH03 object, which we'll use to interact with the sensor

volatile int volumeToWater; // the volume to total volume to water in mL

/*all the valves, flow sensors, pumps, and load cells initialize. READ CLASS DOCUMENTATION*/
//Valve subzonePump(21,0,'p'); //THIS IS A PUMP, NOT A VALVE, I'M JUST USING THE VALVE CLASS FOR SIMPLICITY

Valve tankValve1(false,2,0,'t'); //Tank 1 valve;
Valve tankValve2(false,3,1,'t'); //Tank 2 valve;
Valve tankValve3(false,4,2,'t'); //Tank 3 valve;
Valve tankValve4(false,5,3,'t'); //Tank 4 valve;
Valve tankValve5(false,6,0,'t'); //Tank 5 valve;
Valve tankValve6(false,7,1,'t'); //Tank 6 valve;
Valve tankValve7(false,8,2,'t'); //Tank 7 valve;
Valve tankValve8(false,9,3,'t'); //Tank 8 valve;
Valve tanks[8] = {tankValve1,tankValve2,tankValve3,tankValve4,tankValve5,tankValve6,tankValve7,tankValve8}; //easy array of tanks

Valve emitterValve1_1(false,A5,1,'e'); //Emitter 1; Zone 1
Valve emitterValve1_2(false,A6,2,'e'); //Emitter 2; Zone 1
Valve emitterValve1_3(false,A7,3,'e'); //Emitter 3; Zone 1
Valve emitterValve1_4(false,A8,4,'e'); //Emitter 4; Zone 1
Valve emitterValve2_1(false,10,1,'e'); //Emitter 1; Zone 2
Valve emitterValve2_2(false,11,2,'e'); //Emitter 2; Zone 2
Valve emitterValve2_3(false,12,3,'e'); //Emitter 3; Zone 2
Valve emitterValve2_4(false,13,4,'e'); //Emitter 4; Zone 2
Valve emitterValve3_1(false,15,1,'e'); //Emitter 1; Zone 3
Valve emitterValve3_2(false,16,2,'e'); //Emitter 2; Zone 3
Valve emitterValve3_3(false,17,3,'e'); //Emitter 3; Zone 3
Valve emitterValve3_4(false,18,4,'e'); //Emitter 4; Zone 3
Valve emitters[12] = {emitterValve1_1,emitterValve1_2,emitterValve1_3,emitterValve1_4,emitterValve2_1,emitterValve2_2,emitterValve2_3,emitterValve2_4,emitterValve3_1,emitterValve3_2,emitterValve3_3,emitterValve3_4}; //easy array of emitters


// THESE PINS ARE REVERSED
Valve pumpForward(false,16); //runs the pump in the forward direction
Valve pumpReverse(false,15); //runs the pump in the reverse direction 
Valve sVForward(false,14); //solenoid valve that directs the flow in the forward direction
Valve sVReverse(false,13); //solenoid valve that directs the flow in the reverse direction
//THESE PINS ARE REVERSED
Valve zone1(false,23); //activates zone 1
Valve zone2(false,24); //activates zone 2
Valve zone3(false,25); //activates zone 3
Valve zones[3] = {zone1, zone2, zone3}; //easy zone array

Valve SEMV(false,8); //Special Emitter Manifold Valve TO MADISON: ??? not sure about this, pin also needs to change
Valve SwitchControl(false,9); //TO MADISON: this needs to be changed to switch that controls the switch board

void setup() 
{
  Serial.begin(9600); //begins usb serial connection
  rht.begin(RHT03_DATA_PIN); // begins the temperature sensor readings
}

void loop(){
  readData(); //reads the serial port 
}

/*reads the data taken from the usb serial connection*/
void readData() 
{
   if (Serial.available() > 0){ //Checks if there is serial available 
    char go = Serial.read(); //Reads command
    if (go == 'w') //waters subzone; format: w 1 2 10 <- waters zone 1 subzone 2 for 10 milliliters
    {
      int zoneToWater = Serial.parseInt()-1; //sets subzone to water -1; The -1 is for C++ purposes
      int subzoneToWater = Serial.parseInt()-1; //sets subzone to water -1; The -1 is for C++ purposes
      volumeToWater =  Serial.parseInt(); //sets how much total water is needed in mL; INDIVIDUAL WATER MUST BE SET IN PYTHON
      
      //waters subzone. Uses the valve arrays for convient coding
      waterSubzone(zones[zoneToWater],emitters[subzoneToWater],tanks[subzoneToWater]);
      //TO MADISON: Not sure the proceedure, call me if you have questions
      //empty emitter
      //flushEmitter(zones[zoneToWater],emitters[subzoneToWater]);
      //empty manifold
      //flushManifold(tankValveA);
      
      Serial.println('g'); //confirms subzones watered
    }
    else if (go == 'z'){ //zeros all of the load cells
      int pinIndex = 0;
      for(int i = 1; i <= 271; i = i + 10){
        int eepromAddressOffset = i+5; //eeprom address for offset (6, 16, 26,...,...276)

        float cal = 0; //holds values for calibration and offset
        float offset = 0;
        scale.begin(loadCellPinInputs[pinIndex],loadCellClockInput);
        scale.tare();
        offset = scale.get_offset();
        EEPROM.put(eepromAddressOffset,offset);
        //Serial.println(pinIndex+1);
        delay(300);
        pinIndex++;
      }
      Serial.print('z'); //confirms load cell zeroing
    }
    else if (go == 'r'){ //read loads and returns array of readings
      int pinIndex = 0;
      for(int i = 1; i <= 271; i = i + 10){
        int eepromAddressCal = i; //eeprom addresses for calibration factor and offset
        int eepromAddressOffset = i+5;

        float cal = 0; //holds values for calibration and offset
        float offset = 0;
        scale.begin(loadCellPinInputs[pinIndex],loadCellClockInput);
        scale.set_scale(EEPROM.get(eepromAddressCal,cal));
        scale.set_offset(EEPROM.get(eepromAddressOffset,offset));
        
        float reading = scale.get_units(10); //gets the average from 10 readings
        if (reading < 0){ //if reading is negative, set equal to zero
          reading = 0.00;
        }
        if (reading < 10000){ //pad value with zeroes to create 8 bytes
          Serial.print('0');
        }
        if (reading < 1000){
          Serial.print('0');
        }
        if (reading < 100){
          Serial.print('0');
        }
        if (reading < 10){
          Serial.print('0');
        }
        Serial.print(reading); //prints the readings
        pinIndex++;
        delay(200);
      }

      Serial.println(";;;;;;;;"); //confirms load cells read
    }
    else if (go == 't'){ //read temperature and relative humidity
      //reads the temp and humidity
      //TO MADISON: If alex has problems with this, have him call me
      getTempHumid();
      //Serial.println(';'); //confirms load cells read
    }
    //else{ //anything else
      //Serial.println("e"); //e for error
    //}
  }
}
// Gets the temp and humidity sensor. may take a while
void getTempHumid()
{
  // Call rht.update() to get new humidity and temperature values from the sensor.
  int updateRet = rht.update();
  
  // If successful, the update() function Will return 1.
  // If update fails, it will return a value <0
  int stopCount = 0;
  while(updateRet != 1 && stopCount <10)
  {
    //Serial.println("E");
    delay(RHT_READ_INTERVAL_MS);
    // The humidity(), tempC(), and tempF() functions can be called -- after 
    // a successful update() -- to get the last humidity and temperature
    // value 
    updateRet = rht.update();
    //Serial.print(updateRet);
    stopCount++;
  }
  if(stopCount == 10){
    //figure something out
    Serial.print("99.9");
    delay(1000);
    Serial.print("99.9");
    delay(300);
  }
  else{
    float latestHumidity = rht.humidity();
    float latestTempC = rht.tempC();
    
    // Now print the values:
    Serial.print(latestHumidity, 1); //prints relative humidity
    delay(1000);
    Serial.print(latestTempC, 1); //prints temp in C
    delay(300);
  }
  stopCount = 0;
  Serial.println(";;;;");
}


/* reads the volume while watering or flushing*/
void readingVolume(int inputVolume){
  long startTime = millis(); //the watering start time
  float A = 32.195; //the ratio of mL per second while watering
  float B = -706.12; //the calibrated off set of volume
  long endTime = startTime + inputVolume*A + B; //calculating when to stop whating
  //DELETE: add emergency timer
  while(millis() < endTime){
    //waits until the volume is correct
  }
  startTime = 0; //resets the start time; perhaps obselete
}

/*Waters an individual Subzone. TO MADISON: Likely needs to be changed*/
void waterSubzone(Valve zone, Valve tank, Valve emitter){
  tank.on(); 
  sVForward.on();
  zone.on();
  pumpForward.on();
  delay(3000); //delays 3 seconds
  emitter.on();
  readingVolume(volumeToWater);
  pumpForward.off();
  sVForward.off();
  //tank.off();
  //emitter.off();
}

//Waters the same solution to the subzone. TO MADISON: needs more details
void waterSame(Valve zone, Valve tank, Valve emitter){
  tank.on();//turns on inputted tank valve
  zone.on();//turns on inputted zone
  emitter.on(); //turns on inputted emitter
  //TO MADISON: interlock check to ensure pump is on
  //TO MADISON: I'm not sure what there is to do here
}
//Waters different solutions to the subzone. TO MADISON: needs more details
/*
void waterDifferent(Valve zone, Valve tank, Valve emitter){
  tankValveA.on(); //activates the water tank: MIGHT BE REDUNDENT
  flushManifold(tankValveA); //flushes the manifold
  sVForward.off(); //turns off forward solenoid valve
  tankValveA.off(); //turns off water tank: MIGHT BE REDUNDENT
  pumpForward.off(); //turns off forward pump
  SEMV.on();//OPENS THE SEMV(Special Emitter Manifold Valve)
  zone.on(); //Turns on zone valve
  
  SEMV.off();//CLOSES THE SEMV
  zone.off();//closes zone valve
  
  sVForward.on();//Forward Valve on
  pumpForward.on();//foward pump on
  sVReverse.on();//Reverse Valve on
  pumpReverse.on();//Reverse pump on

  emitter.on();//opens solution valve
  delay(4000); //delays 4 seconds (Change if need be)

  pumpReverse.off();
  sVReverse.off();
  zone.on();
  emitter.on();
  //primeEmitter(zone, tank, emitter); //TO MADISON: Not sure how this will be implemented
  
  //TO MADISON: Not sure how to do this last part
  /*
   CHECK TO SEE IF THERE IS MORE WATERING TO DO IN THE ZONE
  if YES {TURN OFF SZV, REPEAT CLEARING SYSTEM, RERUN different solution}
  if NO {CLOSE SBZMV (SUBZONE MANIFOLD), CLOSE FV, CLOSE TMV, TURN OFF FP
   */
//}

/*Flushes the emitter: TO MADISON: Likely needs changing*/
void flushEmitter(Valve zone, Valve emitter){
  sVReverse.on();
  pumpReverse.on();
  delay(45000);  //delays for 45 seconds
  emitter.off();
  zone.off();
  pumpReverse.off();
}

/*Flushes the manifold: TO MADISON: I was a bit confused on how to do this. might have redundent features from water different solution*/
void flushManifold(Valve tank){
  tank.on(); // water tank
  sVForward.on();
  sVReverse.on();
  pumpForward.on();
  pumpReverse.on();
  delay(1000);//??? HOW LONG
  sVForward.off();
  tank.off();
  pumpForward.off();
  /* possible wrong or outdated
  delay(3000); //delays 3 seconds
  tank.off();
  delay(3000); //delays 3 seonds
  pumpForward.off();
  delay(3000); //delays 3 seconds
  sVForward.off();
  zone1.on();
  zone2.on();
  zone3.on();
  delay(4000);
  zone1.off();
  zone2.off();
  zone3.off();
  sVReverse.off();
  pumpReverse.off();
  */
}

void primeEmitter(Valve zone, Valve tank, Valve emitter){
  //TO MADISON: not sure how this is wanted. call zone, tank, and emitter in this function
}
