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
  public:
  Valve(int pin, int inputSubzone = 0, char inputChar = ';'){
    pinOutput = pin; //sets the pin
    valveChar = inputChar; //sets the specific valve
    stat = false; //it starts off
    subzone = inputSubzone;
    pinMode(pinOutput, OUTPUT); //that pin outputs
    digitalWrite(pinOutput,LOW);
  }
  void on(){ 
    digitalWrite(pinOutput,HIGH); //turns the valve on
    stat = true;
  }
  void off(){
    digitalWrite(pinOutput,LOW); //turns the valve off
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
static byte loadCellClockInput = 17; //which clock pin will the load cells use
//static int loadCellPinInputs[] = {26, 27, 28, 29, 30, 31, 32, 33}; //what pins correspond with the load cells; CHECK THE LOAD CELL EXCEL SHEET FOR PINS, LOAD CELL NUMBERS, AND WIRING
//static long offsetCellPinInputs[] = {-8745, 49078,-8745, 49078,-8745, 49078,-8745, 49078}; //The offset values for the load cells; CHECK THE LOAD CELL EXCEL SHEET
//static float caliCellPinInputs[] = {-440.94, -420.80,-440.94, -420.80,-440.94, -420.80,-440.94, -420.80};//The calibration values for the load cells; CHECK THE LOAD CELL EXCEL SHEET

static byte loadCellPinInputs[] = {26,28,30,32,34,36,38,27,29,31,33,35,37,39,40,42,44,46,48,50,52,41,43,45,47,49,51,53};
static long offsetCellPinInputs[28];
static float caliCellPinInputs[28];


#define loadCellAmount 28 // How many load cells are you using
HX711 scale; //creates scale object
//  Temp and humidity Sensor: Looking at the front (grated side) of the RHT03, the pinout is as follows:
//   1     2        3       4
//  VCC  DATA  No-Connect  GND
const int RHT03_DATA_PIN = 14; // RHT03 data pin
RHT03 rht; // This creates a RTH03 object, which we'll use to interact with the sensor

volatile int volumeToWater; // the volume to total volume to water in mL

/*all the valves, flow sensors, pumps, and load cells initialize*/
//Valve subzonePump(21,0,'p'); //THIS IS A PUMP, NOT A VALVE, I'M JUST USING THE VALVE CLASS FOR SIMPLICITY
Valve tankValveA(22,0,'t'); //Tank A1 valve; Water Tank
Valve tankValveB(23,1,'t'); //Tank A2 valve;
Valve tankValveC(24,2,'t'); //Tank A3 valve;
Valve tankValveD(25,3,'t'); //Tank A4 valve;
Valve tanks[4] = {tankValveA,tankValveB,tankValveC,tankValveD};

Valve emitterValveA(18,1,'e'); //Emitter A; Subzone 1
Valve emitterValveB(19,2,'e'); //Emitter B; Subzone 2
Valve emitterValveC(20,3,'e'); //Emitter C; Subzone 3
Valve emitterValveD(21,4,'e'); //Emitter D; Subzone 4
Valve emitters[4] = {emitterValveA,emitterValveB,emitterValveC,emitterValveD};

Valve pumpForward(16); //runs the pump in the forward direction
Valve pumpReverse(15); //runs the pump in the reverse direction 
Valve sVForward(14); //solenoid valve that directs the flow in the forward direction
Valve sVReverse(13); //solenoid valve that directs the flow in the reverse direction

Valve zone1(12); //activates zone 1
Valve zone2(11); //activates zone 2
Valve zone3(10); //activates zone 3
Valve zones[3] = {zone1, zone2, zone3};

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
      int zoneToWater = Serial.parseInt()-1; //sets subzone to water
      int subzoneToWater = Serial.parseInt()-1; //sets subzone to water
      volumeToWater =  Serial.parseInt(); //sets how much total water is needed in mL; INDIVIDUAL WATER MUST BE SET IN PYTHON
      
      //water subzone
      waterSubzone(zones[zoneToWater],emitters[subzoneToWater],volumeToWater);
      //empty emitter
      flushEmitter(zones[zoneToWater],emitters[subzoneToWater]);
      //empty manifold
      flushManifold(tankValveA);
      
      Serial.println('g'); //confirms subzones watered
    }
    else if (go == 'z'){ //zeros all of the load cells
      int pinIndex = 0;
      for(int i = 1; i < 271; i = i + 10){
        int eepromAddressOffset = i+5; //eeprom address for offset (6, 16, 26,...,...276)

        float cal = 0; //holds values for calibration and offset
        float offset = 0;
        scale.begin(loadCellPinInputs[pinIndex],loadCellClockInput);
        scale.tare();
        offset = scale.get_offset();
        EEPROM.put(eepromAddressOffset,offset);
        delay(1000);
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
          reading = 0;
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

      Serial.print(";;;;;;;;"); //confirms load cells read
    }
    else if (go == 't'){ //read temperature and relative humidity
      //reads the temp and humidity
      getTempHumid();
      Serial.println(';'); //confirms load cells read
    }
    else{ //anything else
      Serial.println("e"); //e for error
    }
  }
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

/*Waters an individual Subzone*/
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

/*Flushes the emitter*/
void flushEmitter(Valve zone, Valve emitter){
  sVReverse.on();
  pumpReverse.on();
  delay(45000);  //delays for 45 seconds
  emitter.off();
  zone.off();
  pumpReverse.off();
}

/*Flushes the manifold*/
void flushManifold(Valve tank){
  sVForward.on();
  tank.on(); // water tank
  pumpForward.on();
  pumpReverse.on();
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
}


void getTempHumid()
{
  // Call rht.update() to get new humidity and temperature values from the sensor.
  int updateRet = rht.update();
  
  // If successful, the update() function will return 1.
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
    stopCount++;
  }
  if(stopCount == 9){
    //figure something out
  }
  stopCount = 0;
  float latestHumidity = rht.humidity();
  float latestTempC = rht.tempC();
  float latestTempF = rht.tempF();
  
  // Now print the values:
  Serial.println("Humidity: " + String(latestHumidity, 1) + " %");
  Serial.println("Temp (F): " + String(latestTempF, 1) + " deg F");
  Serial.println("Temp (C): " + String(latestTempC, 1) + " deg C");
}


void shutDown(){
  
}
