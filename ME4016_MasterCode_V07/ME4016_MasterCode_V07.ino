/* MAIN CODE FOR ARDUINOS: 
*/

#include "HX711.h"
#include <EEPROM.h>
#include <config.h>
#include <HX711_ADC.h>
#include <DHT.h>
#include <DHT_U.h>

//#include <SparkFun_RHT03.h>

/* The Valve class encorporates all of the valves on the manifold, can also work for pump*/
class Valve{
  int pinOutput; //which pin will be toggled
  boolean stat; //is the valve on?
  bool reverse;
  public:
  
  Valve(bool isReverse, int pin)
  {
    pinOutput = pin; //sets the pin
    stat = false; //it starts off
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
    if(reverse){ //turns on low trigger switches
      digitalWrite(pinOutput,LOW); //turns pin low for reversed valves
    }
    else{
      digitalWrite(pinOutput,HIGH); //turns pin high for regular valves
    }
    stat = true;
  }
  void off(){
    if(reverse){ //turns off low trigger switches
      digitalWrite(pinOutput,HIGH);
    }
    else{
      digitalWrite(pinOutput,LOW);
    } //turns the valve off
    stat = false;
  }
  boolean getStat(){ //gets the status of valve
    return stat;
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
#define DHTPIN 25
#define DHTTYPE DHT22

HX711 scale; //creates scale object
DHT dht(DHTPIN, DHTTYPE);
//  Temp and humidity Sensor: Looking at the front (grated side) of the RHT03, the pinout is as follows:
//   1     2        3       4
//  VCC  DATA  No-Connect  GND
//const int RHT03_DATA_PIN = 25; // RHT03 data pin
//RHT03 rht; // This creates a RTH03 object, which we'll use to interact with the sensor

//volatile int volumeToWater; // the volume to total volume to water in mL

/*all the valves, flow sensors, pumps, and load cells initialize. READ CLASS DOCUMENTATION*/

Valve tankValve1(false,2); //Tank 1 valve;
Valve tankValve2(false,3); //Tank 2 valve;
Valve tankValve3(false,4); //Tank 3 valve;
Valve tankValve4(false,5); //Tank 4 valve;
Valve tankValve5(false,6); //Tank 5 valve;
Valve tankValve6(false,7); //Tank 6 valve;
Valve tankValve7(false,8); //Tank 7 valve;
Valve tankValve8(false,9); //Tank 8 valve;
Valve tank[9] = {tankValve1,tankValve1, tankValve2, tankValve3, tankValve4, tankValve5, tankValve6, tankValve7, tankValve8};

Valve emitterValve1_1(false,A6); //Emitter 1; Zone 1
Valve emitterValve1_2(false,A7); //Emitter 2; Zone 1
Valve emitterValve1_3(false,A8); //Emitter 3; Zone 1
Valve emitterValve1_4(false,A9); //Emitter 4; Zone 1
Valve emitterValve2_1(false,10); //Emitter 1; Zone 2
Valve emitterValve2_2(false,11); //Emitter 2; Zone 2
Valve emitterValve2_3(false,12); //Emitter 3; Zone 2
Valve emitterValve2_4(false,13); //Emitter 4; Zone 2
Valve emitterValve3_1(false,15); //Emitter 1; Zone 3
Valve emitterValve3_2(false,16); //Emitter 2; Zone 3
Valve emitterValve3_3(false,17); //Emitter 3; Zone 3
Valve emitterValve3_4(false,18); //Emitter 4; Zone 3
Valve emitter[13] = {emitterValve1_1, emitterValve1_1, emitterValve1_2, emitterValve1_3, emitterValve1_4, emitterValve2_1, emitterValve2_2, emitterValve2_3, emitterValve2_4, emitterValve3_1, emitterValve3_2, emitterValve3_3, emitterValve3_4};

Valve zone1BleedValve(false, A10); //Zone 1 Emitter manifold bleed valve
Valve zone2BleedValve(false, A11); //Zone 1 Emitter manifold bleed valve
Valve zone3BleedValve(false, A12); //Zone 1 Emitter manifold bleed valve
Valve bleedValve[4] = {zone1BleedValve, zone1BleedValve, zone2BleedValve, zone3BleedValve};

Valve switchControl(false,A5); //Power switch for switchboard

// THESE PINS ARE REVERSED
Valve forwardPump(true,19); //runs the forward pump
Valve reversePump(true,20); //runs the reverse pump
Valve reverseValve(true,22); //Allows reverse pump to drain emitter manifold

Valve zone1Valve(true,23); //activates zone 1
Valve zone2Valve(true,24); //activates zone 2
Valve zone3Valve(true,25); //activates zone 3
Valve zone[4] = {zone1Valve, zone1Valve, zone2Valve, zone3Valve};

void setup() 
{
  Serial.begin(9600); //begins usb serial connection
  //rht.begin(RHT03_DATA_PIN); // begins the temperature sensor readings
  
}

void loop(){
  readData(); //reads the serial port 
}

/*reads the data taken from the usb serial connection*/
void readData() 
{
  if (Serial.available() > 0){ //Checks if there is serial available 
    char go = Serial.read(); //Reads command
    if (go == 'w') { //waters subzone; format: w 1 2 10 <- waters zone 1 subzone 2 for 10 milliliters
      int wateringFunction = Serial.parseInt(); 
      int zoneToWater = Serial.parseInt(); 
      int subZoneToWater = Serial.parseInt(); 
      int solutionToEmit = Serial.parseInt();
      int volumeToWater =  Serial.parseInt(); 
      int waterTank = Serial.parseInt();
      long waterTime = (((volumeToWater - 10)/.5) + 10) * 1000; //(((Desired - DripAfterEmitt)/DripRate) + TimeTillDrip)*1000ms
      switchControl.on();
      
      if (wateringFunction == 1) {
        fillZoneManifold(solutionToEmit, zoneToWater);
        emitter[subZoneToWater].on();
        delay(waterTime); //~90 Seconds
        emitter[subZoneToWater].off();
        //Serial.println('1');  
      }
      else if (wateringFunction == 2) {
        emitter[subZoneToWater].on();
        delay(waterTime); //~90 Seconds
        emitter[subZoneToWater].off();
        //Serial.println('2');  
      }
      else if (wateringFunction == 3) {
        emitter[subZoneToWater].on();
        delay(waterTime); //~90 Seconds
        emitter[subZoneToWater].off();
        drainZoneManifold(solutionToEmit, zoneToWater, waterTank);  
        //Serial.println('3');  
      }
      else if (wateringFunction == 4) {
        fillZoneManifold(solutionToEmit, zoneToWater);
        emitter[subZoneToWater].on();
        delay(waterTime); //~90 Seconds
        emitter[subZoneToWater].off();
        drainZoneManifold(solutionToEmit, zoneToWater, waterTank); 
        //Serial.println('4');  
      }
      else if (wateringFunction == 5) {
        flushWholeSystem();
        //Serial.println('5');  
      }
      else {
        Serial.println("e");
      }
      Serial.print('g'); //confirms subzones watered
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
      dht.begin();
      getTempHumid();
      //Serial.println(';'); //confirms load cells read
    }
    //else{ //anything else
      //Serial.println("e"); //e for error
    //}
  }
}

void getTempHumid(){
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.print("99.99");
    delay(500);
    Serial.print("99.99");
    delay(500);
    Serial.print(";;;;;");
  }
  else {
    Serial.print(h);
    delay(500);
    Serial.print(t);
    delay(500);
    Serial.print(";;;;;");
  }
}
// Gets the temp and humidity sensor. may take a while
/*
void getTempHumid()
{
  // Call rht.update() to get new humidity and temperature values from the sensor.
  //int updateRet = rht.update();
  
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
  Serial.println(";;;;;");
}
*/
void fillZoneManifold(int tankNumber, int zoneNumber)
{
  tank[tankNumber].on();
  bleedValve[zoneNumber].on();
  zone[zoneNumber].on();
  forwardPump.on();
  reversePump.on();
  delay(35000); //30 Sec
  reversePump.off();
  //forwardPump.off();
  delay(5000);
  bleedValve[zoneNumber].off();  
}

void drainZoneManifold(int tankNumber, int zoneNumber, int waterTank)
{
  tank[tankNumber].off();
  zone[zoneNumber].on();
  forwardPump.off();
  reverseValve.on();
  reversePump.on();
  delay(60000); //60 Sec
  zone[zoneNumber].off();
  tank[waterTank].on();
  forwardPump.on();
  delay(15000); //15 Sec
  forwardPump.off();
  tank[waterTank].off();
  delay(2500);
  reverseValve.off();
  delay(500); //.5 Sec to minimize dripping
  reversePump.off(); //Should be @ 0 Energy. No solution/water in manifolds. Just water from Tank manifold to Zone Valve. Everything is off.
  zone[zoneNumber].off();
}


void flushWholeSystem()
{
  int xxx = 1;
  int yyy = 0;
  int zzz = 0;
  
  tank[7].on();
  while (xxx < 4)
  {
    zone[xxx].on();
    bleedValve[xxx].on();
    forwardPump.on();
    reversePump.on();
    delay(45000);
    reversePump.off();
    bleedValve[xxx].off();
    if (xxx == 1) {
      int yyy = 1;
      int zzz = 5;
    }
    else if (xxx == 2) {
      int yyy = 5;
      int zzz = 9;
    }
    else if (xxx == 3) {
      int yyy = 9;
      int zzz = 13;
    }
    while (yyy < zzz) //Cycles Through subzone Valves and flushes them with water for 20 seconds
    {
      emitter[yyy].on();
      delay(20000);
      emitter[yyy].off();
      yyy ++;
    }
    forwardPump.off();
    reverseValve.on();
    reversePump.on();
    delay(45000); //Drain the emitter Manifold
    if (xxx == 1) {
      int yyy = 1;
      int zzz = 5;
    }
    else if (xxx == 2) {
      int yyy = 5;
      int zzz = 9;
    }
    else if (xxx == 3) {
      int yyy = 9;
      int zzz = 13;
    }
    while (yyy < zzz) //Cycles Through subzone Valves and drains emitter arrays for 20 sec. Will have to open Ball valves to allow for draining
    {
      emitter[yyy].on();
      delay(20000);
      emitter[yyy].off();
      yyy ++;
    }
    zone[xxx].off();
    reverseValve.off();
    delay (500);
    reversePump.off();
    xxx ++;
  }
  tank[7].off(); //Should be in 0 Energy State    
}
