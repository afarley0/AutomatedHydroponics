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
  Valve(int pin, int inputSubzone, char inputChar)  {
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
static byte loadCellClockInput = 16; //which clock pin will the load cells use
static int loadCellPinInputs[] = {18, 20}; //what pins correspond with the load cells; CHECK THE LOAD CELL EXCEL SHEET FOR PINS, LOAD CELL NUMBERS, AND WIRING
static long offsetCellPinInputs[] = {-8745, 49078}; //The offset values for the load cells; CHECK THE LOAD CELL EXCEL SHEET
static float caliCellPinInputs[] = {-440.94, -420.80};//The calibration values for the load cells; CHECK THE LOAD CELL EXCEL SHEET

//static byte loadCellPinInputs[] = {26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53};
//static int offsetCellPinInputs[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//static int caliCellPinInputs[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#define loadCellAmount 2 // How many load cells are you using
HX711 scale1[loadCellAmount]; //creates scale object

const int RHT03_DATA_PIN = 14; // RHT03 data pin
RHT03 rht; // This creates a RTH03 object, which we'll use to interact with the sensor

#define FLOWSENSORPIN 15 //The pin the flow sensor goes to


volatile uint16_t pulses = 0;// count how many pulses
volatile uint8_t lastflowpinstate;// track the state of the pulse pin
volatile uint32_t lastflowratetimer = 0; // you can try to keep time of how long it is between pulses
volatile float flowrate; // and use that to calculate a flow rate
volatile int volumeToWater; // the volume to total volume to water in mL
// Interrupt is called once a millisecond, looks for any pulses from the sensor!
SIGNAL(TIMER0_COMPA_vect) {
  uint8_t x = digitalRead(FLOWSENSORPIN); //reads the flow sensor
  
  if (x == lastflowpinstate) { 
    lastflowratetimer++; // adds time to the flow rate timer; used for the flow rate (partially obselete)
    return; // nothing changed!
  }
  
  if (x == HIGH) {
    //low to high transition!
    pulses++; //adds to the number of tracked pulses
  }
  lastflowpinstate = x; //sets the last flow pin state
  
}

void useInterrupt(boolean v) { //uses the timer interrupt; Black Magic Register level code; DO NOT TOUCH;
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
  } else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
  }
}



/*all the valves, flow sensors, pumps, and load cells initialize*/
Valve subzonePump(21,0,'p'); //THIS IS A PUMP, NOT A VALVE, I'M JUST USING THE VALVE CLASS FOR SIMPLICITY
Valve tankValveA(22,0,'t'); //Tank A1 valve; Water Tank
Valve tankValveB(23,1,'t'); //Tank A2 valve;
Valve tankValveC(24,2,'t'); //Tank A3 valve;
Valve tankValveD(25,3,'t'); //Tank A4 valve;
 
Valve emitterValveA(17,1,'e'); //Emitter A; Subzone 1
Valve emitterValveB(18,2,'e'); //Emitter B; Subzone 2
Valve emitterValveC(19,3,'e'); //Emitter C; Subzone 3
Valve emitterValveD(20,4,'e'); //Emitter D; Subzone 4

void setup() 
{
  Serial.begin(9600); //begins usb serial connection
  pinMode(FLOWSENSORPIN, INPUT); //sets the flow sensor as an input
  digitalWrite(FLOWSENSORPIN, HIGH); //write the flow sensor pin high; stolen code from library
  lastflowpinstate = digitalRead(FLOWSENSORPIN); //also reads flow sensor state
  rht.begin(RHT03_DATA_PIN); // begins the temperature sensor readings
  for(int i = 0; i < loadCellAmount; i++){

    scale1[i].begin(loadCellPinInputs[i],loadCellClockInput); //begins each load cell
    scale1[i].set_scale(caliCellPinInputs[i]); //sets the scale calibration
    scale1[i].set_offset(offsetCellPinInputs[i]); //sets the scale offset
  }
}

void loop(){
  readData(); //reads the serial port 
  
}

/*reads the data taken from the usb serial connection*/
void readData() 
{
   if (Serial.available() > 0){ //Checks if there is serial available 
    char go = Serial.read(); //Reads command
    if (go == 'w') //waters subzone; format: w 1 10 <- waters subzone 1 for 10 milliliters
    {
      int subzoneToWater = Serial.parseInt(); //sets subzone to water
      volumeToWater =  Serial.parseInt(); //sets how much total water is needed in mL; INDIVIDUAL WATER MUST BE SET IN PYTHON
      switch (subzoneToWater) {
        case 1:
          waterSubzone(tankValveA, emitterValveA, subzonePump); //watering subzone 1
          break;
        case 2:
          waterSubzone(tankValveB, emitterValveB, subzonePump); //watering subzone 2
          break;
        case 3:
          waterSubzone(tankValveC, emitterValveC, subzonePump); //watering subzone 3
          break;
        case 4:
          waterSubzone(tankValveD, emitterValveD, subzonePump); //watering subzone 4
          break;
      }
      
      Serial.println('g'); //confirms subzones watered
    }
    else if (go == 'z'){ //zeros all of the load cells
      Serial.println('g'); //confirms load cell zeroing
    }
    else if (go == 'r'){ //read loads and returns array of readings
      for(int i = 0; i < loadCellAmount;i++){
        float reading = scale1[i].get_units(10); //gets the average from 10 readings
        Serial.println(reading); //prints the readings
      }
      Serial.println('g'); //confirms load cells read
    }
    else if (go == 't'){ //read loads and returns array of readings

      Serial.println(readHumid());
      Serial.println(readTemp());
      Serial.println('g'); //confirms load cells read
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
  /*The pulses are a back up in case the timer fails, it's less accurate but it provides redundancy*/
  pulses = 0; // setting the pulses to 0
  float C = 0.2324; //the ratio of mL per pulse while watering
  float D = 14.306 + 20 ;//the calibrated off set of volume
  uint16_t endPulses = inputVolume*C + D; //the end pulse
  while(millis() < endTime && pulses < endPulses){
    //waits until the volume is correct
  }
  pulses = 0; //resets pulses
  lastflowpinstate = LOW; //resets the flow pin state
  flowrate = 0; //turns off flow rate
  lastflowratetimer = 0; //resets the flow rater timer; perhaps obselete
  startTime = 0; //resets the start time; perhaps obselete
}

/*Waters an individual Subzone*/
void waterSubzone(Valve tank, Valve emitter, Valve pump){
  Serial.println("watering");
  emitter.on();
  tank.on();
  pump.on();
  readingVolume(volumeToWater);
  pump.off();
  tank.off();
  emitter.off();
  Serial.println("finished");
}
/*Reads the relative humidity*/
float readHumid(){
  // If successful, the update() function will return 1.
  // If update fails, it will return a value <0
  long startTime = millis()+5000;
  while(rht.update()< 1)
  {
    if(millis()>startTime){
      return 0;
    }
    delay(RHT_READ_INTERVAL_MS);
  }
  float latestHumidity = rht.humidity();
  return latestHumidity;
}
/*Reads the temperature in C*/
float readTemp(){
  // If successful, the update() function will return 1.
  // If update fails, it will return a value <0
  long startTime = millis()+5000;
  while(rht.update()< 1)
  {
    if(millis()>startTime){
      return 0;
    }
    delay(RHT_READ_INTERVAL_MS);
  }
  float latestTempC = rht.tempC();
  return latestTempC;
}
